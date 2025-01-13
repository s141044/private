
#ifndef TONEMAP_HLSL
#define TONEMAP_HLSL

#include"math.hlsl"
#include"packing.hlsl"

cbuffer tonemap_cbuffer
{
	int2	size;
	float2	inv_size;

	float	base_key;
	float	ws;
	float	wc;
	float	we;

	int		N;
	float	blend_ratio;
	int2	reserved;
};

float3 tonemapping(float3 rgb, float key, float2 L_avg_white)
{
	float L = luminance(rgb);
	float L_avg = L_avg_white[0];
	float L_white = L_avg_white[1];
	float L_orig = L;

	//L_white‚ª‘å‚«‚­‚È‚è‚·‚¬‚é‚Æ‚«‚ê‚¢‚ÉŒ©‚¦‚â‚ñ...
	L_white = 1;
	//L_white *= (key / L_avg);

	L *= (key / L_avg);
	L *= (1 + L / pow2(L_white)) / (1 + L);

	//return saturate(rgb * L / L_orig);
	return rgb * L / L_orig;
}

float encode_weight(float w)
{
	//return sqrt(w);
	return log(w);
	//return sqrt(sqrt(w));
}
float decode_weight(float w)
{
	//return pow2(w);
	return exp(w);
	//return pow2(pow2(w));
}

#if defined(CALC_SCENE_INFO)

#if defined(FIRST)
Texture2D<float3>	src;
RWTexture2D<float2>	dst;
#else
Texture2D<float2>	src;
RWTexture2D<float2>	dst;
#endif

groupshared float shared_avg[32];
groupshared float shared_max[32];

[numthreads(32, 32, 1)]
void calc_scene_info(int2 dtid : SV_DispatchThreadID, int group_idx : SV_GroupIndex)
{
	int2 size;
	src.GetDimensions(size.x, size.y);

	float2 val = 0;
	if(all(dtid < size))
	{
#if defined(FIRST)
		val = luminance(src[dtid]);
		val.x = log(val.x + 1e-10f) / (size.x * size.y);
#else
		val = src[dtid];
#endif
	}

	val.x = WaveActiveSum(val.x);
	val.y = WaveActiveMax(val.y);

	if(WaveIsFirstLane())
	{
		shared_avg[group_idx / 32] = val.x;
		shared_max[group_idx / 32] = val.y;
	}
	GroupMemoryBarrierWithGroupSync();

	if(group_idx < 32)
	{
		val.x = WaveActiveSum(shared_avg[group_idx]);
		val.y = WaveActiveMax(shared_max[group_idx]);
		if(WaveIsFirstLane())
		{
			//TODO: ‡‰ž‚ÉŽžŠÔ‚ð‚©‚¯‚é
#if defined(LAST)
			val.x = exp(val.x);
			//dst[dtid / 32] = val;
			dst[dtid / 32] = lerp(val, dst[dtid / 32], blend_ratio);
#else
			dst[dtid / 32] = val;
#endif
		}
	}
}

#elif defined(GLOBAL_TONEMAPPING)

Texture2D<float2>	L_avg_max;
RWTexture2D<float3>	src_dst;

[numthreads(16, 16, 1)]
void global_tonemapping(int2 dtid : SV_DispatchThreadID)
{
	int2 size;
	src_dst.GetDimensions(size.x, size.y);
	if(all(dtid < size))
		src_dst[dtid] = tonemapping(src_dst[dtid], base_key, L_avg_max[int2(0,0)]);
}

#elif defined(TONEMAPPING_AND_WEIGHTING)

#define KERNEL_SIZE				(3)
#define BLOCK_DIM				(16)
#define SHARED_DIM				(BLOCK_DIM + (KERNEL_SIZE / 2) * 2)
#define WELL_ESPOSEDNESS_SIGMA2	(0.04f)

Texture2D<float>			L_avg_max;
Texture2D<float3>			src;
RWTexture2DArray<float4>	dst;

groupshared float	shared_r[SHARED_DIM][SHARED_DIM];
groupshared float	shared_g[SHARED_DIM][SHARED_DIM];
groupshared float	shared_b[SHARED_DIM][SHARED_DIM];

float3 load_rgb(int x, int y)
{
	return float3(shared_r[y][x], shared_g[y][x], shared_b[y][x]);
}

[numthreads(16, 16, 1)]
void tonemapping_and_weighting(int3 dtid : SV_DispatchThreadID, int2 gid : SV_GroupID, int2 gtid : SV_GroupThreadID, int group_idx : SV_GroupIndex)
{
	int2 size;
	src.GetDimensions(size.x, size.y);

	int base_x = BLOCK_DIM * gid.x;
	int base_y = BLOCK_DIM * gid.y;
	float key = base_key * pow(2.0f, dtid.z - N / 2);
	for(int idx = group_idx; idx < SHARED_DIM * SHARED_DIM; idx += BLOCK_DIM * BLOCK_DIM)
	{
		int local_y = idx / SHARED_DIM;
		int local_x = idx - SHARED_DIM * local_y;
		int x = clamp(base_x + local_x, 0, size.x - 1);
		int y = clamp(base_y + local_y, 0, size.y - 1);

		float3 rgb = src[int2(x, y)];
		rgb = tonemapping(rgb, key, L_avg_max[int2(0,0)]);
		shared_r[local_y][local_x] = rgb[0];
		shared_g[local_y][local_x] = rgb[1];
		shared_b[local_y][local_x] = rgb[2];
	}
	GroupMemoryBarrierWithGroupSync();

	if((dtid.x >= size.x) || (dtid.y >= size.y))
		return;

	uint local_x = gtid.x + (KERNEL_SIZE / 2);
	uint local_y = gtid.y + (KERNEL_SIZE / 2);
	float3 center = load_rgb(local_x, local_y);
	float S = sqrt(average(pow2(center - average(center))));
	float C = abs(average(
		load_rgb(local_x - 1, local_y) + 
		load_rgb(local_x + 1, local_y) + 
		load_rgb(local_x, local_y - 1) + 
		load_rgb(local_x, local_y + 1) - 4 * center));
	float E = exp(-sum(pow2(center - 0.5f)) / (2 * WELL_ESPOSEDNESS_SIGMA2));
	float eps = 1e-10f;
	float weight = max(pow(S, ws), eps) * max(pow(C, wc), eps) * max(pow(E, we), eps);
	dst[dtid] = float4(center, encode_weight(weight));
}

#elif defined(DOWNWARD)

Texture2DArray<float4>		src;
RWTexture2DArray<float4>	dst;

#define KERNEL_SIZE 5
#define BLOCK_DIM 16
#define SHARED_DIM (BLOCK_DIM + (KERNEL_SIZE / 2) * 2)
static const float kernel_weight[] = { 0.05f, 0.25f, 0.4f, 0.25f, 0.05f };

groupshared float	shared_r[SHARED_DIM][SHARED_DIM];
groupshared float	shared_g[SHARED_DIM][SHARED_DIM];
groupshared float	shared_b[SHARED_DIM][SHARED_DIM];
groupshared float	shared_w[SHARED_DIM][SHARED_DIM];

float4 load_rgbw(int x, int y)
{
	return float4(shared_r[y][x], shared_g[y][x], shared_b[y][x], shared_w[y][x]);
}

[numthreads(16, 16, 1)]
void downward(int3 gid : SV_GroupID, int2 gtid : SV_GroupThreadID, int group_idx : SV_GroupIndex)
{
	int3 src_size, dst_size;
	src.GetDimensions(src_size.x, src_size.y, src_size.z);
	dst.GetDimensions(dst_size.x, dst_size.y, dst_size.z);

	int base_x = BLOCK_DIM * gid.x - (KERNEL_SIZE / 2);
	int base_y = BLOCK_DIM * gid.y - (KERNEL_SIZE / 2);
	for(int idx = group_idx; idx < SHARED_DIM * SHARED_DIM; idx += BLOCK_DIM * BLOCK_DIM)
	{
		int local_y = idx / SHARED_DIM;
		int local_x = idx - SHARED_DIM * local_y;
		int x = clamp(base_x + local_x, 0, src_size.x - 1);
		int y = clamp(base_y + local_y, 0, src_size.y - 1);

		float4 rgbw = src[int3(x, y, gid.z)];
		shared_r[local_y][local_x] = rgbw[0];
		shared_g[local_y][local_x] = rgbw[1];
		shared_b[local_y][local_x] = rgbw[2];
		shared_w[local_y][local_x] = rgbw[3];
	}
	GroupMemoryBarrierWithGroupSync();

	//‰¡•ûŒü(ã‰º‚Ì(KERNEL_SIZE/2)‚Ì•”•ª‚àŒvŽZ)
	{
		int local_y = group_idx / BLOCK_DIM;
		int local_x = group_idx - BLOCK_DIM * local_y;
		for(int n = 0; n < 2; n++, local_y += SHARED_DIM / 2)
		{
			float4 sum_rgbw = 0;
			if(group_idx < BLOCK_DIM * SHARED_DIM / 2) //s‚Ì“r’†‚ÅŒvŽZ‚ªI‚í‚é‚Æƒ_ƒ
			{
				for(int i = 0; i < 5; i++)
					sum_rgbw += load_rgbw(local_x + i, local_y) * kernel_weight[i];
			}
			GroupMemoryBarrierWithGroupSync();

			if(group_idx < BLOCK_DIM * SHARED_DIM / 2)
			{
				shared_r[local_y][local_x + (KERNEL_SIZE / 2)] = sum_rgbw[0];
				shared_g[local_y][local_x + (KERNEL_SIZE / 2)] = sum_rgbw[1];
				shared_b[local_y][local_x + (KERNEL_SIZE / 2)] = sum_rgbw[2];
				shared_w[local_y][local_x + (KERNEL_SIZE / 2)] = sum_rgbw[3];
			}
			GroupMemoryBarrierWithGroupSync();
		}
	}

	//c•ûŒü
	{
		float4 sum_rgbw = 0;
		for(int i = 0; i < 5; i++)
			sum_rgbw += load_rgbw(gtid.x + (KERNEL_SIZE / 2), gtid.y + i) * kernel_weight[i];
		GroupMemoryBarrierWithGroupSync();

		shared_r[gtid.y + (KERNEL_SIZE / 2)][gtid.x + (KERNEL_SIZE / 2)] = sum_rgbw[0];
		shared_g[gtid.y + (KERNEL_SIZE / 2)][gtid.x + (KERNEL_SIZE / 2)] = sum_rgbw[1];
		shared_b[gtid.y + (KERNEL_SIZE / 2)][gtid.x + (KERNEL_SIZE / 2)] = sum_rgbw[2];
		shared_w[gtid.y + (KERNEL_SIZE / 2)][gtid.x + (KERNEL_SIZE / 2)] = sum_rgbw[3];
		GroupMemoryBarrierWithGroupSync();
	}

	//ƒXƒgƒA
	if(group_idx < BLOCK_DIM * BLOCK_DIM / 4)
	{
		int local_y = group_idx / (BLOCK_DIM / 2);
		int local_x = group_idx - (BLOCK_DIM / 2) * local_y;
		int x = (BLOCK_DIM / 2) * gid.x + local_x;
		int y = (BLOCK_DIM / 2) * gid.y + local_y;
		if((x < dst_size.x) && (y < dst_size.y))
			dst[int3(x, y, gid.z)] = load_rgbw(2 * local_x + (KERNEL_SIZE / 2), 2 * local_y + (KERNEL_SIZE / 2));
	}
}

#elif defined(UPWARD) || defined(COLLAPSE)

#if defined(UPWARD)
Texture2DArray<float4>		up_src;
Texture2DArray<float4>		src;
RWTexture2DArray<float4>	dst;
#elif defined(COLLAPSE)
Texture2D<float4>			up_src;
RWTexture2D<float4>			dst;
#endif

#define KERNEL_SIZE 5
#define BLOCK_DIM 16
#define SHARED_DIM (BLOCK_DIM + (KERNEL_SIZE / 2) * 2)
static const float kernel_weight[] = { 0.05f, 0.25f, 0.4f, 0.25f, 0.05f };

groupshared float shared_r[SHARED_DIM][SHARED_DIM];
groupshared float shared_g[SHARED_DIM][SHARED_DIM];
groupshared float shared_b[SHARED_DIM][SHARED_DIM];

float3 load_rgb(int x, int y)
{
	return float3(shared_r[y][x], shared_g[y][x], shared_b[y][x]);
}

[numthreads(16, 16, 1)]
void upward(int3 gid : SV_GroupID, int2 gtid : SV_GroupThreadID, int group_idx : SV_GroupIndex)
{
	int3 src_size;
#if defined(UPWARD)
	up_src.GetDimensions(src_size.x, src_size.y, src_size.z);
#elif defined(COLLAPSE)
	up_src.GetDimensions(src_size.x, src_size.y);
#endif

	int base_x = (BLOCK_DIM * gid.x - (KERNEL_SIZE / 2)) / 2;
	int base_y = (BLOCK_DIM * gid.y - (KERNEL_SIZE / 2)) / 2;
	if(group_idx < SHARED_DIM * SHARED_DIM / 4)
	{
		int local_y = group_idx / (SHARED_DIM / 2);
		int local_x = group_idx - (SHARED_DIM / 2) * local_y;
		int x = clamp(base_x + local_x, 0, src_size.x - 1);
		int y = clamp(base_y + local_y, 0, src_size.y - 1);
		local_x *= 2;
		local_y *= 2;

#if defined(UPWARD)
		float3 rgb = up_src[int3(x, y, gid.z)].rgb;
#elif defined(COLLAPSE)
		float3 rgb = up_src[int2(x, y)].rgb;
#endif
		shared_r[local_y][local_x] = shared_r[local_y][local_x + 1] = shared_r[local_y + 1][local_x] = shared_r[local_y + 1][local_x + 1] = rgb[0];
		shared_g[local_y][local_x] = shared_g[local_y][local_x + 1] = shared_g[local_y + 1][local_x] = shared_g[local_y + 1][local_x + 1] = rgb[1];
		shared_b[local_y][local_x] = shared_b[local_y][local_x + 1] = shared_b[local_y + 1][local_x] = shared_b[local_y + 1][local_x + 1] = rgb[2];
	}
	GroupMemoryBarrierWithGroupSync();

	//‰¡•ûŒü(ã‰º‚Ì(KERNEL_SIZE/2)‚Ì•”•ª‚àŒvŽZ)
	{
		int local_y = group_idx / BLOCK_DIM;
		int local_x = group_idx - BLOCK_DIM * local_y;
		for(int n = 0; n < 2; n++, local_y += SHARED_DIM / 2)
		{
			float3 sum_rgb = 0;
			if(group_idx < BLOCK_DIM * SHARED_DIM / 2) //s‚Ì“r’†‚ÅŒvŽZ‚ªI‚í‚é‚Æƒ_ƒ
			{
				for(int i = 0; i < 5; i++)
					sum_rgb += load_rgb(local_x + i, local_y) * kernel_weight[i];
			}
			GroupMemoryBarrierWithGroupSync();

			if(group_idx < BLOCK_DIM * SHARED_DIM / 2)
			{
				shared_r[local_y][local_x + (KERNEL_SIZE / 2)] = sum_rgb[0];
				shared_g[local_y][local_x + (KERNEL_SIZE / 2)] = sum_rgb[1];
				shared_b[local_y][local_x + (KERNEL_SIZE / 2)] = sum_rgb[2];
			}
			GroupMemoryBarrierWithGroupSync();
		}
	}

	//c•ûŒü&ƒXƒgƒA
	{
		float3 sum_rgb = 0;
		for(int i = 0; i < 5; i++)
			sum_rgb += load_rgb(gtid.x + (KERNEL_SIZE / 2), gtid.y + i) * kernel_weight[i];
		GroupMemoryBarrierWithGroupSync();

		int3 dst_size;
#if defined(UPWARD)
		dst.GetDimensions(dst_size.x, dst_size.y, dst_size.z);
#elif defined(COLLAPSE)
		dst.GetDimensions(dst_size.x, dst_size.y);
#endif

		int x = BLOCK_DIM * gid.x + gtid.x;
		int y = BLOCK_DIM * gid.y + gtid.y;
		if((x < dst_size.x) && (y < dst_size.y))
		{
#if defined(UPWARD)
			float4 rgbw = src[int3(x, y, gid.z)];
			rgbw.rgb -= sum_rgb;
			dst[int3(x, y, gid.z)] = rgbw;
#elif defined(COLLAPSE)
			dst[int2(x, y)] += float4(sum_rgb, 0);
#endif
		}
	}
}

#elif defined(BLEND)

Texture2DArray<float4>	src;
RWTexture2D<float4>		dst;

[numthreads(16, 16, 1)]
void blend(int2 dtid : SV_DispatchThreadID)
{
	int2 size;
	dst.GetDimensions(size.x, size.y);

	if((dtid.x >= size.x) || (dtid.y >= size.y))
		return;

	float4 sum = 0;
	for(int i = 0; i < 5; i++)
	{
		if(i >= N){ break; }
		float4 col = src[int3(dtid, i)];
		col.w = decode_weight(col.w);
		sum.rgb += col.w * col.rgb;
		sum.w += col.w;
	}
	dst[dtid] = sum / sum.w;
}

#endif
#endif
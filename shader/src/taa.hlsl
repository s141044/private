
#ifndef TAA_HLSL
#define TAA_HLSL

#include"gbuffer.hlsl"
#include"static_sampler.hlsl"
#include"global_constant.hlsl"

cbuffer constant_buffer
{
	float	update_rate;
	float	sigma_scale;
	float	depth_threshold;
	float	reserved;
};

#define BORDER	2
#define SIZE	16

Texture2D<float>	depth;
Texture2D<float>	prev_depth;
Texture2D<float4>	velocity;
Texture2D<float3>	src;
Texture2D<float3>	history;
RWTexture2D<float3>	dst;
groupshared float3	shared_col[SIZE + 2 * BORDER][SIZE + 2 * BORDER];

[numthreads(SIZE, SIZE, 1)]
void taa(int2 dtid : SV_DispatchThreadID, int2 gtid : SV_GroupThreadID, int2 gid : SV_GroupID, int group_index : SV_GroupIndex)
{
	if(any(dtid >= screen_size))
		return;

	int2 base = SIZE * gid - BORDER;
	for(int i = group_index; i < (SIZE + 2 * BORDER) * (SIZE + 2 * BORDER); i += SIZE * SIZE)
	{
		int2 offset;
		offset.y = i / (SIZE + 2 * BORDER);
		offset.x = i - (SIZE + 2 * BORDER) * offset.y;
		shared_col[offset.y][offset.x] = src[clamp(base + offset, 0, screen_size)];
	}
	GroupMemoryBarrierWithGroupSync();

	float3 col = src[dtid];

	float3 prev_pos;
	prev_pos.xy = dtid + 0.5f;
	prev_pos.z = depth[dtid];
	if(prev_pos.z <= 0)
	{
		dst[dtid] = col;
		return;
	}

	prev_pos += decode_velocity(velocity[dtid]);
	if(any(prev_pos.xy < 0) || any(prev_pos.xy >= screen_size))
	{
		dst[dtid] = col;
		return;
	}

	float2 left_top = floor(prev_pos.xy - 0.5f);
	float2 bilinear_weight = prev_pos.xy - left_top - 0.5f;

	float4 weight;
	weight[0] = (1 - bilinear_weight.x) * (1 - bilinear_weight.y);
	weight[1] = bilinear_weight.x * (1 - bilinear_weight.y);
	weight[2] = (1 - bilinear_weight.x) * bilinear_weight.y;
	weight[3] = bilinear_weight.x * bilinear_weight.y;

	float4 prev_z;
	prev_z = prev_depth.GatherRed(bilinear_clamp, prev_pos.xy * inv_screen_size, 0).wzxy;

	float4 history_col = 0;
	for(int i = 0; i < 4; i++)
	{
		int2 offset;
		offset.x = (i >> 0) & 1;
		offset.y = (i >> 1) & 1;
		history_col += weight[i] * float4(history[left_top + offset], 1);
	}
	if(history_col.w > 0)
		history_col.xyz /= history_col.w;
	else
		history_col.xyz = col;

	float3 sum1 = 0;
	float3 sum2 = 0;
	for(int y = 0; y < 2 * BORDER + 1; y++)
	{
		for(int x = 0; x < 2 * BORDER + 1; x++)
		{
			float3 col = shared_col[gtid.y + y][gtid.x + x];
			sum1 += col;
			sum2 += col * col;
		}
	}
	sum1 /= (2 * BORDER + 1) * (2 * BORDER + 1);
	sum2 /= (2 * BORDER + 1) * (2 * BORDER + 1);

	float3 sigma = sqrt(max(sum2 - sum1 * sum1, 0)) * sigma_scale;
	float3 min_col = sum1 - sigma;
	float3 max_col = sum1 + sigma;

	//if(any(isnan(col)))
	//{
	//	for(int i = 0; i < 10; i ++)
	//		for(int j = 0; j < 10; j++)
	//			dst[dtid + int2(i,j)] = float3(10,0,0);
	//	return;
	//}

	history_col.xyz = clamp(history_col.xyz, min_col, max_col);
	dst[dtid] = lerp(history_col.xyz, col, update_rate);
	//dst[dtid] = lerp(history_col.xyz, col, 1);
}

#endif
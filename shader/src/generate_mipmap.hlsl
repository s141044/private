
#ifndef GENERATE_MIPMAP_HLSL
#define GENERATE_MIPMAP_HLSL

#include"packing.hlsl"
#include"root_constant.hlsl"
#include"static_sampler.hlsl"

#if defined(FORMAT_FLOAT4)
#define TYPE				float4
#define CONVERT(val)		val
#elif defined(FORMAT_FLOAT)
#define TYPE				float
#define CONVERT(val)		val.x
#elif defined(FORMAT_R9G9B9E5)
#define FORMAT_UINT
#define TYPE				uint
#define CONVERT(val)		f32x3_to_r9g9b9e5(val.rgb)
#define CONVERT_UINT(val)	r9g9b9e5_to_f32x3(val).rgbb
#endif

#if defined(ARRAY)
Texture2DArray<TYPE>			src;
RWTexture2DArray<TYPE>			dst1;
RWTexture2DArray<TYPE>			dst2;
RWTexture2DArray<TYPE>			dst3;
RWTexture2DArray<TYPE>			dst4;
RWTexture2DArray<TYPE>			dst5;
RWTexture2DArray<TYPE>			dst6;
#define LOAD(s, uvw)			src.SampleLevel(s, uvw, 0)
#define GATHER(uvw)				src.GatherRed(bilinear_clamp, uvw, 0)
#define STORE(dst, pos, val)	dst[pos] = val
#else
Texture2D<TYPE>					src;
RWTexture2D<TYPE>				dst1;
RWTexture2D<TYPE>				dst2;
RWTexture2D<TYPE>				dst3;
RWTexture2D<TYPE>				dst4;
RWTexture2D<TYPE>				dst5;
RWTexture2D<TYPE>				dst6;
#define LOAD(s, uvw)			src.SampleLevel(s, uvw.xy, 0)
#define GATHER(uvw)				src.GatherRed(bilinear_clamp, uvw.xy, 0)
#define STORE(dst, pos, val)	dst[pos.xy] = val
#endif

#if defined(REDUCE_ADD)
#define REDUCE(a, b, c, d)		((a) + (b) + (c) + (d))
#else
#define REDUCE_AVG
#define REDUCE(a, b, c, d)		((a) + (b) + (c) + (d)) / 4
#endif

groupshared float shared_r[2][32][32];
groupshared float shared_g[2][32][32];
groupshared float shared_b[2][32][32];
groupshared float shared_a[2][32][32];

#define NUM_THREADS (1 << (GEN_COUNT - 1))
[numthreads(NUM_THREADS, NUM_THREADS, 1)]
void generate_mipmap(int3 dtid : SV_DispatchThreadID, int2 gtid : SV_GroupThreadID, int2 gid : SV_GroupID, int group_idx : SV_GroupIndex)
{
	float3 uvw;
	float inv_res = asfloat(root_constant);
	uvw.xy = (2 * dtid.xy + 1) * inv_res;
	uvw.z = dtid.z;

	float4 val;
#if defined(REDUCE_AVG) && !defined(FORMAT_UINT)
	val = LOAD(bilienar_clamp, uvw);
#else

#if defined(FORMAT_UINT)
	uint4 vals = GATHER(uvw);
	val = REDUCE(CONVERT_UINT(vals[0]), CONVERT_UINT(vals[1]), CONVERT_UINT(vals[2]), CONVERT_UINT(vals[3]));
#elif defined(FORMAT_FLOAT)
	float4 vals = GATHER(uvw);
	val = REDUCE(vals[0], vals[1], vals[2], vals[3]);
#else
	uvw.xy = (2 * dtid.xy + 0.5f) * inv_res;
	val = REDUCE(
		LOAD(point_clamp, float3(uv.xy + float2(0, 0) * inv_res, uvw.z)), 
		LOAD(point_clamp, float3(uv.xy + float2(1, 0) * inv_res, uvw.z)), 
		LOAD(point_clamp, float3(uv.xy + float2(0, 1) * inv_res, uvw.z)), 
		LOAD(point_clamp, float3(uv.xy + float2(1, 1) * inv_res, uvw.z)));
#endif
#endif
	
	STORE(dst1, dtid, CONVERT(val));
	shared_r[0][gtid.y][gtid.x] = val.r;
	shared_g[0][gtid.y][gtid.x] = val.g;
	shared_b[0][gtid.y][gtid.x] = val.b;
	shared_a[0][gtid.y][gtid.x] = val.a;
	GroupMemoryBarrierWithGroupSync();

#if (GEN_COUNT > 1)
	if(group_idx < NUM_THREADS * NUM_THREADS / 4)
	{
		gtid.y = group_idx / (NUM_THREADS / 2);
		gtid.x = group_idx - (NUM_THREADS / 2) * gtid.y;
		dtid.xy = (NUM_THREADS / 2) * gid + gtid;

		val.r = REDUCE(shared_r[0][2 * gtid.y][2 * gtid.x], shared_r[0][2 * gtid.y][2 * gtid.x + 1], shared_r[0][2 * gtid.y + 1][2 * gtid.x], shared_r[0][2 * gtid.y + 1][2 * gtid.x + 1]);
		val.g = REDUCE(shared_g[0][2 * gtid.y][2 * gtid.x], shared_g[0][2 * gtid.y][2 * gtid.x + 1], shared_g[0][2 * gtid.y + 1][2 * gtid.x], shared_g[0][2 * gtid.y + 1][2 * gtid.x + 1]);
		val.b = REDUCE(shared_b[0][2 * gtid.y][2 * gtid.x], shared_b[0][2 * gtid.y][2 * gtid.x + 1], shared_b[0][2 * gtid.y + 1][2 * gtid.x], shared_b[0][2 * gtid.y + 1][2 * gtid.x + 1]);
		val.a = REDUCE(shared_a[0][2 * gtid.y][2 * gtid.x], shared_a[0][2 * gtid.y][2 * gtid.x + 1], shared_a[0][2 * gtid.y + 1][2 * gtid.x], shared_a[0][2 * gtid.y + 1][2 * gtid.x + 1]);

		STORE(dst2, dtid, CONVERT(val));
		shared_r[1][gtid.y][gtid.x] = val.r;
		shared_g[1][gtid.y][gtid.x] = val.g;
		shared_b[1][gtid.y][gtid.x] = val.b;
		shared_a[1][gtid.y][gtid.x] = val.a;
	}
	GroupMemoryBarrierWithGroupSync();
#endif
#if (GEN_COUNT > 2)
	if(group_idx < NUM_THREADS * NUM_THREADS / 16)
	{
		gtid.y = group_idx / (NUM_THREADS / 4);
		gtid.x = group_idx - (NUM_THREADS / 4) * gtid.y;
		dtid.xy = (NUM_THREADS / 4) * gid + gtid;

		val.r = REDUCE(shared_r[1][2 * gtid.y][2 * gtid.x], shared_r[1][2 * gtid.y][2 * gtid.x + 1], shared_r[1][2 * gtid.y + 1][2 * gtid.x], shared_r[1][2 * gtid.y + 1][2 * gtid.x + 1]);
		val.g = REDUCE(shared_g[1][2 * gtid.y][2 * gtid.x], shared_g[1][2 * gtid.y][2 * gtid.x + 1], shared_g[1][2 * gtid.y + 1][2 * gtid.x], shared_g[1][2 * gtid.y + 1][2 * gtid.x + 1]);
		val.b = REDUCE(shared_b[1][2 * gtid.y][2 * gtid.x], shared_b[1][2 * gtid.y][2 * gtid.x + 1], shared_b[1][2 * gtid.y + 1][2 * gtid.x], shared_b[1][2 * gtid.y + 1][2 * gtid.x + 1]);
		val.a = REDUCE(shared_a[1][2 * gtid.y][2 * gtid.x], shared_a[1][2 * gtid.y][2 * gtid.x + 1], shared_a[1][2 * gtid.y + 1][2 * gtid.x], shared_a[1][2 * gtid.y + 1][2 * gtid.x + 1]);

		STORE(dst3, dtid, CONVERT(val));
		shared_r[0][gtid.y][gtid.x] = val.r;
		shared_g[0][gtid.y][gtid.x] = val.g;
		shared_b[0][gtid.y][gtid.x] = val.b;
		shared_a[0][gtid.y][gtid.x] = val.a;
	}
	GroupMemoryBarrierWithGroupSync();
#endif
#if (GEN_COUNT > 3)
	if(group_idx < NUM_THREADS * NUM_THREADS / 64)
	{
		gtid.y = group_idx / (NUM_THREADS / 8);
		gtid.x = group_idx - (NUM_THREADS / 8) * gtid.y;
		dtid.xy = (NUM_THREADS / 8) * gid + gtid;

		val.r = REDUCE(shared_r[0][2 * gtid.y][2 * gtid.x], shared_r[0][2 * gtid.y][2 * gtid.x + 1], shared_r[0][2 * gtid.y + 1][2 * gtid.x], shared_r[0][2 * gtid.y + 1][2 * gtid.x + 1]);
		val.g = REDUCE(shared_g[0][2 * gtid.y][2 * gtid.x], shared_g[0][2 * gtid.y][2 * gtid.x + 1], shared_g[0][2 * gtid.y + 1][2 * gtid.x], shared_g[0][2 * gtid.y + 1][2 * gtid.x + 1]);
		val.b = REDUCE(shared_b[0][2 * gtid.y][2 * gtid.x], shared_b[0][2 * gtid.y][2 * gtid.x + 1], shared_b[0][2 * gtid.y + 1][2 * gtid.x], shared_b[0][2 * gtid.y + 1][2 * gtid.x + 1]);
		val.a = REDUCE(shared_a[0][2 * gtid.y][2 * gtid.x], shared_a[0][2 * gtid.y][2 * gtid.x + 1], shared_a[0][2 * gtid.y + 1][2 * gtid.x], shared_a[0][2 * gtid.y + 1][2 * gtid.x + 1]);

		STORE(dst4, dtid, CONVERT(val));
		shared_r[1][gtid.y][gtid.x] = val.r;
		shared_g[1][gtid.y][gtid.x] = val.g;
		shared_b[1][gtid.y][gtid.x] = val.b;
		shared_a[1][gtid.y][gtid.x] = val.a;
	}
	GroupMemoryBarrierWithGroupSync();
#endif
#if (GEN_COUNT > 4)
	if(group_idx < NUM_THREADS * NUM_THREADS / 256)
	{
		gtid.y = group_idx / (NUM_THREADS / 16);
		gtid.x = group_idx - (NUM_THREADS / 16) * gtid.y;
		dtid.xy = (NUM_THREADS / 16) * gid + gtid;

		val.r = REDUCE(shared_r[1][2 * gtid.y][2 * gtid.x], shared_r[1][2 * gtid.y][2 * gtid.x + 1], shared_r[1][2 * gtid.y + 1][2 * gtid.x], shared_r[1][2 * gtid.y + 1][2 * gtid.x + 1]);
		val.g = REDUCE(shared_g[1][2 * gtid.y][2 * gtid.x], shared_g[1][2 * gtid.y][2 * gtid.x + 1], shared_g[1][2 * gtid.y + 1][2 * gtid.x], shared_g[1][2 * gtid.y + 1][2 * gtid.x + 1]);
		val.b = REDUCE(shared_b[1][2 * gtid.y][2 * gtid.x], shared_b[1][2 * gtid.y][2 * gtid.x + 1], shared_b[1][2 * gtid.y + 1][2 * gtid.x], shared_b[1][2 * gtid.y + 1][2 * gtid.x + 1]);
		val.a = REDUCE(shared_a[1][2 * gtid.y][2 * gtid.x], shared_a[1][2 * gtid.y][2 * gtid.x + 1], shared_a[1][2 * gtid.y + 1][2 * gtid.x], shared_a[1][2 * gtid.y + 1][2 * gtid.x + 1]);

		STORE(dst5, dtid, CONVERT(val));
		shared_r[0][gtid.y][gtid.x] = val.r;
		shared_g[0][gtid.y][gtid.x] = val.g;
		shared_b[0][gtid.y][gtid.x] = val.b;
		shared_a[0][gtid.y][gtid.x] = val.a;
	}
	GroupMemoryBarrierWithGroupSync();
#endif
#if (GEN_COUNT > 5)
	if(group_idx < NUM_THREADS * NUM_THREADS / 1024)
	{
		gtid.y = group_idx / (NUM_THREADS / 32);
		gtid.x = group_idx - (NUM_THREADS / 32) * gtid.y;
		dtid.xy = (NUM_THREADS / 32) * gid + gtid;

		val.r = REDUCE(shared_r[0][2 * gtid.y][2 * gtid.x], shared_r[0][2 * gtid.y][2 * gtid.x + 1], shared_r[0][2 * gtid.y + 1][2 * gtid.x], shared_r[0][2 * gtid.y + 1][2 * gtid.x + 1]);
		val.g = REDUCE(shared_g[0][2 * gtid.y][2 * gtid.x], shared_g[0][2 * gtid.y][2 * gtid.x + 1], shared_g[0][2 * gtid.y + 1][2 * gtid.x], shared_g[0][2 * gtid.y + 1][2 * gtid.x + 1]);
		val.b = REDUCE(shared_b[0][2 * gtid.y][2 * gtid.x], shared_b[0][2 * gtid.y][2 * gtid.x + 1], shared_b[0][2 * gtid.y + 1][2 * gtid.x], shared_b[0][2 * gtid.y + 1][2 * gtid.x + 1]);
		val.a = REDUCE(shared_a[0][2 * gtid.y][2 * gtid.x], shared_a[0][2 * gtid.y][2 * gtid.x + 1], shared_a[0][2 * gtid.y + 1][2 * gtid.x], shared_a[0][2 * gtid.y + 1][2 * gtid.x + 1]);

		STORE(dst6, dtid, CONVERT(val));
		shared_r[1][gtid.y][gtid.x] = val.r;
		shared_g[1][gtid.y][gtid.x] = val.g;
		shared_b[1][gtid.y][gtid.x] = val.b;
		shared_a[1][gtid.y][gtid.x] = val.a;
	}
	GroupMemoryBarrierWithGroupSync();
#endif
}

#endif

#ifndef REFLECTANCE_HLSL
#define REFLECTANCE_HLSL

#include"random.hlsl"
#include"bsdf/oren_nayer.hlsl"
#include"bsdf/microfacet.hlsl"

#if defined(CALC_DIFFUSE) || defined(CALC_SPECULAR) 

#if defined(CALC_DIFFUSE)
RWTexture2D<float>	reflectance_table;
groupshared	float	shared_sum[32];
#elif defined(CALC_SPECULAR)
RWTexture2D<float2>	reflectance_table;
groupshared	float2	shared_sum[32];
#endif

[numthreads(1024, 1, 1)]
void calc_reflectance(uint group_index : SV_GroupIndex, uint2 gid : SV_GroupID)
{
	float nv = (gid.x + 0.5f) / 32;
	float roughness = (gid.y + 0.5f) / 32;
	float3 wo = float3(sqrt(1 - nv * nv), 0, nv);

#if defined(CALC_DIFFUSE)
	float sum = 0;
#elif defined(CALC_SPECULAR)
	float2 sum = 0;
#endif
	uint N = 4;
	for(uint i = 0; i < N; i++)
	{
		float2 u = hammersley_sequence(N * group_index + i, N * 1024);

		float3 wi;
#if defined(CALC_DIFFUSE)
		if(!oren_nayer::sample_brdf(wo, wi, 1, roughness, u[0], u[1]))
			continue;
		sum += oren_nayer::calc_weight(wo, wi, 1, roughness).x;
#elif defined(CALC_SPECULAR)
		if(!microfacet::sample_brdf(wo, wi, 1, roughness, u[0], u[1]))
			continue;
		float val = microfacet::calc_weight(wo, wi, 1, roughness).x;
		sum += val * float2(1, pow5(1 - dot(normalize(wo + wi), wi)));
#endif
	}

	sum = WaveActiveSum(sum);
	if(WaveGetLaneIndex() == 0)
		shared_sum[group_index / 32] = sum;

	GroupMemoryBarrierWithGroupSync();

	if(group_index < 32)
	{
		sum = WaveActiveSum(shared_sum[group_index]);
		if(WaveGetLaneIndex() == 0)
			reflectance_table[gid] = sum / (N * 1024);
	}
}

#elif defined(CALC_MATTE)

Texture2D<float2>	reflectance_table;
RWTexture2D<float>	reflectance_table_matte;

[numthreads(32, 32, 1)]
void calc_reflectance(uint2 dtid : SV_DispatchThreadID)
{
	float u = (dtid.x + 0.5f) / 32;
	float val = reflectance_table[dtid].x;
	float avg = WaveActiveSum(val * u) / 32 * 2;
	reflectance_table_matte[dtid] = (1 - val) / (1 - avg);
}

#endif
#endif

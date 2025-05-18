
#ifndef MATERIAL_HAIR_HLSL
#define MATERIAL_HAIR_HLSL

#include"common.hlsl"
#include"../debug.hlsl"
#include"../packing.hlsl"
#include"../static_sampler.hlsl"

#include"../bsdf/sheen.hlsl"
#include"../bsdf/microfacet.hlsl"
#include"../bsdf/oren_nayer.hlsl"

#include"../debug.hlsl"

///////////////////////////////////////////////////////////////////////////////////////////////////

struct hair_material
{
};

///////////////////////////////////////////////////////////////////////////////////////////////////

hair_material load_hair_material(uint handle, float3 wo, float3 normal, float3 tangent, float3 binormal, float2 uv, float lod = 0, uint2 dtid = 0)
{
	hair_material mtl = (hair_material)0;
	return mtl;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

bool has_contribution(float nwi, hair_material mtl)
{
	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void calc_bsdf_pdf(float3 wo, float3 wi, float3 normal, hair_material mtl, out float3 diffuse, out float3 non_diffuse, out float pdf, uint2 dtid = 0)
{
	diffuse = 0;
	non_diffuse = 0;
	pdf = 0;
}

float4 calc_bsdf_pdf(float3 wo, float3 wi, float3 normal, hair_material mtl, uint2 dtid = 0)
{
	float pdf;
	float3 diffuse, non_diffuse;
	calc_bsdf_pdf(wo, wi, normal, mtl, diffuse, non_diffuse, pdf, dtid);
	return float4(diffuse + non_diffuse, pdf);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

bsdf_sample sample_bsdf(float3 wo, float3 normal, hair_material mtl, float u0, float u1, uint2 dtid = 0)
{
	bsdf_sample s = (bsdf_sample)0;
	return s;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

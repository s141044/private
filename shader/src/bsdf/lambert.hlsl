
#ifndef BSDF_LAMBERT_HLSL
#define BSDF_LAMBERT_HLSL

#include"../bsdf.hlsl"
#include"../sampling.hlsl"

///////////////////////////////////////////////////////////////////////////////////////////////////

float3 lambert_brdf(float3 wo, float3 wi, float3 normal, float3 kd)
{
	return kd * dot(normal, wi) * inv_PI;
}

float4 calc_lambert_brdf_pdf(float3 wo, float3 wi, float3 normal, float3 kd)
{
	float cos = dot(normal, wi);
	return float4(kd * cos * inv_PI, cos * inv_PI);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

bsdf_sample sample_lambert_brdf(float3 wo, float3 normal, float3 kd, float u0, float u1)
{
	float3 tangent, binormal;
	calc_orthonormal_basis(normal, tangent, binormal);

	bsdf_sample s;
	s.is_valid = true;
	s.weight = kd;
	s.wi = sample_cosine_hemisphere(u0, u1);
	s.pdf = sample_cosine_hemisphere_pdf(s.wi);
	s.wi = tangent * s.wi.x + binormal * s.wi.y + normal * s.wi.z;
	return s;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

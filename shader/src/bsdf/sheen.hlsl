
#ifndef BSDF_SHEEN_HLSL
#define BSDF_SHEEN_HLSL

#include"../bsdf.hlsl"

/*
* Practical Multiple-Scattering Sheen Using Linearly Transformed Cosines
*/

///////////////////////////////////////////////////////////////////////////////////////////////////

Texture2D<float2>	sheen_ltc_table;
Texture2D<float>	sheen_reflectance_table;

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace sheen{

///////////////////////////////////////////////////////////////////////////////////////////////////

void calc_orthonormal_basis(float3 wo, float3 normal, out float3 tangent, out float3 binormal)
{
	//ƒ[ƒJƒ‹‹óŠÔ‚Íwo‚ªXZ•½–Ê‚Éæ‚é‹óŠÔ
	binormal = normalize(cross(normal, wo));
	tangent = cross(binormal, normal);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void calc_brdf_pdf(float3 wo, float3 wi, float roughness, out float3 brdf, out float pdf, out float3 weight)
{
	float2 ab = sheen_ltc_table.SampleLevel(bilinear_clamp, float2(wo.z, roughness), 0);
	float a = ab[0];
	float b = ab[1];

	float3 w = float3(a * wi.x + b * wi.z, a * wi.y, wi.z);
	float inv_len = 1 / length(w);
	float J = pow2(a) * pow3(inv_len);
	w *= inv_len;

	weight = 1;
	pdf = w.z * inv_PI * J;
	brdf = weight * pdf;
}

float4 calc_brdf_pdf(float3 wo, float3 wi, float roughness)
{
	float3 weight;
	float4 brdf_pdf;
	calc_brdf_pdf(wo, wi, roughness, brdf_pdf.xyz, brdf_pdf.w, weight);
	return brdf_pdf;
}

float3 calc_weight(float3 wo, float3 wi, float roughness)
{
	float3 weight;
	float4 brdf_pdf;
	calc_brdf_pdf(wo, wi, roughness, brdf_pdf.xyz, brdf_pdf.w, weight);
	return weight;
}

void calc_brdf_pdf(float3 wo, float3 wi, float3 color, float roughness, out float3 brdf, out float pdf, out float3 weight)
{
	calc_brdf_pdf(wo, wi, roughness, brdf, pdf, weight);
	float R = sheen_reflectance_table.SampleLevel(bilinear_clamp, float2(wo.z, roughness), 0);
	weight *= R * color;
	brdf *= R * color;
}

float4 calc_brdf_pdf(float3 wo, float3 wi, float3 color, float roughness)
{
	float3 weight;
	float4 brdf_pdf;
	calc_brdf_pdf(wo, wi, color, roughness, brdf_pdf.xyz, brdf_pdf.w, weight);
	return brdf_pdf;
}

float3 calc_weight(float3 wo, float3 wi, float3 color, float roughness)
{
	float3 weight;
	float4 brdf_pdf;
	calc_brdf_pdf(wo, wi, color, roughness, brdf_pdf.xyz, brdf_pdf.w, weight);
	return weight;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

bool sample_brdf(float3 wo, out float3 wi, float roughness, float u0, float u1)
{
	float2 ab = sheen_ltc_table.SampleLevel(bilinear_clamp, float2(wo.z, roughness), 0);
	float inv_a = 1 / ab[0];
	float b = ab[1];

	float3 w = sample_cosine_hemisphere(u0, u1);
	wi = normalize(float3((w.x - b * w.z) * inv_a, w.y * inv_a, w.z));
	return (wi.z >= cosine_threshold);
}

bool sample_brdf(float3 wo, out float3 wi, float3 color, float roughness, float u0, float u1)
{
	return sample_brdf(wo, wi, roughness, u0, u1);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

float3 calc_reflectance(float3 wo, float3 color, float roughness)
{
	return color * sheen_reflectance_table.SampleLevel(bilinear_clamp, float2(wo.z, roughness), 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace sheen

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

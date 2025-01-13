
#ifndef BSDF_OREN_NAYER_HLSL
#define BSDF_OREN_NAYER_HLSL

#include"../bsdf.hlsl"

///////////////////////////////////////////////////////////////////////////////////////////////////

Texture2D<float> diffuse_reflectance_table;

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace oren_nayer{

///////////////////////////////////////////////////////////////////////////////////////////////////

void calc_brdf_pdf(float3 wo, float3 wi, float3 rho, float roughness, out float3 brdf, out float pdf, out float3 weight)
{
	float sigma2 = pow2(roughness);
	float A = 1 - 0.5f * sigma2 / (sigma2 + 0.33f);
	float B = 0.45f * sigma2 / (sigma2 + 0.09f);

	float sin_theta_o = sin_theta(wo);
	float sin_theta_i = sin_theta(wi);
	float cos_phi_o = cos_phi(wo, sin_theta_o);
	float cos_phi_i = cos_phi(wi, sin_theta_i);
	float sin_phi_o = sin_phi(wo, sin_theta_o);
	float sin_phi_i = sin_phi(wi, sin_theta_i);
	float tan_theta_o = sin_theta_o / cos_theta(wo);
	float tan_theta_i = sin_theta_i / cos_theta(wi);
	float sin_a = max(sin_theta_i, sin_theta_o);
	float tan_b = min(tan_theta_i, tan_theta_o);

	weight = rho * (A + (B * max(0, cos_phi_i * cos_phi_o + sin_phi_i * sin_phi_o) * sin_a * tan_b));
	brdf = weight * cos_theta(wi) * inv_PI;
	pdf = cos_theta(wi) * inv_PI;
}

float4 calc_brdf_pdf(float3 wo, float3 wi, float3 rho, float roughness)
{
	float3 weight;
	float4 brdf_pdf;
	calc_brdf_pdf(wo, wi, rho, roughness, brdf_pdf.xyz, brdf_pdf.w, weight);
	return brdf_pdf;
}

float3 calc_weight(float3 wo, float3 wi, float3 rho, float roughness)
{
	float3 weight;
	float4 brdf_pdf;
	calc_brdf_pdf(wo, wi, rho, roughness, brdf_pdf.xyz, brdf_pdf.w, weight);
	return weight;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

bool sample_brdf(float3 wo, out float3 wi, float3 rho, float roughness, float u0, float u1)
{
	wi = sample_cosine_hemisphere(u0, u1);
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

float3 calc_reflectance(float3 wo, float3 rho, float roughness)
{
	return rho * diffuse_reflectance_table.SampleLevel(bilinear_clamp, float2(wo.z, roughness), 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace oren_nayer

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif


#ifndef BSDF_MICROFACET_HLSL
#define BSDF_MICROFACET_HLSL

#include"../bsdf.hlsl"

///////////////////////////////////////////////////////////////////////////////////////////////////

Texture2D<float2>	specular_reflectance_table;
Texture2D<float>	matte_reflectance_table; //(1-specular_reflectance)/(1-average(specular_reflectance))

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace microfacet{

///////////////////////////////////////////////////////////////////////////////////////////////////

//Artist Friendly Metallic Fresnel
void calc_conductor_ior(float3 color0, float3 color90, out float3 n, out float3 k)
{
	float3 r = clamp(color0, 0, 0.99f);
	float3 g = color90;

	float3 sqrt_r = sqrt(r);
	float3 n_min = (1 - r) / (1 + r);
	float3 n_max = (1 + sqrt_r) / (1 - sqrt_r);
	n = lerp(n_max, n_min, g);

	float3 k2 = ((n + 1) * (n + 1) * r - (n - 1) * (n - 1)) / (1 - r);
	k = sqrt(max(k2, 0));
}

void calc_conductor_color(float3 n, float3 k, out float3 color0, out float3 color90)
{
	float3 r = ((n - 1) * (n - 1) + k * k) / ((n + 1) * (n + 1) + k * k);
	float3 sqrt_r = sqrt(r);
	float3 n_max = (1 + sqrt_r) / (1 - sqrt_r);
	float3 n_min = (1 - r) / (1 + r);
	float3 g = (n_max - n) / (n_max - n_min);
	color0 = r;
	color90 = g;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

float Lambda(float3 w, float2 alpha)
{
	w.xy *= alpha;
	return (-1 + length(w) / w.z) / 2;
}

float G1(float3 w, float2 alpha)
{
	return 1 / (1 + Lambda(w, alpha));
}

///////////////////////////////////////////////////////////////////////////////////////////////////

float G(float3 wo, float3 wi, float2 alpha)
{
	return 1 / (1 + Lambda(wo, alpha) + Lambda(wi, alpha));
}

///////////////////////////////////////////////////////////////////////////////////////////////////

float D(float3 w, float2 alpha)
{
	w.xy /= alpha;
	return 1 / (PI * alpha.x * alpha.y * pow2(dot(w, w)));
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void calc_brdf_pdf(float3 wo, float3 wi, float3 color0, float roughness, out float3 brdf, out float pdf, out float3 weight)
{
	float alpha = pow2(roughness);
	float3 h = normalize(wo + wi);
	float3 F = schlick_fresnel(dot(h, wi), color0);
	float D = microfacet::D(h, alpha);
	float G = microfacet::G(wo, wi, alpha);
	float G1 = microfacet::G1(wo, alpha);
	float common = D / (4 * wo.z);

	weight = F * G / G1;
	brdf = F * G * common;
	pdf = G1 * common;
}

float4 calc_brdf_pdf(float3 wo, float3 wi, float3 color0, float roughness)
{
	float3 weight;
	float4 brdf_pdf;
	calc_brdf_pdf(wo, wi, color0, roughness, brdf_pdf.xyz, brdf_pdf.w, weight);
	return brdf_pdf;
}

float3 calc_weight(float3 wo, float3 wi, float3 color0, float roughness)
{
	float3 weight;
	float4 brdf_pdf;
	calc_brdf_pdf(wo, wi, color0, roughness, brdf_pdf.xyz, brdf_pdf.w, weight);
	return weight;
}

void calc_matte_brdf_pdf(float3 wo, float3 wi, float3 reflectance, float roughness, out float3 brdf, out float pdf, out float3 weight)
{
	float val = matte_reflectance_table.SampleLevel(bilinear_clamp, float2(wi.z, roughness), 0);
	float common = wi.z * inv_PI;

	weight = reflectance * val;
	brdf = reflectance * val * common;
	pdf = common;
}

float4 calc_matte_brdf_pdf(float3 wo, float3 wi, float3 reflectance, float roughness)
{
	float3 weight;
	float4 brdf_pdf;
	calc_brdf_pdf(wo, wi, reflectance, roughness, brdf_pdf.xyz, brdf_pdf.w, weight);
	return brdf_pdf;
}

float3 calc_matte_weight(float3 wo, float3 wi, float3 reflectance, float roughness)
{
	float3 weight;
	float4 brdf_pdf;
	calc_brdf_pdf(wo, wi, reflectance, roughness, brdf_pdf.xyz, brdf_pdf.w, weight);
	return weight;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

bool sample_brdf(float3 wo, out float3 wi, float roughness, float u0, float u1)
{
	float alpha = pow2(roughness);
	float3 Vh = normalize(float3(alpha * wo.x, alpha * wo.y, wo.z));
	float lensq = Vh.x * Vh.x + Vh.y * Vh.y;
	float3 T1 = (lensq > 0) ? float3(-Vh.y, Vh.x, 0) * rsqrt(lensq) : float3(1,0,0);
	float3 T2 = cross(Vh, T1);
	float r = sqrt(u0);
	float phi = 2 * PI * u1;
	float t1 = r * cos(phi);
	float t2 = r * sin(phi);
	float s = 0.5 * (1.0 + Vh.z);
	t2 = (1.0 - s) * sqrt(1.0 - t1 * t1) + s * t2;
	float3 Nh = t1 * T1 + t2 * T2 + sqrt(max(0, 1 - t1 * t1 - t2 * t2)) * Vh;
	float3 Ne = normalize(float3(alpha * Nh.x, alpha * Nh.y, max(0, Nh.z)));
	wi = reflect(-wo, Ne);
	return (wi.z >= cosine_threshold);
}

bool sample_brdf(float3 wo, out float3 wi, float3 color0, float roughness, float u0, float u1)
{
	return sample_brdf(wo, wi, roughness, u0, u1);
}

bool sample_matte_brdf(float3 wo, out float3 wi, float3 reflectance, float roughness, float u0, float u1)
{
	wi = sample_cosine_hemisphere(u0, u1);
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

float3 calc_reflectance(float3 wo, float3 color0, float roughness)
{
	float2 vals = specular_reflectance_table.SampleLevel(bilinear_clamp, float2(wo.z, roughness), 0);
	return color0 * vals.x + (1 - color0) * vals.y;
}

float3 calc_matte_reflectance(float3 wo, float3 color0, float roughness)
{
	float val = specular_reflectance_table.SampleLevel(bilinear_clamp, float2(wo.z, roughness), 0).x;
	return (1 - val) * schlick_average_fresnel(color0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace microfacet

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif


#ifndef MATERIAL_SUBSURFACE_HLSL
#define MATERIAL_SUBSURFACE_HLSL

#include"../math.hlsl"

///////////////////////////////////////////////////////////////////////////////////////////////////

float3 calc_bssrdf(float r, float3 d)
{
	float3 e = exp(-r / (3 * d));
	return e * (1 + e * e) / (8 * PI * d * r);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

float sample_bssrdf(float d, float u)
{
	float c = 2.5715f;
	return d * ((2 - c) * u - 2) * log(1 - u);
}

float sample_bssrdf_pdf(float r, float d)
{
	return calc_bssrdf(r, d).x;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

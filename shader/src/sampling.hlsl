
#ifndef SAMPLING_HLSL
#define SAMPLING_HLSL

#include"math.hlsl"

///////////////////////////////////////////////////////////////////////////////////////////////////

float3 sample_cosine_hemisphere(float u0, float u1)
{
	float phi = (2 * PI) * u0;
	float ct = sqrt(1 - u1);
	float st = sqrt(u1);
	float cp = cos(phi);
	float sp = sin(phi);
	return float3(st * cp, st * sp, ct);
}

float sample_cosine_hemisphere_pdf(float3 w)
{
	return w.z * inv_PI;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

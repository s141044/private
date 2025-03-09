
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

float3 sample_uniform_hemisphere(float u0, float u1)
{
	float phi = (2 * PI) * u0;
	float ct = u1;
	float st = sqrt(1 - ct * ct);
	float cp = cos(phi);
	float sp = sin(phi);
	return float3(st * cp, st * sp, ct);
}

float sample_uniform_hemisphere_pdf(float3 w)
{
	return inv_2PI;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

float3 sample_uniform_cone(float cos_max, float u0, float u1)
{
	float phi = (2 * PI) * u0;
	float ct = 1 - u1 * (1 - cos_max);
	float st = sqrt(1 - ct * ct);
	float cp = cos(phi);
	float sp = sin(phi);
	return float3(st * cp, st * sp, ct);
}

float3 sample_uniform_cone(float cos_max, float3 t, float3 b, float3 n, float u0, float u1)
{
	float3 w = sample_uniform_cone(cos_max, u0, u1);
	return w.x * t + w.y * b + w.z * n;
}

float sample_uniform_cone_pdf(float3 w, float cos_max)
{
	return 1 / (2 * PI * (1 - cos_max));
}

float sample_uniform_cone_pdf(float3 w, float cos_max, float3 t, float3 b, float3 n)
{
	return sample_uniform_cone_pdf(float3(dot(w, t), dot(b, t), dot(n, t)), cos_max);
}

bool is_in_cone(float3 w, float cos_max)
{
	return (w.z > cos_max);
}

bool is_in_cone(float3 w, float cos_max, float3 t, float3 b, float3 n)
{
	return is_in_cone(float3(dot(w, t), dot(w, b), dot(w, n)), cos_max);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

float2 sample_uniform_disk(float R, float u0, float u1)
{
	u0 = 2 * u0 - 1;
	u1 = 2 * u1 - 1;

	if(u0 == 0 && u1 == 0)
		return 0;

	float r, theta;
	if(abs(u0) > abs(u1))
	{
		r = u0;
		theta = (PI / 4) * (u1 / u0);
	}
	else
	{
		r = u1;
		theta = (PI / 2) - (PI / 4) * (u0 / u1);
	}
	return R * float2(cos(theta), sin(theta));
}

float3 sample_uniform_disk(float R, float3 c, float3 t, float3 b, float u0, float u1)
{
	float2 p = sample_uniform_disk(R, u0, u1);
	return c + p.x * t + p.y * b;
}

float sample_uniform_disk_pdf(float R, float3 p)
{
	return PI * R * R;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

float3 sample_uniform_triangle(float u0, float u1)
{
	float sqrt_u0 = sqrt(u0);
	return float3(1 - sqrt_u0, sqrt_u0 * (1 - u1), sqrt_u0 * u1);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

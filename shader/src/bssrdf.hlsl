
#ifndef BSSRDF_HLSL
#define BSSRDF_HLSL

#include"math.hlsl"
#include"debug.hlsl"
#include"sampling.hlsl"

#define BSSRDF_R_MIN				1e-6
#define BSSRDF_U_MAX				0.95
#define BSSRDF_LOG_ONE_MINUS_U_MAX	-2.99573
#define BSSRDF_SAMPLE_PMF_AXIS		0.5, 0.25, 0.25

///////////////////////////////////////////////////////////////////////////////////////////////////

float3 bssrdf(float r, float3 d)
{
	r = max(r, BSSRDF_R_MIN);
	float log2_e = 1.44269504089;
	float3 e = exp2(-r / (3 * d)) * log2_e;
	return e * (1 + e * e) / (8 * PI * r * d);
}

float sample_bssrdf(float d, float u)
{
	float c = 2.5715;
	u *= BSSRDF_U_MAX;
	return d * ((2 - c) * u - 2) * log(1 - u);
}

float3 sample_bssrdf_pdf(float r, float3 d)
{
	return bssrdf(r, d);
}

float sample_bssrdf_pdf(float r, float d)
{
	return sample_bssrdf_pdf(r, d).r;
}

float3 sample_bssrdf_max_r(float3 d)
{
	float c = 2.5715;
	return d * ((2 - c) * BSSRDF_U_MAX - 2) * BSSRDF_LOG_ONE_MINUS_U_MAX;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

ray sample_bssrdf(float3 position, float3 normal, float3 d, float u0, float u1, uint2 dtid = 0)
{
	float pmf_ch = 1.0 / 3.0;
	float pmf_axis[] = { BSSRDF_SAMPLE_PMF_AXIS };

	int ch = u0 * 3;
	u0 = frac(u0 * 3);

	float3 axes[3];
	axes[0] = normal;
	calc_orthonormal_basis(axes[0], axes[1], axes[2]);

	uint v;
	if(u1 < pmf_axis[0])
	{
		v = 0;
		u1 /= pmf_axis[0];
	}
	else if((u1 -= pmf_axis[0]) < pmf_axis[1])
	{
		v = 1;
		u1 /= pmf_axis[1];
	}
	else
	{
		v = 2;
		u1 -= pmf_axis[1];
		u1 /= pmf_axis[2];
	}
	float3 axis = axes[0];
	axes[0] = axes[v];
	axes[v] = axis;

	float r = sample_bssrdf(d[ch], u0);
	float phi = 2 * PI * u1;

	float3 R = sample_bssrdf_max_r(d);
	float R_max = max(R[0], max(R[1], R[2]));

	ray ray;
	ray.origin = position + axes[0] * R_max + (axes[1] * cos(phi) + axes[2] * sin(phi)) * r;
	ray.direction = -axes[0];

	float sqrt_D = sqrt(R_max * R_max - r * r);
	ray.tmin = R_max - sqrt_D;
	ray.tmax = R_max + sqrt_D;
	return ray;
}

float sample_bssrdf_pdf(float3 position, float3 normal, float3 sample_position, float3 sample_normal, int hit_count, float3 d, uint2 dtid = 0)
{
	float3 axes[3];
	axes[0] = normal;
	calc_orthonormal_basis(axes[0], axes[1], axes[2]);
	
	float pmf_ch = 1.0 / 3.0;
	float pmf_axis[3] = { BSSRDF_SAMPLE_PMF_AXIS };
	float3 R = sample_bssrdf_max_r(d);

	float pdf = 0;
	for(int v = 0; v < 3; v++)
	{
		float3 diff = sample_position - position;
		diff -= axes[v] * dot(axes[v], diff);

		float r = length(diff);
		float3 pdfs = bssrdf(r, d) * pmf_axis[v] * pmf_ch * max(abs(dot(axes[v], sample_normal)), 1e-6);
		if(r < R[0]){ pdf += pdfs[0]; }
		if(r < R[1]){ pdf += pdfs[1]; }
		if(r < R[2]){ pdf += pdfs[2]; }
	}
	return pdf / hit_count;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

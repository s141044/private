
#ifndef VOLUME_HLSL
#define VOLUME_HLSL

#include"math.hlsl"
#include"sampling.hlsl"
#include"static_sampler.hlsl"

///////////////////////////////////////////////////////////////////////////////////////////////////

float transmittance(float sigma_t, float t)
{
	return exp(-sigma_t * t);
}

float sample_medium_distance(float sigma_t, float u)
{
	return -log(1 - u) / sigma_t;
}

float sample_medium_distance_pdf(float sigma_t, float t)
{
	return sigma_t * exp(-sigma_t * t);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

float hg_phase(float3 w1, float3 w2, float g)
{
	float tmp = 1 + g * g - 2 * g * dot(w1, w2);
	return (1 - g * g) / (4 * PI * tmp * sqrt(tmp));
}

float3 sample_hg_phase(float3 w, float g, float u0, float u1)
{
	float phi = 2 * PI * u0;
	float ct = (1 + g * g - pow2((1 - g * g) / (1 + g - 2 * g * u1))) / (2 * g);
	float st = sqrt(1 - ct * ct);
	return float3(st * cos(phi), st * sin(phi), ct);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

struct sggx_param
{
	float Sxx, Syy, Szz;
	float Sxy, Sxz, Syz;
};

float sggx_quadratic_form(float3 w, sggx_param param)
{
	return 
		+ (w.x * w.x * param.Sxx + w.y * w.y * param.Syy + w.z * w.z * param.Szz)
		+ (w.x * w.y * param.Sxy + w.x * w.z * param.Sxz + w.y * w.z * param.Syz) * 2;
}

float sggx_determinant(sggx_param param)
{
	return 
		+ param.Sxx * param.Syy * param.Szz
		+ param.Sxy * param.Sxz * param.Syz * 2
		- param.Sxx * param.Syz * param.Syz
		- param.Syy * param.Sxz * param.Sxz
		- param.Szz * param.Sxy * param.Sxy;
}

float sggx_ndf(float3 w, sggx_param param)
{
	sggx_param adj;
	adj.Sxx = param.Syy * param.Szz - param.Syz * param.Syz;
	adj.Syy = param.Sxx * param.Szz - param.Sxz * param.Sxz;
	adj.Szz = param.Sxx * param.Syy - param.Sxy * param.Sxy;
	adj.Sxy = param.Sxz * param.Syz - param.Sxy * param.Szz;
	adj.Sxz = param.Sxy * param.Syz - param.Sxz * param.Syy;
	adj.Syz = param.Sxy * param.Sxz - param.Syz * param.Sxx;

	float det = sggx_determinant(param);
	float den = sggx_quadratic_form(w, adj);
	return pow(abs(det), 1.5) / (PI * den * den);
}

float sggx_projected_area(float3 w, sggx_param param)
{
	return sqrt(max(0, sggx_quadratic_form(w, param)));
}

float sggx_specular_phase(float3 w1, float3 w2, sggx_param param)
{
	float3 wh = normalize(w1 + w2);
	return sggx_ndf(wh, param) / (4 * sggx_projected_area(w1, param));
}

float3 sample_sggx_vndf(float3 w, sggx_param param, float u0, float u1)
{
	float r = sqrt(u0);
	float phi = 2 * PI * u1;
	float u = r * cos(phi);
	float v = r * sin(phi);

	float3 wi = w, wj, wk;
	calc_orthonormal_basis(wi, wj, wk);

	sggx_param proj;
	proj.Sxx = wk.x * wk.x * param.Sxx + wk.y * wk.y * param.Syy + wk.z * wk.z * param.Szz + 2 * (wk.x * wk.y * param.Sxy + wk.x * wk.z * param.Sxz + wk.y * wk.z * param.Syz);
	proj.Syy = wj.x * wj.x * param.Sxx + wj.y * wj.y * param.Syy + wj.z * wj.z * param.Szz + 2 * (wj.x * wj.y * param.Sxy + wj.x * wj.z * param.Sxz + wj.y * wj.z * param.Syz);
	proj.Szz = wi.x * wi.x * param.Sxx + wi.y * wi.y * param.Syy + wi.z * wi.z * param.Szz + 2 * (wi.x * wi.y * param.Sxy + wi.x * wi.z * param.Sxz + wi.y * wi.z * param.Syz);
	proj.Sxy = wk.x * wj.x * param.Sxx + wk.y * wj.y * param.Syy + wk.z * wj.z * param.Szz + (wk.x * wj.y + wk.y * wj.x) * param.Sxy + (wk.x * wj.z + wk.z * wj.x) * param.Sxz + (wk.y * wj.z + wk.z * wj.y) * param.Syz;
	proj.Sxz = wk.x * wi.x * param.Sxx + wk.y * wi.y * param.Syy + wk.z * wi.z * param.Szz + (wk.x * wi.y + wk.y * wi.x) * param.Sxy + (wk.x * wi.z + wk.z * wi.x) * param.Sxz + (wk.y * wi.z + wk.z * wi.y) * param.Syz;
	proj.Syz = wj.x * wi.x * param.Sxx + wj.y * wi.y * param.Syy + wj.z * wi.z * param.Szz + (wj.x * wi.y + wj.y * wi.x) * param.Sxy + (wj.x * wi.z + wj.z * wi.x) * param.Sxz + (wj.y * wi.z + wj.z * wi.y) * param.Syz;

	float tmp0 = 1 / sqrt(proj.Szz);
	float tmp1 = sqrt(proj.Syy * proj.Szz - proj.Syz * proj.Syz);
	float3 Mk = float3(sqrt(abs(sggx_determinant(proj))) / tmp1, 0, 0);
	float3 Mj = float3(-(proj.Sxz * proj.Syz - proj.Sxy * proj.Szz) / tmp1, tmp1, 0) * tmp0;
	float3 Mi = float3(proj.Sxz, proj.Syz, proj.Szz) * tmp0;

	float3 wh = normalize(u * Mk + v * Mj + sqrt(1 - u * u - v * v) * Mi);
	return wh.x * wk + wh.y * wj + wh.z * wi;
}

float3 sample_sggx_specuar_phase(float3 w, sggx_param param, float u0, float u1)
{
	float3 wh = sample_sggx_vndf(w, param, u0, u1);
	return reflect(-w, wh);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif


#ifndef BSDF_HLSL
#define BSDF_HLSL

#include"math.hlsl"
#include"sampling.hlsl"
#include"static_sampler.hlsl"

/*/////////////////////////////////////////////////////////////////////////////////////////////////

wo,wiが補間法線の下半球でも，複数あるシェーディング法線のどれかでは上半球になる可能性があるが
この救済を許容すると早期returnができないようになる...
(GPUなのでどれかのスレッドが生存してるなら早期returnの恩恵は大きくないかもしれないが...)

BRDF,BTDFの計算の判断は補間法線の上半球か下半球かで決める

サンプリングで下半球の方向が得られたとき，それを有効なサンプルとみなすか？
そのBSDFでは値は0だが，他のBSDFで値があるかもしれない.
有効と見なさいない方針でいく．

/////////////////////////////////////////////////////////////////////////////////////////////////*/

static const uint material_type_standard	= 0;
static const uint material_type_transparent	= 1;

///////////////////////////////////////////////////////////////////////////////////////////////////

static const float cosine_threshold = 1e-3f;

///////////////////////////////////////////////////////////////////////////////////////////////////

struct bsdf_sample
{
	float3	wi;
	float3	weight;
	float	pdf;
	bool	is_valid;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

float cos_theta(float3 w)
{
	return w.z;
}

float sin_theta(float3 w)
{
	return sqrt(1 - w.z * w.z);
}

float cos_phi(float3 w, float sin_theta)
{
	return (sin_theta == 0) ? 0 : clamp(w.x / sin_theta, -1, 1);
}

float sin_phi(float3 w, float sin_theta)
{
	return (sin_theta == 0) ? 0 : clamp(w.y / sin_theta, -1, 1);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//eta = eta2 / eta1
float fresnel_dielectric(float cos, float eta)
{
	float g = eta * eta - 1 + cos * cos;
	if(g > 0)
	{
		g = sqrt(g);
		return pow2(g - cos) / pow2(g + cos) * (1 + pow2(cos * (g + cos) - 1) / pow2(cos * (g - cos) + 1)) / 2;
	}
	//全反射
	else{ return 1; }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//eta = n + i*k
float3 fresnel_conductor(float ct1, float3 nb, float3 kb, float3 na, out float3 phi_s, out float3 phi_p)
{
	float st2 = 1 - ct1 * ct1;
	float3 U = nb * nb - kb * kb - na * na * st2;
	float3 V = sqrt(U * U + 4 * nb * nb * kb * kb);
	float3 ub = sqrt((V + U) / 2);
	float3 vb = sqrt((V - U) / 2);

	phi_s = atan2(2 * vb * na * ct1, ub * ub + vb * vb - pow2(na * ct1));
	phi_p = atan2(2 * na * ct1 * (2 * nb * kb * ub - (nb * nb - kb * kb) * vb), pow2((nb * nb + kb * kb) * ct1) - na * na * (ub * ub + vb * vb));

	float3 Rs = 
		(pow2(na * ct1 - ub) + vb * vb) / 
		(pow2(na * ct1 + ub) + vb * vb);
	float3 Rp = 
		(pow2((nb * nb - kb * kb) * ct1 - na * ub) + pow2(2 * nb * kb * ct1 - na * vb)) /
		(pow2((nb * nb - kb * kb) * ct1 + na * ub) + pow2(2 * nb * kb * ct1 + na * vb));
	return (Rs + Rp) / 2;
}

float3 fresnel_conductor(float ct, float3 nb, float3 kb, float3 na = 1)
{
	float3 phi_s, phi_p;
	return fresnel_conductor(ct, nb, kb, na, phi_s, phi_p);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

float3 schlick_fresnel(float cos, float3 F0)
{
	return F0 + (1 - F0) * pow5(1 - cos);
}

float3 schlick_average_fresnel(float3 F0)
{
	//F*cos/piの積分
	return (20 * F0 + 1) / (42 * PI);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

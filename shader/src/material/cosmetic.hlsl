
#ifndef MATERIAL_COSMETIC_HLSL
#define MATERIAL_COSMETIC_HLSL

#include"common.hlsl"
#include"../debug.hlsl"
#include"../packing.hlsl"
#include"../intersection.hlsl"
#include"../static_sampler.hlsl"

#include"../bsdf/microfacet.hlsl"

#include"../debug.hlsl"

#define COMPRESS_MATERIAL

/*/////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////*/

#if defined(COMPRESS_MATERIAL)

struct cosmetic_material
{
	uint	base_normal;
	uint	coat_normal;
	uint	diffuse_color;
	uint	specular_color0;
	uint	subsurface_radius;

	uint	diffuser_albedo;
	uint	platelet_albedo;

	uint	data0; //coat_roughness, specular_roughness, platelet_roughness
	uint	data1; //coat_scale, specular_scale, -
	uint	data2; //subsurface, diffuser_cd, -
	uint	data3; //diffuser_g1, diffuser_g2, diffuser_wg
	uint	data4; //coat_eta, specular_eta
	uint	data5; //thickness, -
};

float3	get_base_normal(cosmetic_material mtl){ return oct_to_f32x3(u16x2_unorm_to_f32x2(mtl.base_normal) * 2 - 1); }
float3	get_coat_normal(cosmetic_material mtl){ return oct_to_f32x3(u16x2_unorm_to_f32x2(mtl.coat_normal) * 2 - 1); }
float	get_coat_scale(cosmetic_material mtl){ return r10g10b10a2_to_f32x4(mtl.data1).x; }
float	get_coat_roughness(cosmetic_material mtl){ return r10g10b10a2_to_f32x4(mtl.data0).x; }
float	get_specular_scale(cosmetic_material mtl){ return r10g10b10a2_to_f32x4(mtl.data1).y; }
float	get_specular_roughness(cosmetic_material mtl){ return r10g10b10a2_to_f32x4(mtl.data0).y; }
float3	get_diffuse_color(cosmetic_material mtl){ return r10g10b10a2_to_f32x4(mtl.diffuse_color).xyz; }
float	get_subsurface(cosmetic_material mtl){ return r10g10b10a2_to_f32x4(mtl.data2).x; }
float3	get_subsurface_radius(cosmetic_material mtl){ return r9g9b9e5_to_f32x3(mtl.subsurface_radius); }

float	get_coat_eta(cosmetic_material mtl){ return f16_to_f32(mtl.data4); }
float	get_specular_eta(cosmetic_material mtl){ return f16_to_f32(mtl.data4 >> 16); }
float	get_thickness(cosmetic_material mtl){ return asfloat(mtl.data5); }
float3	get_diffuser_albedo(cosmetic_material mtl){ return r10g10b10a2_to_f32x4(mtl.diffuser_albedo).xyz; }
float	get_diffuser_cd(cosmetic_material mtl){ return r10g10b10a2_to_f32x4(mtl.data2).y; }
float	get_diffuser_g1(cosmetic_material mtl){ return r10g10b10a2_to_f32x4(mtl.data3).x; }
float	get_diffuser_g2(cosmetic_material mtl){ return r10g10b10a2_to_f32x4(mtl.data3).y; }
float	get_diffuser_wg(cosmetic_material mtl){ return r10g10b10a2_to_f32x4(mtl.data3).z; }
float3	get_platelet_albedo(cosmetic_material mtl){ return r10g10b10a2_to_f32x4(mtl.platelet_albedo).xyz; }
float	get_platelet_roughness(cosmetic_material mtl){ return r10g10b10a2_to_f32x4(mtl.data0).z; }

#else

#endif

///////////////////////////////////////////////////////////////////////////////////////////////////

struct cosmetic_material_host
{
	//0
	uint	normal_map;
	uint	coat_scale_map;
	uint	coat_color0_map;
	uint	coat_roughness_map;
	//1
	uint	diffuse_color_map;
	uint	specular_scale_map;
	uint	specular_color0_map;
	uint	specular_roughness_map;
	//2
	uint	subsurface_map;
	uint	thickness_map;
	uint	diffuser_ratio_map;
	uint	diffuser_albedo_map;
	//3
	uint	diffuser_anisotropy1_map;
	uint	diffuser_anisotropy2_map;
	uint	diffuser_lobe_weight_map;
	uint	platelet_albedo_map;
	//4
	uint	platelet_roughness_map;
	uint	coat_color0;
	uint	diffuse_color;
	uint	specular_color0;
	//5
	uint	subsurface_radius; //r9g9b9e5
	uint	thickness; //offset,scale
	uint	diffuser_albedo;
	uint	platelet_albedo;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

cosmetic_material load_cosmetic_material(intersection isect, float3 wo, float lod = 0, uint2 dtid = 0)
{
	uint handle = isect.material_handle;
	float3 position = isect.position;
	float3 normal = isect.normal;
	float3 tangent = isect.tangent.xyz;
	float3 binormal = get_binormal(isect);
	float2 uv = isect.uv;

	ByteAddressBuffer buf = get_byteaddress_buffer(handle);
	uint4 data0 = buf.Load4(MATERIAL_HEADER_SIZE + 16 * 0);
	uint4 data1 = buf.Load4(MATERIAL_HEADER_SIZE + 16 * 1);
	uint4 data2 = buf.Load4(MATERIAL_HEADER_SIZE + 16 * 2);
	uint4 data3 = buf.Load4(MATERIAL_HEADER_SIZE + 16 * 3);
	uint4 data4 = buf.Load4(MATERIAL_HEADER_SIZE + 16 * 4);
	uint4 data5 = buf.Load4(MATERIAL_HEADER_SIZE + 16 * 5);

	float3 base_normal = normal;
	float3 coat_normal = normal;
	if((data0.x & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float3> tex = get_texture2d<float3>(data0.x & 0x00ffffff);
		float3 local_normal = tex.SampleLevel(RT_SAMPLER, uv, lod);
		local_normal.xy = 2 * local_normal.xy - 1;
		base_normal = normalize(tangent * local_normal.x + binormal * local_normal.y + normal * local_normal.z);
		coat_normal = normalize(tangent * local_normal.x + binormal * local_normal.y + normal * local_normal.z * 2); //パラメータ化？thicknessから？

		//ローカル化
		float3 coat_tangent, coat_binormal;
		calc_orthonormal_basis(coat_normal, coat_tangent, coat_binormal);
		base_normal = float3(dot(coat_tangent, base_normal), dot(coat_binormal, base_normal), dot(coat_normal, base_normal));
	}

	float coat_scale = u8_unorm_to_f32(data0.y >> 24);
	if((data0.y & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float> tex = get_texture2d<float>(data0.y & 0x00ffffff);
		coat_scale *= tex.SampleLevel(RT_SAMPLER, uv, lod);
	}

	float3 coat_color0 = u8x4_unorm_to_f32x4(data4.y).xyz;
	if((data0.z & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float3> tex = get_texture2d<float3> (data0.z & 0x00ffffff);
		coat_color0 *= tex.SampleLevel(RT_SAMPLER, uv, lod);
	}

	float coat_roughness = u8_unorm_to_f32(data0.w >> 24);
	if((data0.w & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float> tex = get_texture2d<float>(data0.w & 0x00ffffff);
		coat_roughness *= tex.SampleLevel(RT_SAMPLER, uv, lod);
	}

	float3 diffuse_color = u8x4_unorm_to_f32x4(data4.z).xyz;
	if((data1.x & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float3> tex = get_texture2d<float3> (data1.x & 0x00ffffff);
		diffuse_color *= tex.SampleLevel(RT_SAMPLER, uv, lod);
	}
	
	float specular_scale = u8_unorm_to_f32(data1.y >> 24);
	if((data1.y & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float> tex = get_texture2d<float>(data1.y & 0x00ffffff);
		specular_scale *= tex.SampleLevel(RT_SAMPLER, uv, lod);
	}

	float3 specular_color0 = u8x4_unorm_to_f32x4(data4.w).xyz;
	if((data1.z & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float3> tex = get_texture2d<float3>(data1.z & 0x00ffffff);
		specular_color0 *= tex.SampleLevel(RT_SAMPLER, uv, lod);
	}

	float specular_roughness = u8_unorm_to_f32(data1.w >> 24);
	if((data1.w & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float> tex = get_texture2d<float>(data1.w & 0x00ffffff);
		specular_roughness *= tex.SampleLevel(RT_SAMPLER, uv, lod);
	}

	float subsurface = u8_unorm_to_f32(data2.x >> 24);
	if((data2.x & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float> tex = get_texture2d<float>(data2.x & 0x00ffffff);
		subsurface *= tex.SampleLevel(RT_SAMPLER, uv, lod);
	}

	float thickness = f16_to_f32(data5.y);
	if((data2.y & 0x00ffffff) != 0x00ffffff)
	{
		float scale = f16_to_f32(data5.y >> 16);
		Texture2D<float> tex = get_texture2d<float>(data2.y & 0x00ffffff);
		thickness += tex.SampleLevel(RT_SAMPLER, uv, lod) * scale;
	}

	float diffuser_cd = u8_unorm_to_f32(data2.z >> 24);
	if((data2.z & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float> tex = get_texture2d<float>(data2.z & 0x00ffffff);
		diffuser_cd *= tex.SampleLevel(RT_SAMPLER, uv, lod);
	}

	float3 diffuser_albedo = u8x4_unorm_to_f32x4(data5.z).xyz;
	if((data2.w & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float3> tex = get_texture2d<float3>(data2.w & 0x00ffffff);
		diffuser_albedo *= tex.SampleLevel(RT_SAMPLER, uv, lod);
	}

	float diffuser_g1 = u8_unorm_to_f32(data3.x >> 24);
	if((data3.x & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float> tex = get_texture2d<float>(data3.x & 0x00ffffff);
		diffuser_g1 *= tex.SampleLevel(RT_SAMPLER, uv, lod);
	}

	float diffuser_g2 = u8_unorm_to_f32(data3.y >> 24);
	if((data3.y & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float> tex = get_texture2d<float>(data3.y & 0x00ffffff);
		diffuser_g2 *= tex.SampleLevel(RT_SAMPLER, uv, lod);
	}

	float diffuser_wg = u8_unorm_to_f32(data3.z >> 24);
	if((data3.z & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float> tex = get_texture2d<float>(data3.z & 0x00ffffff);
		diffuser_wg *= tex.SampleLevel(RT_SAMPLER, uv, lod);
	}

	float3 platelet_albedo = u8x4_unorm_to_f32x4(data5.w).xyz;
	if((data3.w & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float3> tex = get_texture2d<float3>(data3.w & 0x00ffffff);
		platelet_albedo *= tex.SampleLevel(RT_SAMPLER, uv, lod);
	}

	float platelet_roughness = u8_unorm_to_f32(data4.x >> 24);
	if((data4.x & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float> tex = get_texture2d<float>(data4.x & 0x00ffffff);
		platelet_roughness *= tex.SampleLevel(RT_SAMPLER, uv, lod);
	}

	float3 subsurface_radius = r9g9b9e5_to_f32x3(data5.x);

	specular_roughness = max(specular_roughness, 1e-3);
	platelet_roughness = max(platelet_roughness, 1e-3);

	//単色と仮定
	float coat_eta = calc_eta(coat_color0.x);
	float specular_eta = calc_eta(specular_color0.x);

	cosmetic_material mtl;

#if defined(COMPRESS_MATERIAL)
	
	mtl.base_normal = f32x2_to_u16x2_unorm(f32x3_to_oct(base_normal) * 0.5 + 0.5);
	mtl.coat_normal = f32x2_to_u16x2_unorm(f32x3_to_oct(coat_normal) * 0.5 + 0.5);
	mtl.diffuse_color = f32x4_to_r10g10b10a2(float4(diffuse_color, 0));
	mtl.specular_color0 = f32x4_to_r10g10b10a2(float4(specular_color0, 0));
	mtl.subsurface_radius = f32x3_to_r9g9b9e5(subsurface_radius);

	mtl.diffuser_albedo = f32x4_to_r10g10b10a2(float4(diffuser_albedo, 0));
	mtl.platelet_albedo = f32x4_to_r10g10b10a2(float4(platelet_albedo, 0));

	mtl.data0 = f32x4_to_r10g10b10a2(float4(coat_roughness, specular_roughness, platelet_roughness, 0));
	mtl.data1 = f32x4_to_r10g10b10a2(float4(coat_scale, specular_scale, 0, 0));
	mtl.data2 = f32x4_to_r10g10b10a2(float4(subsurface, diffuser_cd, 0, 0));
	mtl.data3 = f32x4_to_r10g10b10a2(float4(diffuser_g1, diffuser_g2, diffuser_wg, 0));
	mtl.data4 = f32x2_to_f16x2(float2(coat_eta, specular_eta));
	mtl.data5 = asuint(thickness);

#else

#endif

	return mtl;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

bool has_contribution(float nwi, cosmetic_material mtl)
{
	if(nwi > cosine_threshold)
		return true;
	else
		return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

enum sample_type
{
	sample_type_brdf, 
	sample_type_btdf, 
	sample_type_default,
};

bsdf_sample sample_bsdf(float3 wo, float eta_o, float eta_i, float roughness, float u0, float u1, float u2, sample_type sample_type = sample_type_default)
{
	bsdf_sample s;
	s.is_valid = false;

	float3 h = microfacet::sample_vndf(wo, roughness, u0, u1);

	float alpha = pow2(roughness);
	float hwo = abs(dot(h, wo));
	float Fr = fresnel_dielectric(hwo, eta_i / eta_o);

	float Pr;
	if(sample_type == sample_type_default)
		Pr = Fr;
	else if(sample_type == sample_type_brdf)
		Pr = 1;
	else if(sample_type == sample_type_btdf)
	{
		Pr = 0;

		//全反射
		if(Fr >= 1)
			return s;
	}

	if(u2 < Pr)
	{
		float3 wi = reflect(-wo, h);
		if(wi.z <= 0)
			return s;
		
		float D = microfacet::D(h, alpha);
		float G = microfacet::G(wo, wi, alpha);
		float G1 = microfacet::G1(wo, alpha);

		s.w = wi;

		s.weight = G / G1 * (Fr / Pr);
		s.pdf = Pr * G1 * D / (4 * wo.z);
	}
	else
	{
		float3 wi = refract(-wo, h, eta_o / eta_i);
		if(wi.z >= 0)
			return s;

		float D = microfacet::D(h, alpha);
		float G = microfacet::G(wo, wi, alpha);
		float G1 = microfacet::G1(wo, alpha);
		float hwi = abs(dot(h, wi));
		float hwo = abs(dot(h, wo));

		s.w = wi;
		s.weight = hwi * G / G1 * ((1 - Fr) / (1 - Pr));
		s.pdf = (1 - Pr) * G1 * D * hwo * hwi * pow2(eta_i) / (wo.z * pow2(eta_i * hwi + eta_o * hwo));
	}
	return s;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

float2 calc_bsdf_pdf(float3 wo, float3 wi, float eta_o, float eta_i, float roughness)
{
	float alpha = pow2(roughness);

	//brdf
	if(wo.z * wi.z > 0)
	{
		float3 h = normalize(wo + wi);
		float hwo = abs(dot(h, wo));

		float F = fresnel_dielectric(hwo, eta_i / eta_o);
		float D = microfacet::D(h, alpha);
		float G = microfacet::G(wo, wi, alpha);
		float G1 = microfacet::G1(wo, alpha);
		float common = F * D / (4 * wo.z);

		float brdf = G * common;
		float pdf = G1 * common;
		return float2(brdf, pdf);)
	}
	//btdf
	else
	{
		float3 h = normalize(eta_o * wo + eta_i * wi);
		if(eta_o < eta_i)
			h = -h;

		if(h.z <= 0)
			return 0;

		float hwo = dot(h, wo);
		float hwi = dot(h, wi);
		if(hwo * hwi >= 0)
			return 0;

		hwo = abs(hwo);
		hwi = abs(hwi);
		float nwo = abs(wo.z);
		float nwi = abs(wi.z);

		float F = fresnel_dielectric(hwo, eta_i / eta_o);
		float D = microfacet::D(h, alpha);
		float G = microfacet::G(wo, wi, alpha);
		float G1 = microfacet::G1(wo, alpha);
		float common = (1 - F) * D * hwo * pow2(eta_i) / (nwo * pow2(eta_o * hwo + eta_i * hwi));

		float btdf = G * hwi * common;
		float pdf = G1 * common;
		return float2(btdf, pdf);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

float calc_pdf(float3 wo, float3 wi, float3 normal, cosmetic_material mtl, inout rng rng)
{
	//MISに使うだけなので近似値を使用
	//wo,wiは方向はローカル空間

	float coat_eta = get_coat_eta(mtl);
	float coat_roughness = get_coat_roughness(mtl);

	//coatで反射のpdf
	float pdf = calc_bsdf_pdf(wo, wi, 1, coat_eta, coat_roughness).y;

	//透過方向をサンプリング
	bsdf_sample s = sample_bsdf(wo, 1, coat_eta, coat_roughness, randF(rng), randF(rng), 0, sample_type_btdf);
	if(s.is_valid)
	{
		float sigma_t = 1;
		float T1 = transmittance(sigma_t, get_thickness(mtl) / abs(s.w.z));
		pdf += s.weight.x * (1 - T1) * wi.z * inv_PI;
		s.weight.x *= T1;

		float3 base_tangent, base_binormal, base_normal = get_base_normal(mtl);
		calc_orthonormal_basis(base_normal, base_tangent, base_binormal);

		s.w = float3(dot(s.w, base_tangent), dot(s.w, base_binormal), dot(s.w, base_normal));
		if(s.w.z < 0)
		{
			//反射方向をサンプリング
			bsdf_sample s2 = sample_bsdf(-s.w, coat_eta, get_specular_eta(mtl), get_specular_roughness(mtl), randF(rng), randF(rng), 0, sample_type_brdf);
			if(s2.is_valid)
			{
				s2.w = s2.w.x * base_tangent + s2.w.y * base_binormal + s2.w.z * base_normal;

				float T2 = transmittance(sigma_t, get_thickness(mtl) / abs(s2.w.z));
				pdf += s.weight.x * s2.weight.x * (1 - T2) * wi.z * inv_PI;
				s2.weight.x *= T2;

				//coatで透過のpdf
				//法線が下向きのローカル空間なので入射出射の符号がこうなる
				pdf += s.weight.x * s2.weight.x * calc_bsdf_pdf(s2.w, -wi, coat_eta, 1, coat_roughness).y;
			}
		}
	}
	return pdf;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

float4 calc_bsdf_pdf(float3 wo, float3 wi, float3 normal, cosmetic_material mtl, inout rng rng, uint2 dtid = 0)
{
	float3 coat_tangent, coat_binormal;
	float3 coat_normal = get_coat_normal(mtl);
	calc_orthonormal_basis(coat_normal, coat_tangent, coat_binormal);

	wo = float3(dot(wo, coat_tangent), dot(wo, coat_binormal), dot(wo, coat_normal));
	wi = float3(dot(wi, coat_tangent), dot(wi, coat_binormal), dot(wi, coat_normal));

	float pdf = calc_pdf(wo, wi, normal, mtl, rng);

	//coatで反射
	float3 bsdf = calc_bsdf_pdf(wo, wi, 1, get_coat_eta(mtl), get_coat_roughness(mtl)).x;

	//透過方向をサンプリング
	bsdf_sample s = sample_bsdf(wo, 1, get_coat_eta(mtl), get_coat_roughness(mtl), randF(rng), randF(rng), 0, sample_type_btdf);
	if(!s.is_valid)
		return float4(bsdf, pdf);

	//反対側からも透過方向をサンプリング
	bsdf_sample s2 = sample_bsdf(wi, 1, get_coat_eta(mtl), get_coat_roughness(mtl), randF(rng), randF(rng), 0, sample_type_btdf);
	if(!s2.is_valid)
		return float4(bsdf, pdf);

	sggx_param sggx_param;
	sggx_param.Sxx = coat_normal.x * coat_normal.x + get_platelet_roughness(mtl) * (coat_normal.y * coat_normal.y + coat_normal.z * coat_normal.z);
	sggx_param.Syy = coat_normal.y * coat_normal.y + get_platelet_roughness(mtl) * (coat_normal.x * coat_normal.x + coat_normal.z * coat_normal.z);
	sggx_param.Szz = coat_normal.z * coat_normal.z + get_platelet_roughness(mtl) * (coat_normal.x * coat_normal.x + coat_normal.y * coat_normal.y);
	sggx_param.Sxy = coat_normal.x * coat_normal.y - get_platelet_roughness(mtl) * (coat_normal.x * coat_normal.y);
	sggx_param.Sxz = coat_normal.x * coat_normal.z - get_platelet_roughness(mtl) * (coat_normal.x * coat_normal.z);
	sggx_param.Syz = coat_normal.y * coat_normal.z - get_platelet_roughness(mtl) * (coat_normal.y * coat_normal.z);

	float z = 0;
	float thickness = get_thickness(mtl);
	for(uint i = 0; i < 128; i++)
	{
		float sigma_t = 1;
		float t = sample_medium_distance(sigma_t, randF(rng));
		z += s.w.z * t;

		//air
		if(z > 0)
		{
			//s.wは点に向かう方向だが，法線が下向きなので反転させてs.wのままでいい
			bsdf_sample s3 = sample_bsdf(s.w, get_coat_eta(mtl), 1, get_coat_roughness(mtl), randF(rng), randF(rng), randF(rng));
			if(!s3.is_valid)
				break;

			s.w = -s3.w; //法線が下向きの空間から戻す
			s.weight *= s3.weight;

			//反射
			if(s.w.z < 0)
			{
				z = 0;
			}
			//透過
			else
			{
				break;
			}
		}
		//volume
		else if(z > -thickness)
		{
			float cd = get_diffuser_cd(mtl);
			float wg = get_diffuser_wg(mtl);
			float g1 = get_diffuser_g1(mtl);
			float g2 = get_diffuser_g2(mtl);
			float3 Ad = get_diffuser_albedo(mtl);
			float3 Ap = get_platelet_albedo(mtl);

			float proj_area = sggx_projected_area(s.w, sggx_param); 
			float fd = lerp(hg_phase(-s.w, -s2.w, g2), hg_phase(-s.w, -s2.w, g1), wg);
			float fp = sggx_specular_phase(-s.w, -s2.w, sggx_param, proj_area);
			float T = transmittance(sigma_t, z / abs(s2.w.z));
			bsdf += s.weight * (cd * fd * Ad + (1 - cd) * proj_area * fp * Ap) * T;

			float u0 = randF(rng);
			float u1 = randF(rng);

			//diffuser
			float pmf_d = cd / (cd + (1 - cd) * proj_area);
			if(u0 < pmf_d)
			{
				u0 /= pmf_d;

				float g, pmf_d1 = wg;
				if(u1 < pmf_d1)
				{
					u1 /= pmf_d1;
					g = g1;
				}
				else
				{
					u1 = (u1 - pmf_d1) / (1 - pmf_d1);
					g = g2;
				}

				s.wi = sample_hg_phase(-s.w, g, u0, u1);
				s.weight *= Ad;
			}
			//platelet
			else
			{
				u0 = (u0 - pmf_d) / (1 - pmf_d);

				s.wi = sample_sggx_specular_phase(-s.w, sggx_param, u0, u1);
				s.weight *= Ap;
			}
		}
		//skin
		else
		{
			float3 base_tangent, base_binormal, base_normal = get_base_normal(mtl);
			calc_orthonormal_basis(base_normal, base_tangent, base_binormal);

			float3 local_wo = -float3(dot(s.w, base_tangent), dot(s.w, base_binormal), dot(s.w, base_normal));
			if(local_wo.z > 0)
			{
				float3 local_wi = -float3(dot(s2.w, base_tangent), dot(s2.w, base_binormal), dot(s2.w, base_normal));
				if(local_wi.z > 0)
				{
					float T = transmittance(sigma_t, get_thickness(mtl) / abs(s2.w.z));
					bsdf += s.weight * calc_bsdf_pdf(local_wo, local_wi, get_coat_eta(mtl), get_specular_eta(mtl), get_specular_roughness(mtl)).x * T;
				}

				bsdf_sample s3 = sample_bsdf(local_wo, get_coat_eta(mtl), get_specular_eta(mtl), get_specular_roughness(mtl), randF(rng), randF(rng), randF(rng));
				if(!s3.is_valid)
					break;

				s.w = s3.w.x * base_tangent + s3.w.y * base_binormal + s3.w.z * base_normal;
				s.weight *= s3.weight;
			}

			//反射
			if(s.w.z > 0)
			{
				z = -thickness;
			}
			//透過(SSS)
			else
			{
				break;
			}
		}
	}
	return float4(bsdf, pdf);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

bsdf_sample sample_bsdf(float3 wo, float3 normal, cosmetic_material mtl, float u0, float u1, uint2 dtid = 0)
{
	rng rng;
	rng.state = asuint(u0) ^ asuint(u1);

	float3 coat_tangent, coat_binormal;
	float3 coat_normal = get_coat_normal(mtl);
	calc_orthonormal_basis(coat_normal, coat_tangent, coat_binormal);

	float3 local_wo = float3(dot(wo, coat_tangent), dot(wo, coat_binormal), dot(wo, coat_normal));
	bsdf_sample s = sample_bsdf(local_wo, 1, get_coat_eta(mtl), get_coat_roughness(mtl), u0, u1, randF(rng));
	if(!s.is_valid)
		return s;

	//反射なら終了
	if(s.w.z > 0)
	{
		s.w = s.w.x * coat_tangent + s.w.y * coat_binormal + s.w.z * coat_normal;
		s.is_valid = (dot(s.w, normal) > cosine_threshold);
		s.non_diffuse_weight = s.weight;
		s.diffuse_weight = 0;
		return s;
	}

	sggx_param sggx_param;
	sggx_param.Sxx = coat_normal.x * coat_normal.x + get_platelet_roughness(mtl) * (coat_normal.y * coat_normal.y + coat_normal.z * coat_normal.z);
	sggx_param.Syy = coat_normal.y * coat_normal.y + get_platelet_roughness(mtl) * (coat_normal.x * coat_normal.x + coat_normal.z * coat_normal.z);
	sggx_param.Szz = coat_normal.z * coat_normal.z + get_platelet_roughness(mtl) * (coat_normal.x * coat_normal.x + coat_normal.y * coat_normal.y);
	sggx_param.Sxy = coat_normal.x * coat_normal.y - get_platelet_roughness(mtl) * (coat_normal.x * coat_normal.y);
	sggx_param.Sxz = coat_normal.x * coat_normal.z - get_platelet_roughness(mtl) * (coat_normal.x * coat_normal.z);
	sggx_param.Syz = coat_normal.y * coat_normal.z - get_platelet_roughness(mtl) * (coat_normal.y * coat_normal.z);

	float z = 0;
	float thickness = get_thickness(mtl);
	for(uint i = 0; i < 128; i++)
	{
		float sigma_t = 1;
		float t = sample_medium_distance(sigma_t, randF(rng));
		z += s.w.z * t;

		//air
		if(z > 0)
		{
			//s.wは点に向かう方向だが，法線が下向きなので反転させてs.wのままでいい
			bsdf_sample s2 = sample_bsdf(s.w, get_coat_eta(mtl), 1, get_coat_roughness(mtl), randF(rng), randF(rng), randF(rng));
			if(!s2.is_valid)
				return s2;

			s.w = -s2.w; //法線が下向きの空間から戻す
			s.weight *= s2.weight;

			//反射
			if(s.w.z < 0)
			{
				z = 0;
			}
			//透過
			else
			{
				s.diffuse_weight = 0;
				s.non_diffuse_weight = s.weight;
				s.pdf = calc_pdf(local_wo, s.w, normal, mtl);
				s.w = coat_tangent * s.w.x + coat_binormal * s.w.y + coat_normal * s.w.z;
				s.is_valid = (dot(s.w, normal) > cosine_threshold);
				break;
			}
		}
		//volume
		else if(z > -thickness)
		{
			float cd = get_diffuser_cd(mtl);
			float proj_area = sggx_projected_area(s.w, sggx_param); 
			float pmf_d = cd / (cd + (1 - cd) * proj_area);

			float u0 = randF(rng);
			float u1 = randF(rng);

			//diffuser
			if(u0 < pmf_d)
			{
				u0 /= pmf_d;

				float g, pmf_d1 = get_diffuser_wg(mtl);
				if(u1 < pmf_d1)
				{
					u1 /= pmf_d1;
					g = get_diffuser_g1(mtl);
				}
				else
				{
					u1 = (u1 - pmf_d1) / (1 - pmf_d1);
					g = get_diffuser_g2(mtl);
				}

				s.w = sample_hg_phase(-s.w, g, u0, u1);
				s.weight *= get_diffuser_albedo(mtl);
			}
			//platelet
			else
			{
				u0 = (u0 - pmf_d) / (1 - pmf_d);

				s.w = sample_sggx_specular_phase(-s.w, sggx_param, u0, u1);
				s.weight *= get_platelet_albedo(mtl);
			}
		}
		//skin
		else
		{
			float3 base_tangent, base_binormal, base_normal = get_base_normal(mtl);
			calc_orthonormal_basis(base_normal, base_tangent, base_binormal);

			float3 local_wo = -float3(dot(s.w, base_tangent), dot(s.w, base_binormal), dot(s.w, base_normal));
			if(local_wo.z > 0)
			{
				bsdf_sample s2 = sample_bsdf(local_wo, get_coat_eta(mtl), get_specular_eta(mtl), get_specular_roughness(mtl), randF(rng), randF(rng), randF(rng));
				if(!s2.is_valid)
					return s2;

				s.w = s2.w.x * base_tangent + s2.w.y * base_binormal + s2.w.z * base_normal;
				s.weight *= s2.weight;
			}

			//反射
			if(s.w.z > 0)
			{
				z = -thickness;
			}
			//透過(SSS)
			else
			{
				s.weight *= get_diffuse_color(mtl);
				s.diffuse_weight = s.weight;
				s.non_diffuse_weight = 0;
				s.is_valid = true;

				//方向は下半球なら何でもOK
				s.w = -s.normal;

				//SSSでPDFは使わないから何でもOK
				s.pdf = 1;
				break;
			}
		}
	}
	return s;
}

float3 calc_subsurface(cosmetic_material mtl)
{
	return 1; //もうこれでいいや．出射点側の情報だけでSSS感を出す
}

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

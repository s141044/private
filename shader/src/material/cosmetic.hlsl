
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
	uint	data1; //coat_scale, specular_scale, specular_reflectance, 
	uint	data2; //subsurface, diffuser_cd, -
	uint	data3; //diffuser_g1, diffuser_g2, diffuser_wg
	unit	data4; //thickness,eta
};

float3	get_base_normal(cosmetic_material mtl){ return oct_to_f32x3(u16x2_unorm_to_f32x2(mtl.base_normal) * 2 - 1); }
float3	get_coat_normal(cosmetic_material mtl){ return oct_to_f32x3(u16x2_unorm_to_f32x2(mtl.coat_normal) * 2 - 1); }
float	get_coat_scale(cosmetic_material mtl){ return r10g10b10a2_to_f32x4(mtl.data1).x; }
float	get_coat_roughness(cosmetic_material mtl){ return r10g10b10a2_to_f32x4(mtl.data0).x; }
float	get_specular_scale(cosmetic_material mtl){ return r10g10b10a2_to_f32x4(mtl.data1).y; }
float3	get_specular_color0(cosmetic_material mtl){ return r10g10b10a2_to_f32x4(mtl.specular_color0).xyz; }
float	get_specular_roughness(cosmetic_material mtl){ return r10g10b10a2_to_f32x4(mtl.data0).y; }
float	get_specular_reflectance(cosmetic_material mtl){ return r10g10b10a2_to_f32x4(mtl.data1).z; }
float3	get_diffuse_color(cosmetic_material mtl){ return r10g10b10a2_to_f32x4(mtl.diffuse_color).xyz; }
float	get_subsurface(cosmetic_material mtl){ return r10g10b10a2_to_f32x4(mtl.data2).x; }
float3	get_subsurface_radius(cosmetic_material mtl){ return r9g9b9e5_to_f32x3(mtl.subsurface_radius); }

float	get_eta(cosmetic_material mtl){ return f16_to_f32(mtl.data4 >> 16); }
float	get_thickness(cosmetic_material mtl){ return f16_to_f32(mtl.data4); }
float3	get_diffuser_albedo(cosmetic_material mtl){ return r10g10b10a2_to_f32x4(mtl.diffuser_albedo).xyz; }
float	get_diffuser_cd(cosmetic_material mtl){ return r10g10b10a2_to_f32x4(mtl.data2).y; }
float	get_diffuser_g1(cosmetic_material mtl){ return r10g10b10a2_to_f32x4(mtl.data3).x; }
float	get_diffuser_g2(cosmetic_material mtl){ return r10g10b10a2_to_f32x4(mtl.data3).y; }
float	get_diffuser_wg(cosmetic_material mtl){ return r10g10b10a2_to_f32x4(mtl.data3).z; }
float3	get_platelet_albedo(cosmetic_material mtl){ return r10g10b10a2_to_f32x4(mtl.platelet_albedo).xyz; }
float	get_platelet_roughness(cosmetic_material mtl){ return r10g10b10a2_to_f32x4(mtl.data0).z; }

#else

struct cosmetic_material
{
	float3	base_normal;
	float3	diffuse_color;
	float	diffuse_fresnel;
	float	diffuse_fresnel_falloff;
	float	diffuse_fresnel_tangent_falloff;
	float	retroreflection;
	float	retroreflection_falloff;
	float	retroreflection_tangent_falloff;
	float	specular_scale;
	float3	specular_color0;
	float	specular_roughness;
	float	specular_fresnel_falloff;
	float	smooth_terminator;
	float	smooth_terminator_length;
	float	subsurface;
	float3	subsurface_radius;
	uint	flags;
};

float3	get_coat_normal(cosmetic_material mtl){ return mtl.coat_normal; }
float3	get_base_normal(cosmetic_material mtl){ return mtl.base_normal; }
float3	get_sheen_color(cosmetic_material mtl){ return mtl.sheen_color; }
float	get_sheen_roughness(cosmetic_material mtl){ return mtl.sheen_roughness; }
float3	get_sheen_reflectance(cosmetic_material mtl){ return mtl.sheen_reflectance; }
float	get_coat_scale(cosmetic_material mtl){ return mtl.coat_scale; }
float3	get_coat_color0(cosmetic_material mtl){ return mtl.coat_color0; }
float	get_coat_roughness(cosmetic_material mtl){ return mtl.coat_roughness; }
float3	get_coat_reflectance(cosmetic_material mtl){ return mtl.coat_reflectance; }
float3	get_emissive_color(cosmetic_material mtl){ return mtl.emissive_color; }
float	get_specular_scale(cosmetic_material mtl){ return mtl.specular_scale; }
float3	get_specular_color0(cosmetic_material mtl){ return mtl.specular_color0; }
float	get_specular_roughness(cosmetic_material mtl){ return mtl.specular_roughness; }
float3	get_specular_reflectance(cosmetic_material mtl){ return mtl.specular_reflectance; }
float3	get_matte_reflectance(cosmetic_material mtl){ return mtl.matte_reflectance; }
float3	get_diffuse_color(cosmetic_material mtl){ return mtl.diffuse_color; }
float	get_diffuse_roughness(cosmetic_material mtl){ return mtl.diffuse_roughness; }
float3	get_diffuse_reflectance(cosmetic_material mtl){ return mtl.diffuse_reflectance; }
float	get_subsurface(cosmetic_material mtl){ return mtl.subsurface; }
float3	get_subsurface_radius(cosmetic_material mtl){ return mtl.subsurface_radius; }
bool	is_twoside(cosmetic_material mtl){ return mtl.flags & 1; }

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
	uint	thickness;
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

	float3 base_normal = normal;
	float3 coat_normal = normal;
	if((data0.x & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float3> tex = get_texture2d<float3>(data0.x & 0x00ffffff);
		float3 local_normal = tex.SampleLevel(RT_SAMPLER, uv, lod);
		local_normal.xy = 2 * local_normal.xy - 1;
		base_normal = normalize(tangent * local_normal.x + binormal * local_normal.y + normal * local_normal.z);
		coat_normal = normalize(tangent * local_normal.x + binormal * local_normal.y + normal * local_normal.z * 2); //パラメータ化？thicknessから？
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
		diffuse_color *= tex.SampleLevel(RT_SAMPLER, uv, lod);
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
		Texture2D<float> tex = get_texture2d<float3>(data2.w & 0x00ffffff);
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
		Texture2D<float> tex = get_texture2d<float3>(data3.w & 0x00ffffff);
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
	float specular_reflectance = luminance(microfacet::calc_reflectance(wo, specular_color0, specular_roughness));
	specular_reflectance *= specular_scale;

	float sqrt_F0 = clamp(sqrt(luminance(coat_color0)), 0, 0.999);
	float eta = (1 + sqrt_F0) / (1 - sqrt_F0);

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
	mtl.data1 = f32x4_to_r10g10b10a2(float4(coat_scale, specular_scale, specular_reflectance, 0));
	mtl.data2 = f32x4_to_r10g10b10a2(float4(subsurface, diffuser_cd, 0, 0));
	mtl.data3 = f32x4_to_r10g10b10a2(float4(diffuser_g1, diffuser_g2, diffuser_wg, 0));
	mtl.data4 = f32x2_to_f16x2(float2(thickness, eta));

#else

	mtl.coat_normal = coat_normal;
	mtl.base_normal = base_normal;
	mtl.sheen_color = sheen_color;
	mtl.sheen_roughness = sheen_roughness;
	mtl.sheen_reflectance = sheen_reflectance;
	mtl.coat_scale = coat_scale;
	mtl.coat_color0 = coat_color0;
	mtl.coat_roughness = coat_roughness;
	mtl.coat_reflectance = coat_reflectance;
	mtl.emissive_color = emissive_color;
	mtl.specular_scale = specular_scale;
	mtl.specular_color0 = specular_color0;
	mtl.specular_roughness = specular_roughness;
	mtl.specular_reflectance = specular_reflectance;
	mtl.matte_reflectance = matte_reflectance;
	mtl.diffuse_color = diffuse_color;
	mtl.diffuse_roughness = diffuse_roughness;
	mtl.diffuse_reflectance = diffuse_reflectance;
	mtl.subsurface = subsurface;
	mtl.subsurface_radius = subsurface_radius;

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

void calc_bsdf_pdf(float3 wo, float3 wi, float3 normal, cosmetic_material mtl, out float3 diffuse, out float3 non_diffuse, out float pdf, uint2 dtid = 0)
{
	//TODO
	diffuse = 0;
	non_diffuse = 0;
	pdf = 0;
}

float4 calc_bsdf_pdf(float3 wo, float3 wi, float3 normal, cosmetic_material mtl, uint2 dtid = 0)
{
	float pdf;
	float3 diffuse, non_diffuse;
	calc_bsdf_pdf(wo, wi, normal, mtl, diffuse, non_diffuse, pdf, dtid);
	return float4(diffuse + non_diffuse, pdf);
}

float3 calc_subsurface(cosmetic_material mtl)
{
	return 1; //もうこれでいいや．出射点側の情報だけでSSS感を出す
}

///////////////////////////////////////////////////////////////////////////////////////////////////

bsdf_sample sample_bsdf(float3 wo, float eta_o, float eta_i, float roughness, float u0, float u1, float u2)
{
	bsdf_sample s;
	s.is_valid = false;

	float3 h = microfacet::sample_vndf(wo, roughness, u0, u1);

	float alpha = pow2(roughness);
	float hwo = abs(dot(h, wo));
	float Fr = fresnel_dielectric(hwo, eta_i / eta_o);
	if(u2 < Fr)
	{
		float3 wi = reflect(-wo, h);
		if(wi.z <= 0)
			return s;
		
		float D = microfacet::D(h, alpha);
		float G = microfacet::G(wo, wi, alpha);
		float G1 = microfacet::G1(wo, alpha);

		s.w = wi;
		s.weight = G / G1;
		s.pdf = Fr * G1 * D / (4 * wo.z);
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
		s.weight = hwi * G / G1;
		s.pdf = (1 - Fr) * G1 * D * hwo * hwi * pow2(eta_i) / (wo.z * pow2(eta_i * hwi + eta_o * hwo));
	}
	return s;
}

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

	float z = 0;
	for(uint i = 0; i < 128; i++)
	{
		float sigma_t = 1;
		float t = sample_medium_distance(sigma_t, randF(rng));
		z += s.w.z * t;

		//air
		if(z > 0)
		{
			bsdf_sample s2 = sample_bsdf(s.w, get_coat_eta(mtl), 1, get_coat_roughness(mtl), randF(rng), randF(rng), randF(rng));
			if(!s2.is_valid)
				return s2;

			s.w = -s2.w;
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
				s.w = coat_tangent * s.w.x + coat_binormal * s.w.y + coat_normal * s.w.z;
				s.is_valid = (dot(s.w, normal) > cosine_threshold);
				break;
			}
		}
		//volume
		else if(z > -thickness)
		{

			//diffuser
			if()
			{
				local_wi;
			}
			//platelet
			else
			{
				local_wi;
			}

		}
		//skin
		else
		{
			float3 base_normal = get_base_normal(mtl);
			base_normal = float3(dot(base_normal, coat_tangent), dot(base_normal, coat_binormal), dot(base_normal, coat_normal));
			
			float3 base_tangent, base_binormal;
			calc_orthonormal_basis(coat_normal, coat_tangent, coat_binormal);

			bsdf_sample s2 = sample_bsdf(-s.w, get_coat_eta(mtl), get_base_eta(mtl), get_specular_roughness(mtl), randF(rng), randF(rng), randF(rng));
			if(!s2.is_valid)
				return s2;

			s.w = s2.w;
			s.weight *= s2.weight;

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
				break;
			}
		}
	}

	s.pdf = 1e15; //MISでBSDFサンプリングの重みが1になるように大きい値を適当に設定. TODO: 近似PDF対応
	return s;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

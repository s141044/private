
#ifndef MATERIAL_STANDARD_HLSL
#define MATERIAL_STANDARD_HLSL

#include"../packing.hlsl"
#include"../static_sampler.hlsl"

#include"../bsdf/sheen.hlsl"
#include"../bsdf/microfacet.hlsl"
#include"../bsdf/oren_nayer.hlsl"

//Ç‹Ç†å≈íËÇ≈ëÂè‰ïvÇæÇÎÇ§
#if !defined(RT_SAMPLER)
#define RT_SAMPLER bilinear_wrap
#endif

#define COMPRESS_MATERIAL

///////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(COMPRESS_MATERIAL)

struct standard_material
{
	uint	coat_normal;
	uint	base_normal;
	uint	sheen_color;
	uint	sheen_reflectance;
	uint	coat_color0;
	uint	coat_reflectance;
	uint	emission_color;
	uint	specular_color0;
	uint	specular_reflectance;
	uint	matte_reflectance;
	uint	diffuse_color;
	uint	diffuse_reflectance;
	uint	subsurface_radius;
	uint	roughness; //sheen,coat,specular
	uint	roughness_subsurface_misc; //diffuse,subsurface
	uint	scale; //coat,specular
};

float3	get_coat_normal(standard_material mtl){ return oct_to_f32x3(u16x2_unorm_to_f32x2(mtl.coat_normal) * 2 - 1); }
float3	get_base_normal(standard_material mtl){ return oct_to_f32x3(u16x2_unorm_to_f32x2(mtl.base_normal) * 2 - 1); }
float3	get_sheen_color(standard_material mtl){ return r10g10b10a2_to_f32x4(mtl.sheen_color).xyz; }
float	get_sheen_roughness(standard_material mtl){ return r10g10b10a2_to_f32x4(mtl.roughness).x; }
float3	get_sheen_reflectance(standard_material mtl){ return r10g10b10a2_to_f32x4(mtl.sheen_reflectance).xyz; }
float	get_coat_scale(standard_material mtl){ return r10g10b10a2_to_f32x4(mtl.scale).x; }
float3	get_coat_color0(standard_material mtl){ return r10g10b10a2_to_f32x4(mtl.coat_color0).xyz; }
float	get_coat_roughness(standard_material mtl){ return r10g10b10a2_to_f32x4(mtl.roughness).y; }
float3	get_coat_reflectance(standard_material mtl){ return r10g10b10a2_to_f32x4(mtl.coat_reflectance).xyz; }
float3	get_emission_color(standard_material mtl){ return r9g9b9e5_to_f32x3(mtl.emission_color); }
float	get_specular_scale(standard_material mtl){ return r10g10b10a2_to_f32x4(mtl.scale).y; }
float3	get_specular_color0(standard_material mtl){ return r10g10b10a2_to_f32x4(mtl.specular_color0).xyz; }
float	get_specular_roughness(standard_material mtl){ return r10g10b10a2_to_f32x4(mtl.roughness).z; }
float3	get_specular_reflectance(standard_material mtl){ return r10g10b10a2_to_f32x4(mtl.specular_reflectance).xyz; }
float3	get_matte_reflectance(standard_material mtl){ return r10g10b10a2_to_f32x4(mtl.matte_reflectance).xyz; }
float3	get_diffuse_color(standard_material mtl){ return r10g10b10a2_to_f32x4(mtl.diffuse_color).xyz; }
float	get_diffuse_roughness(standard_material mtl){ return r10g10b10a2_to_f32x4(mtl.roughness_subsurface_misc).x; }
float3	get_diffuse_reflectance(standard_material mtl){ return r10g10b10a2_to_f32x4(mtl.diffuse_reflectance).xyz; }
float	get_subsurface(standard_material mtl){ return r10g10b10a2_to_f32x4(mtl.roughness_subsurface_misc).y; }
float3	get_subsurface_radius(standard_material mtl){ return r9g9b9e5_to_f32x3(mtl.subsurface_radius); }

#else

struct standard_material
{
	float3	coat_normal;
	float3	base_normal;
	float3	sheen_color;
	float	sheen_roughness;
	float3	sheen_reflectance;
	float	coat_scale;
	float3	coat_color0;
	float	coat_roughness;
	float3	coat_reflectance;
	float3	emission_color;
	float	specular_scale;
	float3	specular_color0;
	float	specular_roughness;
	float3	specular_reflectance;
	float3	matte_reflectance;
	float3	diffuse_color;
	float	diffuse_roughness;
	float3	diffuse_reflectance;
	float	subsurface;
	float3	subsurface_radius;
};

float3	get_coat_normal(standard_material mtl){ return mtl.coat_normal; }
float3	get_base_normal(standard_material mtl){ return mtl.base_normal; }
float3	get_sheen_color(standard_material mtl){ return mtl.sheen_color; }
float	get_sheen_roughness(standard_material mtl){ return mtl.sheen_roughness; }
float3	get_sheen_reflectance(standard_material mtl){ return mtl.sheen_reflectance; }
float	get_coat_scale(standard_material mtl){ return mtl.coat_scale; }
float3	get_coat_color0(standard_material mtl){ return mtl.coat_color0; }
float	get_coat_roughness(standard_material mtl){ return mtl.coat_roughness; }
float3	get_coat_reflectance(standard_material mtl){ return mtl.coat_reflectance; }
float3	get_emission_color(standard_material mtl){ return mtl.emission_color; }
float	get_specular_scale(standard_material mtl){ return mtl.specular_scale; }
float3	get_specular_color0(standard_material mtl){ return mtl.specular_color0; }
float	get_specular_roughness(standard_material mtl){ return mtl.specular_roughness; }
float3	get_specular_reflectance(standard_material mtl){ return mtl.specular_reflectance; }
float3	get_matte_reflectance(standard_material mtl){ return mtl.matte_reflectance; }
float3	get_diffuse_color(standard_material mtl){ return mtl.diffuse_color; }
float	get_diffuse_roughness(standard_material mtl){ return mtl.diffuse_roughness; }
float3	get_diffuse_reflectance(standard_material mtl){ return mtl.diffuse_reflectance; }
float	get_subsurface(standard_material mtl){ return mtl.subsurface; }
float3	get_subsurface_radius(standard_material mtl){ return mtl.subsurface_radius; }

#endif

///////////////////////////////////////////////////////////////////////////////////////////////////

struct standard_material_host
{
	//0
	uint	alpha_map;
	uint	coat_normal_map;
	uint	base_normal_map;
	uint	sheen_color_map;
	//1
	uint	sheen_roughness_map;
	uint	coat_scale_map;
	uint	coat_color0_map;
	uint	coat_roughness_map;
	//2
	uint	emission_scale_map;
	uint	emission_color_map;
	uint	specular_scale_map;
	uint	specular_color0_map;
	//3
	uint	specular_roughness_map;
	uint	diffuse_color_map;
	uint	diffuse_roughness_map;
	uint	subsurface_map;
	//4
	uint	emission_color;
	uint	subsurface_radius;
	uint	sheen_coat_color; //sheen.xyz,coat.x
	uint	coat_specular_color; //coat.yz,specular.xy
	//5
	uint	specular_diffuse_color; //specular.z,diffuse_xyz
};

///////////////////////////////////////////////////////////////////////////////////////////////////

standard_material load_standard_material(uint handle, float3 wo, float3 normal, float3 tangent, float3 binormal, float2 uv, float lod = 0, uint2 dtid = 0)
{
	ByteAddressBuffer buf = get_byteaddress_buffer(handle);
	uint4 data0 = buf.Load4(16 * 0);
	uint4 data1 = buf.Load4(16 * 1);
	uint4 data2 = buf.Load4(16 * 2);
	uint4 data3 = buf.Load4(16 * 3);
	uint4 data4 = buf.Load4(16 * 4);
	uint  data5 = buf.Load (16 * 5);

	float3 emission_color = r9g9b9e5_to_f32x3(data4.x);
	float3 subsurface_radius = r9g9b9e5_to_f32x3(data4.y);
	float4 sheen_coat_color = u8x4_unorm_to_f32x4(data4.z);
	float4 coat_specular_color = u8x4_unorm_to_f32x4(data4.w);
	float4 specular_diffuse_color = u8x4_unorm_to_f32x4(data5.x);
	float3 sheen_color = float3(sheen_coat_color.xyz);
	float3 coat_color0 = float3(sheen_coat_color.w, coat_specular_color.xy);
	float3 specular_color0 = float3(coat_specular_color.zw, specular_diffuse_color.x);
	float3 diffuse_color = float3(specular_diffuse_color.yzw);

	float3 coat_normal = normal;
	if((data0.y & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float3> tex = get_texture2d<float3>(data0.y & 0x00ffffff);
		float3 local_normal = tex.SampleLevel(RT_SAMPLER, uv, lod);
		local_normal.xy = 2 * local_normal.xy - 1;
		coat_normal = normalize(tangent * local_normal.x + binormal * local_normal.y + normal * local_normal.z);
	}

	float3 base_normal = normal;
	if((data0.z & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float3> tex = get_texture2d<float3>(data0.z & 0x00ffffff);
		float3 local_normal = tex.SampleLevel(RT_SAMPLER, uv, lod);
		local_normal.xy = 2 * local_normal.xy - 1;
		base_normal = normalize(tangent * local_normal.x + binormal * local_normal.y + normal * local_normal.z);
	}

	if((data0.w & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float3> tex = get_texture2d<float3>(data0.w & 0x00ffffff);
		sheen_color *= tex.SampleLevel(RT_SAMPLER, uv, lod);
	}

	float sheen_roughness = u8_unorm_to_f32(data1.x >> 24);
	if((data1.x & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float> tex = get_texture2d<float>(data1.x & 0x00ffffff);
		sheen_roughness *= tex.SampleLevel(RT_SAMPLER, uv, lod);
	}

	float coat_scale = u8_unorm_to_f32(data1.y >> 24);
	if((data1.y & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float> tex = get_texture2d<float>(data1.y & 0x00ffffff);
		coat_scale *= tex.SampleLevel(RT_SAMPLER, uv, lod);
	}

	if((data1.z & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float3> tex = get_texture2d<float3>(data1.z & 0x00ffffff);
		coat_color0 *= tex.SampleLevel(RT_SAMPLER, uv, lod);
	}

	float coat_roughness = u8_unorm_to_f32(data1.w >> 24);
	if((data1.w & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float> tex = get_texture2d<float>(data1.w & 0x00ffffff);
		coat_roughness *= tex.SampleLevel(RT_SAMPLER, uv, lod);
	}

	emission_color *= u8_unorm_to_f32(data2.x >> 24);
	if((data2.x & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float> tex = get_texture2d<float>(data2.x & 0x00ffffff);
		emission_color *= tex.SampleLevel(RT_SAMPLER, uv, lod);
	}

	if((data2.y & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float3> tex = get_texture2d<float3>(data2.y & 0x00ffffff);
		emission_color *= tex.SampleLevel(RT_SAMPLER, uv, lod);
	}

	float specular_scale = u8_unorm_to_f32(data2.z >> 24);
	if((data2.z & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float> tex = get_texture2d<float>(data2.z & 0x00ffffff);
		specular_scale *= tex.SampleLevel(RT_SAMPLER, uv, lod);
	}

	if((data2.w & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float3> tex = get_texture2d<float3>(data2.w & 0x00ffffff);
		specular_color0 *= tex.SampleLevel(RT_SAMPLER, uv, lod);
	}

	float specular_roughness = u8_unorm_to_f32(data3.x >> 24);
	if((data3.x & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float> tex = get_texture2d<float>(data3.x & 0x00ffffff);
		specular_roughness *= tex.SampleLevel(RT_SAMPLER, uv, lod);
	}

	if((data3.y & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float3> tex = get_texture2d<float3> (data3.y & 0x00ffffff);
		diffuse_color *= tex.SampleLevel(RT_SAMPLER, uv, lod);
	}
	
	float diffuse_roughness = u8_unorm_to_f32(data3.z >> 24);
	if((data3.z & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float> tex = get_texture2d<float>(data3.z & 0x00ffffff);
		diffuse_roughness *= tex.SampleLevel(RT_SAMPLER, uv, lod);
	}

	float subsurface = u8_unorm_to_f32(data3.w >> 24);
	if((data3.w & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float> tex = get_texture2d<float>(data3.w & 0x00ffffff);
		subsurface *= tex.SampleLevel(RT_SAMPLER, uv, lod);
	}

	float3 sheen_reflectance = 0;
	if(any(sheen_color > 0))
	{
		float3 sheen_tangent, sheen_binormal;
		sheen::calc_orthonormal_basis(wo, normal, sheen_tangent, sheen_binormal);
		float3 sheen_wo = float3(dot(sheen_tangent, wo), dot(sheen_binormal, wo), dot(normal, wo));
		if(sheen_wo.z > cosine_threshold){ sheen_reflectance = sheen::calc_reflectance(sheen_wo, sheen_color, sheen_roughness); }
	}

	float3 coat_reflectance = 0;
	if(coat_scale > 0)
	{
		float3 coat_tangent, coat_binormal;
		calc_orthonormal_basis(coat_normal, coat_tangent, coat_binormal);
		float3 coat_wo = float3(dot(coat_tangent, wo), dot(coat_binormal, wo), dot(coat_normal, wo));
		if(coat_wo.z > cosine_threshold){ coat_reflectance = coat_scale * microfacet::calc_reflectance(coat_wo, coat_color0, coat_roughness); }
	}

	float3 matte_reflectance = 0;
	float3 diffuse_reflectance = 0;
	float3 specular_reflectance = 0;
	if((specular_scale > 0) || any(diffuse_color > 0))
	{
		float3 base_tangent, base_binormal;
		calc_orthonormal_basis(base_normal, base_tangent, base_binormal);
		float3 base_wo = float3(dot(base_tangent, wo), dot(base_binormal, wo), dot(base_normal, wo));
		if(base_wo.z > cosine_threshold)
		{
			if(specular_scale > 0)
			{
				specular_reflectance = specular_scale * microfacet::calc_reflectance(base_wo, specular_color0, specular_roughness);
				matte_reflectance = specular_scale * microfacet::calc_matte_reflectance(base_wo, specular_color0, specular_roughness);
			}
			if(any(diffuse_color > 0))
				diffuse_reflectance = oren_nayer::calc_reflectance(base_wo, diffuse_color, diffuse_roughness);
		}
	}

	standard_material mtl;

#if defined(COMPRESS_MATERIAL)
	
	mtl.coat_normal = f32x2_to_u16x2_unorm(f32x3_to_oct(coat_normal) * 0.5f + 0.5f);
	mtl.base_normal = f32x2_to_u16x2_unorm(f32x3_to_oct(base_normal) * 0.5f + 0.5f);
	mtl.sheen_color = f32x4_to_r10g10b10a2(float4(sheen_color, 0));
	mtl.sheen_reflectance = f32x4_to_r10g10b10a2(float4(sheen_reflectance, 0));
	mtl.coat_color0 = f32x4_to_r10g10b10a2(float4(coat_color0, 0));
	mtl.coat_reflectance = f32x4_to_r10g10b10a2(float4(coat_reflectance, 0));
	mtl.emission_color = f32x3_to_r9g9b9e5(emission_color);
	mtl.specular_color0 = f32x4_to_r10g10b10a2(float4(specular_color0, 0));
	mtl.specular_reflectance = f32x4_to_r10g10b10a2(float4(specular_reflectance, 0));
	mtl.matte_reflectance = f32x4_to_r10g10b10a2(float4(matte_reflectance, 0));
	mtl.diffuse_color = f32x4_to_r10g10b10a2(float4(diffuse_color, 0));
	mtl.diffuse_reflectance = f32x4_to_r10g10b10a2(float4(diffuse_reflectance, 0));
	mtl.subsurface_radius = f32x3_to_r9g9b9e5(subsurface_radius);
	mtl.roughness = f32x4_to_r10g10b10a2(float4(sheen_roughness, coat_roughness, specular_roughness, 0));
	mtl.roughness_subsurface_misc = f32x4_to_r10g10b10a2(float4(diffuse_roughness, subsurface, 0, 0));
	mtl.scale = f32x4_to_r10g10b10a2(float4(coat_scale, specular_scale, 0, 0));

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
	mtl.emission_color = emission_color;
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

bool has_contribution(float nwi, standard_material mtl)
{
	return (nwi > cosine_threshold);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

float3 calc_emissive(float3 wo, float3 normal, standard_material mtl)
{
	float3 tangent, binormal;
	calc_orthonormal_basis(normal, tangent, binormal);

	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

float4 calc_bsdf_pdf(float3 wo, float3 wi, float3 normal, standard_material mtl, uint2 dtid = 0)
{
	float4 bsdf_pdf = 0;
	float3 throughput = 1;
	float sum_weight = 0;

	float3 sheen_color = get_sheen_color(mtl);
	if(any(sheen_color > 0))
	{
		float3 sheen_tangent, sheen_binormal;
		sheen::calc_orthonormal_basis(wo, normal, sheen_tangent, sheen_binormal);
		float3 sheen_wo = float3(dot(sheen_tangent, wo), dot(sheen_binormal, wo), dot(normal, wo));
		float3 sheen_wi = float3(dot(sheen_tangent, wi), dot(sheen_binormal, wi), dot(normal, wi));

		if((sheen_wo.z > cosine_threshold) && (sheen_wi.z > cosine_threshold))
		{
			float3 reflectance = get_sheen_reflectance(mtl);
			float weight = luminance(reflectance);
			sum_weight += weight;

			float4 brdf_pdf = sheen::calc_brdf_pdf(sheen_wo, sheen_wi, get_sheen_roughness(mtl));
			brdf_pdf.xyz *= reflectance;
			bsdf_pdf += float4(brdf_pdf.xyz, brdf_pdf.w * weight);
			//bsdf_pdf += float4(0, 0, 0, brdf_pdf.w * weight);

			throughput *= 1 - reflectance;
			if(all(throughput == 0))
				return float4(bsdf_pdf.xyz, bsdf_pdf.w / sum_weight);
		}
	}

	float coat_scale = get_coat_scale(mtl);
	if(coat_scale > 0)
	{
		float3 coat_tangent, coat_binormal;
		float3 coat_normal = get_coat_normal(mtl);
		calc_orthonormal_basis(coat_normal, coat_tangent, coat_binormal);
		float3 coat_wo = float3(dot(coat_tangent, wo), dot(coat_binormal, wo), dot(coat_normal, wo));
		float3 coat_wi = float3(dot(coat_tangent, wi), dot(coat_binormal, wi), dot(coat_normal, wi));
	
		if((coat_wo.z > 0) && (coat_wi.z > 0))
		{
			float3 reflectance = get_coat_reflectance(mtl);
			float weight = luminance(reflectance * throughput);
			sum_weight += weight;
	
			float4 brdf_pdf = microfacet::calc_brdf_pdf(coat_wo, coat_wi, get_coat_color0(mtl), get_coat_roughness(mtl));
			bsdf_pdf += float4(coat_scale * brdf_pdf.xyz * throughput, brdf_pdf.w * weight);
			//bsdf_pdf += float4(0, 0, 0, brdf_pdf.w * weight);

			throughput *= 1 - reflectance;
			if(all(throughput == 0))
				return float4(bsdf_pdf.xyz, bsdf_pdf.w / sum_weight);
		}
	}

	float3 base_tangent, base_binormal;
	float3 base_normal = get_base_normal(mtl);
	calc_orthonormal_basis(base_normal, base_tangent, base_binormal);
	float3 base_wo = float3(dot(base_tangent, wo), dot(base_binormal, wo), dot(base_normal, wo));
	float3 base_wi = float3(dot(base_tangent, wi), dot(base_binormal, wi), dot(base_normal, wi));

	float specular_scale = get_specular_scale(mtl);
	if(specular_scale > 0)
	{
		float3 reflectance = get_specular_reflectance(mtl);
		float weight = specular_scale * luminance(throughput * reflectance);
		sum_weight += weight;

		float roughness = get_specular_roughness(mtl);
		float4 brdf_pdf = microfacet::calc_brdf_pdf(base_wo, base_wi, get_specular_color0(mtl), roughness);
		bsdf_pdf += float4(specular_scale * brdf_pdf.xyz * throughput, brdf_pdf.w * weight);
		//bsdf_pdf += float4(0, 0, 0, brdf_pdf.w * weight);

		float3 matte_reflectance = get_matte_reflectance(mtl);
		weight = specular_scale * luminance(throughput * matte_reflectance);
		sum_weight += weight;

		brdf_pdf = microfacet::calc_matte_brdf_pdf(base_wo, base_wi, matte_reflectance, roughness);
		bsdf_pdf += float4(specular_scale * brdf_pdf.xyz * throughput, brdf_pdf.w * weight);
		//bsdf_pdf += float4(0, 0, 0, brdf_pdf.w * weight);

		throughput *= 1 - (reflectance + matte_reflectance);
		if(all(throughput == 0))
			return float4(bsdf_pdf.xyz, bsdf_pdf.w / sum_weight);
	}

	float3 diffuse_color = get_diffuse_color(mtl);
	if(any(diffuse_color > 0))
	{
		float3 reflectance = get_diffuse_reflectance(mtl);
		float weight = luminance(throughput * reflectance);
		sum_weight += weight;

		float4 brdf_pdf = oren_nayer::calc_brdf_pdf(base_wo, base_wi, get_diffuse_color(mtl), get_diffuse_roughness(mtl));
		bsdf_pdf += float4(brdf_pdf.xyz * throughput, brdf_pdf.w * weight);
		//bsdf_pdf += float4(0, 0, 0, brdf_pdf.w * weight);
	}

	return float4(bsdf_pdf.xyz, bsdf_pdf.w / sum_weight);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

bsdf_sample sample_bsdf(float3 wo, float3 normal, standard_material mtl, float u0, float u1, float u2, uint2 dtid = 0)
{
	float3 throughput = 1;
	
	int sample_type;
	float sum_weight = 0;
	float sample_weight;
	
	float3 sheen_reflectance = get_sheen_reflectance(mtl);
	if(any(sheen_reflectance) > 0)
	{
		float weight = luminance(sheen_reflectance);
		sum_weight += weight;

		float pmf = weight / sum_weight;
		if(u0 < pmf)
		{
			u0 /= pmf;
			sample_type = 0;
			sample_weight = weight;
		}
		else
		{
			u0 = (1 - u0) / (1 - pmf);
		}
		throughput *= 1 - sheen_reflectance;
	}

	float3 coat_reflectance = get_coat_reflectance(mtl);
	if(any(coat_reflectance > 0))
	{
		float weight = luminance(throughput * coat_reflectance);
		sum_weight += weight;
	
		float pmf = weight / sum_weight;
		if(u0 < pmf)
		{
			u0 /= pmf;
			sample_type = 1;
			sample_weight = weight;
		}
		else
		{
			u0 = (1 - u0) / (1 - pmf);
		}
		throughput *= 1 - coat_reflectance;
	}

	float3 specular_reflectance = get_specular_reflectance(mtl);
	if(any(specular_reflectance > 0))
	{
		float weight = luminance(throughput * specular_reflectance);
		sum_weight += weight;
			
		float pmf = weight / sum_weight;
		if(u0 < pmf)
		{
			u0 /= pmf;
			sample_type = 2;
			sample_weight = weight;
		}
		else
		{
			u0 = (1 - u0) / (1 - pmf);
		}
	
		float3 matte_reflectance = get_matte_reflectance(mtl);
		weight = luminance(throughput * matte_reflectance);
		sum_weight += weight;
			
		pmf = weight / sum_weight;
		if(u0 < pmf)
		{
			u0 /= pmf;
			sample_type = 3;
			sample_weight = weight;
		}
		else
		{
			u0 = (1 - u0) / (1 - pmf);
		}
		throughput *= 1 - (specular_reflectance + matte_reflectance);
	}

	float3 diffuse_reflectance = get_diffuse_reflectance(mtl);
	if(any(diffuse_reflectance > 0))
	{
		float weight = luminance(throughput * diffuse_reflectance);
		sum_weight += weight;

		float pmf = weight / sum_weight;
		if(u0 < pmf)
		{
			u0 /= pmf;
			sample_type = 4;
			sample_weight = weight;
		}
		else
		{
			u0 = (1 - u0) / (1 - pmf);
		}
	}
	
	bsdf_sample s;
	s.is_valid = false;
	if(sum_weight == 0)
		return s;
	
	float3 sample_normal;
	float3 sample_tangent;
	float3 sample_binormal;

	//sheen
	if(sample_type == 0)
	{
		sample_normal = normal;
		sheen::calc_orthonormal_basis(wo, sample_normal, sample_tangent, sample_binormal);
		float3 sheen_wo = float3(dot(sample_tangent, wo), dot(sample_binormal, wo), dot(sample_normal, wo));

		s.is_valid = sheen::sample_brdf(sheen_wo, s.wi, get_sheen_roughness(mtl), u0, u1);
		if(!s.is_valid)
			return s;
	}
	//coat
	else if(sample_type == 1)
	{
		sample_normal = get_coat_normal(mtl);
		calc_orthonormal_basis(sample_normal, sample_tangent, sample_binormal);
		float3 coat_wo = float3(dot(sample_tangent, wo), dot(sample_binormal, wo), dot(sample_normal, wo));

		s.is_valid = microfacet::sample_brdf(coat_wo, s.wi, get_coat_color0(mtl), get_coat_roughness(mtl), u1, u2);
		if(!s.is_valid)
			return s;
	}
	//base
	else
	{
		sample_normal = get_base_normal(mtl);
		calc_orthonormal_basis(sample_normal, sample_tangent, sample_binormal);
		float3 base_wo = float3(dot(sample_tangent, wo), dot(sample_binormal, wo), dot(sample_normal, wo));

		//specular
		if(sample_type == 2)
		{
			s.is_valid = microfacet::sample_brdf(base_wo, s.wi, get_specular_color0(mtl), get_specular_roughness(mtl), u1, u2);
		}
		//matte
		else if(sample_type == 3)
		{
			s.is_valid = microfacet::sample_matte_brdf(base_wo, s.wi, get_matte_reflectance(mtl), get_specular_roughness(mtl), u1, u2);
		}
		//diffuse
		else
		{
			s.is_valid = oren_nayer::sample_brdf(base_wo, s.wi, get_diffuse_color(mtl), get_diffuse_roughness(mtl), u1, u2);
		}
		if(!s.is_valid)
			return s;
	}

	s.wi = sample_tangent * s.wi.x + sample_binormal * s.wi.y + sample_normal * s.wi.z;

	if(!has_contribution(dot(normal, s.wi), mtl))
	{
		s.is_valid = false;
		return s;
	}
	
	float4 bsdf_pdf = calc_bsdf_pdf(wo, s.wi, normal, mtl, dtid);
	if(bsdf_pdf.w <= 0) //Ç»ÇÒÇ≈Ç¢ÇÈÇÒÇ‚Ç¡ÇØÅH
	{
		s.is_valid = false;
		return s;
	}

	s.weight = bsdf_pdf.xyz / bsdf_pdf.w;
	s.pdf = bsdf_pdf.w;
	return s;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

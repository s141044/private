
#ifndef MATERIAL_EYEBALL_HLSL
#define MATERIAL_EYEBALL_HLSL

#include"common.hlsl"
#include"../debug.hlsl"
#include"../packing.hlsl"
#include"../intersection.hlsl"
#include"../static_sampler.hlsl"

#include"../bsdf/microfacet.hlsl"
#include"../bsdf/oren_nayer.hlsl"

#include"../debug.hlsl"

#define COMPRESS_MATERIAL

///////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(COMPRESS_MATERIAL)

struct eyeball_material
{
	uint	coat_normal;
	uint	base_normal;
	uint	coat_color0;
	uint	coat_reflectance;
	uint	emissive_color;
	uint	specular_color0;
	uint	specular_reflectance;
	uint	diffuse_color;
	uint	diffuse_reflectance;
	uint	subsurface_radius;
	uint	roughness; //coat,specular
	uint	roughness_subsurface_misc; //diffuse,subsurface
	uint	scale; //coat,specular
};

float3	get_coat_normal(eyeball_material mtl){ return oct_to_f32x3(u16x2_unorm_to_f32x2(mtl.coat_normal) * 2 - 1); }
float3	get_base_normal(eyeball_material mtl){ return oct_to_f32x3(u16x2_unorm_to_f32x2(mtl.base_normal) * 2 - 1); }
float	get_coat_scale(eyeball_material mtl){ return r10g10b10a2_to_f32x4(mtl.scale).x; }
float3	get_coat_color0(eyeball_material mtl){ return r10g10b10a2_to_f32x4(mtl.coat_color0).xyz; }
float	get_coat_roughness(eyeball_material mtl){ return r10g10b10a2_to_f32x4(mtl.roughness).x; }
float3	get_coat_reflectance(eyeball_material mtl){ return r10g10b10a2_to_f32x4(mtl.coat_reflectance).xyz; }
float	get_specular_scale(eyeball_material mtl){ return r10g10b10a2_to_f32x4(mtl.scale).y; }
float3	get_specular_color0(eyeball_material mtl){ return r10g10b10a2_to_f32x4(mtl.specular_color0).xyz; }
float	get_specular_roughness(eyeball_material mtl){ return r10g10b10a2_to_f32x4(mtl.roughness).y; }
float3	get_specular_reflectance(eyeball_material mtl){ return r10g10b10a2_to_f32x4(mtl.specular_reflectance).xyz; }
float3	get_diffuse_color(eyeball_material mtl){ return r10g10b10a2_to_f32x4(mtl.diffuse_color).xyz; }
float	get_diffuse_roughness(eyeball_material mtl){ return r10g10b10a2_to_f32x4(mtl.roughness_subsurface_misc).x; }
float3	get_diffuse_reflectance(eyeball_material mtl){ return r10g10b10a2_to_f32x4(mtl.diffuse_reflectance).xyz; }
float	get_subsurface(eyeball_material mtl){ return r10g10b10a2_to_f32x4(mtl.roughness_subsurface_misc).y; }
float3	get_subsurface_radius(eyeball_material mtl){ return r9g9b9e5_to_f32x3(mtl.subsurface_radius); }
bool	is_twoside(eyeball_material mtl){ return false; }

#else

struct eyeball_material
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
	float3	emissive_color;
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
	uint	flags;
};

float3	get_coat_normal(eyeball_material mtl){ return mtl.coat_normal; }
float3	get_base_normal(eyeball_material mtl){ return mtl.base_normal; }
float3	get_sheen_color(eyeball_material mtl){ return mtl.sheen_color; }
float	get_sheen_roughness(eyeball_material mtl){ return mtl.sheen_roughness; }
float3	get_sheen_reflectance(eyeball_material mtl){ return mtl.sheen_reflectance; }
float	get_coat_scale(eyeball_material mtl){ return mtl.coat_scale; }
float3	get_coat_color0(eyeball_material mtl){ return mtl.coat_color0; }
float	get_coat_roughness(eyeball_material mtl){ return mtl.coat_roughness; }
float3	get_coat_reflectance(eyeball_material mtl){ return mtl.coat_reflectance; }
float3	get_emissive_color(eyeball_material mtl){ return mtl.emissive_color; }
float	get_specular_scale(eyeball_material mtl){ return mtl.specular_scale; }
float3	get_specular_color0(eyeball_material mtl){ return mtl.specular_color0; }
float	get_specular_roughness(eyeball_material mtl){ return mtl.specular_roughness; }
float3	get_specular_reflectance(eyeball_material mtl){ return mtl.specular_reflectance; }
float3	get_matte_reflectance(eyeball_material mtl){ return mtl.matte_reflectance; }
float3	get_diffuse_color(eyeball_material mtl){ return mtl.diffuse_color; }
float	get_diffuse_roughness(eyeball_material mtl){ return mtl.diffuse_roughness; }
float3	get_diffuse_reflectance(eyeball_material mtl){ return mtl.diffuse_reflectance; }
float	get_subsurface(eyeball_material mtl){ return mtl.subsurface; }
float3	get_subsurface_radius(eyeball_material mtl){ return mtl.subsurface_radius; }
bool	is_twoside(eyeball_material mtl){ return mtl.flags & 1; }

#endif

///////////////////////////////////////////////////////////////////////////////////////////////////

struct eyeball_material_host
{
	//0
	uint	height_map;
	uint	coat_normal_map;
	uint	base_normal_map;
	uint	coat_scale_map;
	//1
	uint	coat_color0_map;
	uint	coat_roughness_map;
	uint	specular_scale_map;
	uint	specular_color0_map;
	//2
	uint	specular_roughness_map;
	uint	diffuse_color_map;
	uint	diffuse_roughness_map;
	uint	subsurface_map;
	//3
	uint	subsurface_radius;
	uint	data0; //height_scale, coat_color0.xy
	uint	data1; //coat_color0.z, specular_color0.xyz
	uint	data2; //diffuse_color.xyz, -
	//4
	uint	uv_offset;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

eyeball_material load_eyeball_material(intersection isect, float3 wo, float lod = 0, uint2 dtid = 0)
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
	uint  data4 = buf.Load (MATERIAL_HEADER_SIZE + 16 * 4);

	float3 subsurface_radius = r9g9b9e5_to_f32x3(data3.x);
	float4 tmp0 = u8x4_unorm_to_f32x4(data3.y);
	float4 tmp1 = u8x4_unorm_to_f32x4(data3.z);
	float4 tmp2 = u8x4_unorm_to_f32x4(data3.w);
	float3 coat_color0 = float3(tmp0.zw, tmp1.x);
	float3 specular_color0 = float3(tmp1.yzw);
	//float3 diffuse_color = float3(tmp2.xyz);
	float3 diffuse_color = float3(tmp2.xyz) * 1.5;

	//uv.x += f16tof32(data4);
	//uv.y += f16tof32(data4 >> 16);

	float3 coat_normal = normal;
	if((data0.y & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float3> tex = get_texture2d<float3>(data0.y & 0x00ffffff);
		float3 local_normal = tex.SampleLevel(RT_SAMPLER, uv, lod);
		local_normal.xy = 2 * local_normal.xy - 1;
		coat_normal = normalize(tangent * local_normal.x + binormal * local_normal.y + normal * local_normal.z);
	}

	float coat_scale = u8_unorm_to_f32(data0.w >> 24);
	if((data0.w & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float> tex = get_texture2d<float>(data0.w & 0x00ffffff);
		coat_scale *= tex.SampleLevel(RT_SAMPLER, uv, lod);
	}

	if((data1.x & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float3> tex = get_texture2d<float3>(data1.x & 0x00ffffff);
		coat_color0 *= tex.SampleLevel(RT_SAMPLER, uv, lod);
	}

	float coat_roughness = u8_unorm_to_f32(data1.y >> 24);
	if((data1.y & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float> tex = get_texture2d<float>(data1.y & 0x00ffffff);
		coat_roughness *= tex.SampleLevel(RT_SAMPLER, uv, lod);
	}

	//parallax
	if((data0.x & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float> tex = get_texture2d<float>(data0.x & 0x00ffffff);
		float scale = f16tof32(data3.y);

		float3 wt = refract(-wo, isect.normal, 1 / 1.376);
		wt = float3(dot(wt, tangent), dot(wt, binormal), dot(wt, normal));
		wt.z = -wt.z;

		bool fallback = true;
		//float prev_depth = scale;
		//for(uint i = 0; i < 32; i++)
		//{
		//	float depth = (1 - tex.SampleLevel(RT_SAMPLER, uv, lod)) * scale;
		//
		//	float3 pos = wt * (0.005 / wt.z) * i;
		//	if(pos.z >= depth)
		//	{
		//		uv += pos.xy;
		//		fallback = false;
		//		break;
		//	}
		//}

		if(fallback)
		{
			float depth = (1 - tex.SampleLevel(RT_SAMPLER, uv, lod));
			uv += wt.xy / wt.z * (depth * scale);
		}
	}

	float3 base_normal = normal;
	if((data0.z & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float3> tex = get_texture2d<float3>(data0.z & 0x00ffffff);
		float3 local_normal = tex.SampleLevel(RT_SAMPLER, uv, lod);
		local_normal.xy = 2 * local_normal.xy - 1;
		base_normal = normalize(tangent * local_normal.x + binormal * local_normal.y + normal * local_normal.z);
	}

	float specular_scale = u8_unorm_to_f32(data1.z >> 24);
	if((data1.z & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float> tex = get_texture2d<float>(data1.z & 0x00ffffff);
		specular_scale *= tex.SampleLevel(RT_SAMPLER, uv, lod);
	}

	if((data1.w & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float3> tex = get_texture2d<float3>(data1.w & 0x00ffffff);
		specular_color0 *= tex.SampleLevel(RT_SAMPLER, uv, lod);
	}

	float specular_roughness = u8_unorm_to_f32(data2.x >> 24);
	if((data2.x & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float> tex = get_texture2d<float>(data2.x & 0x00ffffff);
		specular_roughness *= tex.SampleLevel(RT_SAMPLER, uv, lod);
	}

	if((data2.y & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float3> tex = get_texture2d<float3> (data2.y & 0x00ffffff);
		diffuse_color *= tex.SampleLevel(RT_SAMPLER, uv, lod);
	}
	
	float diffuse_roughness = u8_unorm_to_f32(data2.z >> 24);
	if((data2.z & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float> tex = get_texture2d<float>(data2.z & 0x00ffffff);
		diffuse_roughness *= tex.SampleLevel(RT_SAMPLER, uv, lod);
	}

	float subsurface = u8_unorm_to_f32(data2.w >> 24);
	if((data2.w & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float> tex = get_texture2d<float>(data2.w & 0x00ffffff);
		subsurface *= tex.SampleLevel(RT_SAMPLER, uv, lod);
	}

	float3 coat_reflectance = 0;
	if(coat_scale > 0)
	{
		float3 coat_tangent, coat_binormal;
		calc_orthonormal_basis(coat_normal, coat_tangent, coat_binormal);
		float3 coat_wo = float3(dot(coat_tangent, wo), dot(coat_binormal, wo), dot(coat_normal, wo));
		if(coat_wo.z > cosine_threshold){ coat_reflectance = coat_scale * microfacet::calc_reflectance(coat_wo, coat_color0, coat_roughness); }
	}

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
				specular_reflectance = specular_scale * microfacet::calc_reflectance(base_wo, specular_color0, specular_roughness);
			if(any(diffuse_color > 0))
				diffuse_reflectance = oren_nayer::calc_reflectance(base_wo, diffuse_color, diffuse_roughness);
		}
	}

	eyeball_material mtl;

#if defined(COMPRESS_MATERIAL)
	
	mtl.coat_normal = f32x2_to_u16x2_unorm(f32x3_to_oct(coat_normal) * 0.5f + 0.5f);
	mtl.base_normal = f32x2_to_u16x2_unorm(f32x3_to_oct(base_normal) * 0.5f + 0.5f);
	mtl.coat_color0 = f32x4_to_r10g10b10a2(float4(coat_color0, 0));
	mtl.coat_reflectance = f32x4_to_r10g10b10a2(float4(coat_reflectance, 0));
	mtl.specular_color0 = f32x4_to_r10g10b10a2(float4(specular_color0, 0));
	mtl.specular_reflectance = f32x4_to_r10g10b10a2(float4(specular_reflectance, 0));
	mtl.diffuse_color = f32x4_to_r10g10b10a2(float4(diffuse_color, 0));
	mtl.diffuse_reflectance = f32x4_to_r10g10b10a2(float4(diffuse_reflectance, 0));
	mtl.subsurface_radius = f32x3_to_r9g9b9e5(subsurface_radius);
	mtl.roughness = f32x4_to_r10g10b10a2(float4(coat_roughness, specular_roughness, 0, 0));
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

bool has_contribution(float nwi, eyeball_material mtl)
{
	if(nwi > cosine_threshold)
		return true;
	else
		return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void calc_bsdf_pdf(float3 wo, float3 wi, float3 normal, eyeball_material mtl, out float3 diffuse, out float3 non_diffuse, out float pdf, bool is_sss, uint2 dtid = 0)
{
	pdf = 0;
	diffuse = 0;
	non_diffuse = 0;

	float3 throughput = 1;
	float sum_weight = 1e-10;

	float3 coat_reflectance = get_coat_reflectance(mtl);
	if(any(coat_reflectance > 0))
	{
		float weight = luminance(coat_reflectance * throughput);
		sum_weight += weight;
	
		float3 coat_tangent, coat_binormal;
		float3 coat_normal = get_coat_normal(mtl);
		calc_orthonormal_basis(coat_normal, coat_tangent, coat_binormal);
		float3 coat_wo = float3(dot(coat_tangent, wo), dot(coat_binormal, wo), dot(coat_normal, wo));
		float3 coat_wi = float3(dot(coat_tangent, wi), dot(coat_binormal, wi), dot(coat_normal, wi));
	
		if(!is_sss && (coat_wi.z > cosine_threshold))
		{
			float4 brdf_pdf = microfacet::calc_brdf_pdf(coat_wo, coat_wi, get_coat_color0(mtl), get_coat_roughness(mtl));
			non_diffuse += get_coat_scale(mtl) * brdf_pdf.xyz * throughput;
			pdf += brdf_pdf.w * weight;
		}

		throughput *= 1 - coat_reflectance;
		if(all(throughput == 0))
		{
			pdf /= sum_weight;
			return;
		}
	}

	float3 base_tangent, base_binormal;
	float3 base_normal = get_base_normal(mtl);
	calc_orthonormal_basis(base_normal, base_tangent, base_binormal);
	float3 base_wo = float3(dot(base_tangent, wo), dot(base_binormal, wo), dot(base_normal, wo));
	float3 base_wi = float3(dot(base_tangent, wi), dot(base_binormal, wi), dot(base_normal, wi));

	float3 specular_reflectance = get_specular_reflectance(mtl);
	if(any(specular_reflectance) > 0)
	{
		float weight = luminance(throughput * specular_reflectance);
		sum_weight += weight;

		if(!is_sss && (base_wi.z > cosine_threshold))
		{
			float specular_scale = get_specular_scale(mtl);
			float roughness = get_specular_roughness(mtl);
			float4 brdf_pdf = microfacet::calc_brdf_pdf(base_wo, base_wi, get_specular_color0(mtl), roughness);
			non_diffuse += specular_scale * brdf_pdf.xyz * throughput;
			pdf += brdf_pdf.w * weight;
		}

		throughput *= 1 - specular_reflectance;
		if(all(throughput == 0))
		{
			pdf /= sum_weight;
			return;
		}
	}

	float3 diffuse_reflectance = get_diffuse_reflectance(mtl);
	if(any(diffuse_reflectance > 0))
	{
		float weight = luminance(throughput * diffuse_reflectance);
		sum_weight += weight;

		if(base_wi.z > cosine_threshold)
		{
			float4 brdf_pdf = oren_nayer::calc_brdf_pdf(base_wo, base_wi, get_diffuse_color(mtl), get_diffuse_roughness(mtl));
			if(is_sss)
				brdf_pdf *= get_subsurface(mtl);
			else
				brdf_pdf *= 1 - get_subsurface(mtl);
			
			diffuse += brdf_pdf.xyz * throughput;
			pdf += brdf_pdf.w * weight;
		}
	}

	pdf /= sum_weight;
}

float4 calc_bsdf_pdf(float3 wo, float3 wi, float3 normal, eyeball_material mtl, uint2 dtid = 0)
{
	float pdf;
	float3 diffuse, non_diffuse;
	calc_bsdf_pdf(wo, wi, normal, mtl, diffuse, non_diffuse, pdf, false, dtid);
	return float4(diffuse + non_diffuse, pdf);
}

float3 calc_subsurface(eyeball_material mtl)
{
	return 1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

bsdf_sample sample_bsdf(float3 wo, float3 normal, eyeball_material mtl, float u0, float u1, uint2 dtid = 0)
{
	float3 throughput = 1;
	
	int sample_type;
	float sum_weight = 0;
	
	float3 coat_reflectance = get_coat_reflectance(mtl);
	if(any(coat_reflectance > 0))
	{
		float weight = luminance(throughput * coat_reflectance);
		sum_weight += weight;
	
		float pmf = weight / sum_weight;
		if(u1 < pmf)
		{
			u1 /= pmf;
			sample_type = 0;
		}
		else
		{
			u1 = (1 - u1) / (1 - pmf);
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
			sample_type = 1;
		}
		else
		{
			u0 = (1 - u0) / (1 - pmf);
		}
		throughput *= 1 - specular_reflectance;
	}

	float3 diffuse_reflectance = get_diffuse_reflectance(mtl);
	if(any(diffuse_reflectance > 0))
	{
		float weight = luminance(throughput * diffuse_reflectance);
		sum_weight += weight;

		float pmf = weight / sum_weight;
		if(u1 < pmf)
		{
			u1 /= pmf;
			sample_type = 2;

			float subsurface = get_subsurface(mtl);
			if(u1 < subsurface)
			{
				u1 /= subsurface;
				sample_type = 3;
			}
			else
			{
				u1 = (1 - u1) / (1 - subsurface);
			}
		}
		else
		{
			u1 = (1 - u1) / (1 - pmf);
		}
	}
	
	bsdf_sample s;
	s.is_valid = false;
	if(sum_weight == 0)
		return s;
	
	float3 sample_normal;
	float3 sample_tangent;
	float3 sample_binormal;

	//coat
	if(sample_type == 0)
	{
		sample_normal = get_coat_normal(mtl);
		calc_orthonormal_basis(sample_normal, sample_tangent, sample_binormal);
		float3 coat_wo = float3(dot(sample_tangent, wo), dot(sample_binormal, wo), dot(sample_normal, wo));

		s.is_valid = microfacet::sample_brdf(coat_wo, s.w, get_coat_color0(mtl), get_coat_roughness(mtl), u0, u1);
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
		if(sample_type == 1)
		{
			s.is_valid = microfacet::sample_brdf(base_wo, s.w, get_specular_color0(mtl), get_specular_roughness(mtl), u0, u1);
		}
		//diffuse
		else
		{
			s.is_valid = oren_nayer::sample_brdf(base_wo, s.w, get_diffuse_color(mtl), get_diffuse_roughness(mtl), u0, u1);
		}
		if(!s.is_valid)
			return s;
	}

	s.w = sample_tangent * s.w.x + sample_binormal * s.w.y + sample_normal * s.w.z;

	float nwi = dot(normal, s.w);
	if(nwi < cosine_threshold)
	{
		s.is_valid = false;
		return s;
	}

	float3 diffuse, non_diffuse;
	bool is_sss = (sample_type == 3);
	calc_bsdf_pdf(wo, s.w, normal, mtl, diffuse, non_diffuse, s.pdf, is_sss, dtid);

	if(is_sss)
		s.w = -s.w;

	float inv_pdf = 1 / s.pdf;
	s.diffuse_weight = diffuse * inv_pdf;
	s.non_diffuse_weight = non_diffuse * inv_pdf;
	s.weight = (diffuse + non_diffuse) * inv_pdf;
	return s;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

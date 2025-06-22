
#ifndef MATERIAL_GLINT_SPECULAR_HLSL
#define MATERIAL_GLINT_SPECULAR_HLSL

#include"common.hlsl"
#include"../debug.hlsl"
#include"../packing.hlsl"
#include"../intersection.hlsl"
#include"../static_sampler.hlsl"

#include"../bsdf/glint.hlsl"
#include"../bsdf/sheen.hlsl"
#include"../bsdf/microfacet.hlsl"
#include"../bsdf/oren_nayer.hlsl"

#include"../debug.hlsl"

///////////////////////////////////////////////////////////////////////////////////////////////////

struct glint_specular_material
{
	uint	coat_normal;
	uint	base_normal;
	uint	sheen_color;
	uint	sheen_reflectance;
	uint	coat_color0;
	uint	coat_reflectance;
	uint	specular_color0;
	uint	specular_reflectance;
	uint	diffuse_color;
	uint	diffuse_reflectance;
	uint	subsurface_radius;
	uint	roughness; //sheen,coat,specular
	uint	roughness_subsurface_misc; //diffuse,subsurface
	uint	scale; //coat,specular
	uint	flags_glint_cell_size;

	float2	patch_center;
	float2	patch_axis0;
	float2	patch_axis1;
};

float3	get_coat_normal(glint_specular_material mtl){ return oct_to_f32x3(u16x2_unorm_to_f32x2(mtl.coat_normal) * 2 - 1); }
float3	get_base_normal(glint_specular_material mtl){ return oct_to_f32x3(u16x2_unorm_to_f32x2(mtl.base_normal) * 2 - 1); }
float3	get_sheen_color(glint_specular_material mtl){ return r10g10b10a2_to_f32x4(mtl.sheen_color).xyz; }
float	get_sheen_roughness(glint_specular_material mtl){ return r10g10b10a2_to_f32x4(mtl.roughness).x; }
float3	get_sheen_reflectance(glint_specular_material mtl){ return r10g10b10a2_to_f32x4(mtl.sheen_reflectance).xyz; }
float	get_coat_scale(glint_specular_material mtl){ return r10g10b10a2_to_f32x4(mtl.scale).x; }
float3	get_coat_color0(glint_specular_material mtl){ return r10g10b10a2_to_f32x4(mtl.coat_color0).xyz; }
float	get_coat_roughness(glint_specular_material mtl){ return r10g10b10a2_to_f32x4(mtl.roughness).y; }
float3	get_coat_reflectance(glint_specular_material mtl){ return r10g10b10a2_to_f32x4(mtl.coat_reflectance).xyz; }
float	get_specular_scale(glint_specular_material mtl){ return r10g10b10a2_to_f32x4(mtl.scale).y; }
float3	get_specular_color0(glint_specular_material mtl){ return r10g10b10a2_to_f32x4(mtl.specular_color0).xyz; }
float	get_specular_roughness(glint_specular_material mtl){ return r10g10b10a2_to_f32x4(mtl.roughness).z; }
float3	get_specular_reflectance(glint_specular_material mtl){ return r10g10b10a2_to_f32x4(mtl.specular_reflectance).xyz; }
float3	get_diffuse_color(glint_specular_material mtl){ return r10g10b10a2_to_f32x4(mtl.diffuse_color).xyz; }
float	get_diffuse_roughness(glint_specular_material mtl){ return r10g10b10a2_to_f32x4(mtl.roughness_subsurface_misc).x; }
float3	get_diffuse_reflectance(glint_specular_material mtl){ return r10g10b10a2_to_f32x4(mtl.diffuse_reflectance).xyz; }
float	get_subsurface(glint_specular_material mtl){ return r10g10b10a2_to_f32x4(mtl.roughness_subsurface_misc).y; }
float3	get_subsurface_radius(glint_specular_material mtl){ return r9g9b9e5_to_f32x3(mtl.subsurface_radius); }
bool	is_twoside(glint_specular_material mtl){ return mtl.flags_glint_cell_size & 1; }

float2	get_patch_center(glint_specular_material mtl) { return mtl.patch_center; }
float2	get_patch_axis0(glint_specular_material mtl){ return mtl.patch_axis0; }
float2	get_patch_axis1(glint_specular_material mtl){ return mtl.patch_axis1; }
float	get_lod_bias(glint_specular_material mtl){ return 2; }
float	get_cell_size(glint_specular_material mtl){ return f16tof32(mtl.flags_glint_cell_size >> 16); }

///////////////////////////////////////////////////////////////////////////////////////////////////

struct glint_specular_material_host
{
	//0
	uint	coat_normal_map;
	uint	base_normal_map;
	uint	sheen_color_map;
	uint	sheen_roughness_map;
	//1
	uint	coat_scale_map;
	uint	coat_color0_map;
	uint	coat_roughness_map;
	uint	specular_scale_map;
	//2
	uint	specular_color0_map;
	uint	specular_roughness_map;
	uint	diffuse_color_map;
	uint	diffuse_roughness_map;
	//3
	uint	subsurface_map;
	uint	subsurface_radius;
	uint	sheen_coat_color; //sheen.xyz,coat.x
	uint	coat_specular_color; //coat.yz,specular.xy
	//4
	uint	specular_diffuse_color; //specular.z,diffuse_xyz
	uint	flags_glint_cell_size; //two_side, 
};

///////////////////////////////////////////////////////////////////////////////////////////////////

glint_specular_material load_glint_specular_material(intersection isect, float3 wo, float lod = 0, uint2 dtid = 0)
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
	uint2 data4 = buf.Load2(MATERIAL_HEADER_SIZE + 16 * 4);

	float3 subsurface_radius = r9g9b9e5_to_f32x3(data3.y);
	float4 sheen_coat_color = u8x4_unorm_to_f32x4(data3.z);
	float4 coat_specular_color = u8x4_unorm_to_f32x4(data3.w);
	float4 specular_diffuse_color = u8x4_unorm_to_f32x4(data4.x);
	float3 sheen_color = float3(sheen_coat_color.xyz);
	float3 coat_color0 = float3(sheen_coat_color.w, coat_specular_color.xy);
	float3 specular_color0 = float3(coat_specular_color.zw, specular_diffuse_color.x);
	float3 diffuse_color = float3(specular_diffuse_color.yzw);

	float3 coat_normal = normal;
	if((data0.x & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float3> tex = get_texture2d<float3>(data0.x & 0x00ffffff);
		float3 local_normal = tex.SampleLevel(RT_SAMPLER, uv, lod);
		local_normal.xy = 2 * local_normal.xy - 1;
		coat_normal = normalize(tangent * local_normal.x + binormal * local_normal.y + normal * local_normal.z);
	}

	float3 base_normal = normal;
	if((data0.y & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float3> tex = get_texture2d<float3>(data0.y & 0x00ffffff);
		float3 local_normal = tex.SampleLevel(RT_SAMPLER, uv, lod);
		local_normal.xy = 2 * local_normal.xy - 1;
		base_normal = normalize(tangent * local_normal.x + binormal * local_normal.y + normal * local_normal.z);
	}

	if((data0.z & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float3> tex = get_texture2d<float3>(data0.z & 0x00ffffff);
		sheen_color *= tex.SampleLevel(RT_SAMPLER, uv, lod);
	}

	float sheen_roughness = u8_unorm_to_f32(data0.w >> 24);
	if((data0.w & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float> tex = get_texture2d<float>(data0.w & 0x00ffffff);
		sheen_roughness *= tex.SampleLevel(RT_SAMPLER, uv, lod);
	}

	float coat_scale = u8_unorm_to_f32(data1.x >> 24);
	if((data1.x & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float> tex = get_texture2d<float>(data1.x & 0x00ffffff);
		coat_scale *= tex.SampleLevel(RT_SAMPLER, uv, lod);
	}

	if((data1.y & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float3> tex = get_texture2d<float3>(data1.y & 0x00ffffff);
		coat_color0 *= tex.SampleLevel(RT_SAMPLER, uv, lod);
	}

	float coat_roughness = u8_unorm_to_f32(data1.z >> 24);
	if((data1.z & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float> tex = get_texture2d<float>(data1.z & 0x00ffffff);
		coat_roughness *= tex.SampleLevel(RT_SAMPLER, uv, lod);
	}

	float specular_scale = u8_unorm_to_f32(data1.w >> 24);
	if((data1.w & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float> tex = get_texture2d<float>(data1.w & 0x00ffffff);
		specular_scale *= tex.SampleLevel(RT_SAMPLER, uv, lod);
	}

	if((data2.x & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float3> tex = get_texture2d<float3>(data2.x & 0x00ffffff);
		specular_color0 *= tex.SampleLevel(RT_SAMPLER, uv, lod);
	}

	float specular_roughness = u8_unorm_to_f32(data2.y >> 24);
	if((data2.y & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float> tex = get_texture2d<float>(data2.y & 0x00ffffff);
		specular_roughness *= tex.SampleLevel(RT_SAMPLER, uv, lod);
	}

	if((data2.z & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float3> tex = get_texture2d<float3> (data2.z & 0x00ffffff);
		diffuse_color *= tex.SampleLevel(RT_SAMPLER, uv, lod);
	}
	
	float diffuse_roughness = u8_unorm_to_f32(data2.w >> 24);
	if((data2.w & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float> tex = get_texture2d<float>(data2.w & 0x00ffffff);
		diffuse_roughness *= tex.SampleLevel(RT_SAMPLER, uv, lod);
	}

	float subsurface = u8_unorm_to_f32(data3.x >> 24);
	if((data3.x & 0x00ffffff) != 0x00ffffff)
	{
		Texture2D<float> tex = get_texture2d<float>(data3.x & 0x00ffffff);
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

	float3 diffuse_reflectance = 0;
	float3 specular_reflectance = 0;
	if((specular_scale > 0) || any(diffuse_color > 0))
	{
		float3 base_tangent, base_binormal;
		calc_orthonormal_basis(base_normal, base_tangent, base_binormal);
		float3 base_wo = float3(dot(base_tangent, wo), dot(base_binormal, wo), dot(base_normal, wo));
		if(base_wo.z > cosine_threshold)
		{
			if (specular_scale > 0)
				specular_reflectance = specular_scale * glint::calc_reflectance(base_wo, specular_color0, specular_roughness);
			if(any(diffuse_color > 0))
				diffuse_reflectance = oren_nayer::calc_reflectance(base_wo, diffuse_color, diffuse_roughness);
		}
	}

	glint_specular_material mtl;
	mtl.flags_glint_cell_size = data4.y;

	float view_z = abs(mul(float4(position, 1), view_mat).z);
	float3 ray_o = camera_pos - position;
	float3 ray_d_px = normalize(+camera_axis_x * pixel_size * view_z - ray_o);
	float3 ray_d_nx = normalize(-camera_axis_x * pixel_size * view_z - ray_o);
	float3 ray_d_py = normalize(+camera_axis_y * pixel_size * view_z - ray_o);
	float3 ray_d_ny = normalize(-camera_axis_y * pixel_size * view_z - ray_o);
	float3 ray_d_x = (dot(ray_d_px, isect.geometry_normal) > dot(ray_d_nx, isect.geometry_normal)) ? ray_d_px : ray_d_nx; //範囲が大きくなりすぎない方にする
	float3 ray_d_y = (dot(ray_d_py, isect.geometry_normal) > dot(ray_d_ny, isect.geometry_normal)) ? ray_d_py : ray_d_ny;
	float ray_t_x = -dot(ray_o, isect.geometry_normal) / dot(ray_d_x, isect.geometry_normal);
	float ray_t_y = -dot(ray_o, isect.geometry_normal) / dot(ray_d_y, isect.geometry_normal);
	float3 p_x = ray_o + ray_d_x * ray_t_x;
	float3 p_y = ray_o + ray_d_y * ray_t_y;
	float2 uv_axis_x = float2(dot(p_x, isect.dpdu), dot(p_x, isect.dpdv));
	float2 uv_axis_y = float2(dot(p_y, isect.dpdu), dot(p_y, isect.dpdv));

	float2 uv_axis0;
	float2 uv_axis1;
	if(dot(uv_axis_x, uv_axis_x) > dot(uv_axis_y, uv_axis_y))
	{
		uv_axis0 = uv_axis_x;
		uv_axis1 = uv_axis_y - dot(uv_axis_y, uv_axis_x) * uv_axis_x;
	}
	else
	{
		uv_axis0 = uv_axis_y;
		uv_axis1 = uv_axis_x - dot(uv_axis_x, uv_axis_y) * uv_axis_y;
	}
	
	mtl.coat_normal = f32x2_to_u16x2_unorm(f32x3_to_oct(coat_normal) * 0.5 + 0.5);
	mtl.base_normal = f32x2_to_u16x2_unorm(f32x3_to_oct(base_normal) * 0.5 + 0.5);
	mtl.sheen_color = f32x4_to_r10g10b10a2(float4(sheen_color, 0));
	mtl.sheen_reflectance = f32x4_to_r10g10b10a2(float4(sheen_reflectance, 0));
	mtl.coat_color0 = f32x4_to_r10g10b10a2(float4(coat_color0, 0));
	mtl.coat_reflectance = f32x4_to_r10g10b10a2(float4(coat_reflectance, 0));
	mtl.specular_color0 = f32x4_to_r10g10b10a2(float4(specular_color0, 0));
	mtl.specular_reflectance = f32x4_to_r10g10b10a2(float4(specular_reflectance, 0));
	mtl.diffuse_color = f32x4_to_r10g10b10a2(float4(diffuse_color, 0));
	mtl.diffuse_reflectance = f32x4_to_r10g10b10a2(float4(diffuse_reflectance, 0));
	mtl.subsurface_radius = f32x3_to_r9g9b9e5(subsurface_radius);
	mtl.roughness = f32x4_to_r10g10b10a2(float4(sheen_roughness, coat_roughness, specular_roughness, 0));
	mtl.roughness_subsurface_misc = f32x4_to_r10g10b10a2(float4(diffuse_roughness, subsurface, 0, 0));
	mtl.scale = f32x4_to_r10g10b10a2(float4(coat_scale, specular_scale, 0, 0));

	mtl.patch_center = uv;
	mtl.patch_axis0 = uv_axis0 / 2;
	mtl.patch_axis1 = uv_axis1 / 2;

	return mtl;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

bool has_contribution(float nwi, glint_specular_material mtl)
{
	if(nwi > cosine_threshold)
		return true;
	else if(nwi < -cosine_threshold)
		return is_twoside(mtl) && (get_subsurface(mtl) > 0);
	else
		return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void calc_bsdf_pdf(float3 wo, float3 wi, float3 normal, glint_specular_material mtl, out float3 diffuse, out float3 non_diffuse, out float pdf, uint2 dtid = 0)
{
	pdf = 0;
	diffuse = 0;
	non_diffuse = 0;

	float nwi = dot(normal, wi);
	bool eval_brdf = (nwi > +cosine_threshold);
	bool eval_btdf = (nwi < -cosine_threshold);
	
	float3 throughput = 1;
	float sum_weight = 1e-10;
	
	float3 sheen_reflectance = get_sheen_reflectance(mtl);
	if(eval_brdf && any(sheen_reflectance > 0))
	{
		float weight = luminance(sheen_reflectance);
		sum_weight += weight;
	
		float3 sheen_tangent, sheen_binormal;
		sheen::calc_orthonormal_basis(wo, normal, sheen_tangent, sheen_binormal);
		float3 sheen_wo = float3(dot(sheen_tangent, wo), dot(sheen_binormal, wo), dot(normal, wo));
		float3 sheen_wi = float3(dot(sheen_tangent, wi), dot(sheen_binormal, wi), dot(normal, wi));
	
		if(sheen_wi.z > cosine_threshold)
		{
			float4 brdf_pdf = sheen::calc_brdf_pdf(sheen_wo, sheen_wi, get_sheen_roughness(mtl));
			brdf_pdf.xyz *= sheen_reflectance;
			non_diffuse += brdf_pdf.xyz;
			pdf += brdf_pdf.w * weight;
		}
	
		throughput *= 1 - sheen_reflectance;
		if(all(throughput == 0))
		{
			pdf /= sum_weight;
			return;
		}
	}
	
	float3 coat_reflectance = get_coat_reflectance(mtl);
	if(eval_brdf && any(coat_reflectance > 0))
	{
		float weight = luminance(coat_reflectance * throughput);
		sum_weight += weight;
	
		float3 coat_tangent, coat_binormal;
		float3 coat_normal = get_coat_normal(mtl);
		calc_orthonormal_basis(coat_normal, coat_tangent, coat_binormal);
		float3 coat_wo = float3(dot(coat_tangent, wo), dot(coat_binormal, wo), dot(coat_normal, wo));
		float3 coat_wi = float3(dot(coat_tangent, wi), dot(coat_binormal, wi), dot(coat_normal, wi));
	
		if(coat_wi.z > cosine_threshold)
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
	if(eval_brdf && any(specular_reflectance) > 0)
	{
		float weight = luminance(throughput * specular_reflectance);
		sum_weight += weight;

		if(base_wi.z > cosine_threshold)
		{
			float specular_scale = get_specular_scale(mtl);
			float roughness = get_specular_roughness(mtl);
			float4 brdf_pdf = glint::calc_brdf_pdf(base_wo, base_wi, get_specular_color0(mtl), get_specular_roughness(mtl), get_patch_center(mtl), get_patch_axis0(mtl), get_patch_axis1(mtl), get_cell_size(mtl), get_lod_bias(mtl));
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
	
		if((eval_brdf && (base_wi.z > cosine_threshold)) || (eval_btdf && (base_wi.z < -cosine_threshold))) //サンプリングから呼ぶときは片面でも計算してほしいので両面の判定はしない.NEEのときはhas_contributionで片面下半球をはじく
		//if((eval_brdf && (base_wi.z > cosine_threshold)) || (eval_btdf && (base_wi.z < -cosine_threshold) && is_twoside(mtl)))
		{
			float subsurface = get_subsurface(mtl);
			float3 diffuse_color = get_diffuse_color(mtl);
			if(eval_brdf && (base_wi.z > cosine_threshold))
			{
				diffuse_color *= (1 - subsurface);
			}
			else
			{
				diffuse_color *= subsurface;
				base_wi = -base_wi;
			}
	
			float4 brdf_pdf = oren_nayer::calc_brdf_pdf(base_wo, base_wi, diffuse_color, get_diffuse_roughness(mtl));
			diffuse += brdf_pdf.xyz * throughput;
			pdf += brdf_pdf.w * weight;
		}
	}
	
	pdf /= sum_weight;
}

float4 calc_bsdf_pdf(float3 wo, float3 wi, float3 normal, glint_specular_material mtl, uint2 dtid = 0)
{
	float pdf;
	float3 diffuse, non_diffuse;
	calc_bsdf_pdf(wo, wi, normal, mtl, diffuse, non_diffuse, pdf, dtid);
	return float4(diffuse + non_diffuse, pdf);
}

float3 calc_subsurface(glint_specular_material mtl)
{
	float3 throughput = 1;
	throughput *= 1 - get_sheen_reflectance(mtl);
	throughput *= 1 - get_coat_reflectance(mtl);
	throughput *= 1 - get_specular_reflectance(mtl);
	//return throughput * get_diffuse_reflectance(mtl) * get_subsurface(mtl); //アルベドが2重にかかると色がおかしくなる
	return throughput * get_subsurface(mtl);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

bsdf_sample sample_bsdf(float3 wo, float3 normal, glint_specular_material mtl, float u0, float u1, uint2 dtid = 0)
{
	float3 throughput = 1;
	
	int sample_type;
	float sum_weight = 0;
	
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
		if(u1 < pmf)
		{
			u1 /= pmf;
			sample_type = 1;
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
			sample_type = 2;
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
			sample_type = 4;
	
			float subsurface = get_subsurface(mtl);
			if(u1 < subsurface)
			{
				u1 /= subsurface;
				sample_type = 5;
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
	
	//sheen
	if(sample_type == 0)
	{
		sample_normal = normal;
		sheen::calc_orthonormal_basis(wo, sample_normal, sample_tangent, sample_binormal);
		float3 sheen_wo = float3(dot(sample_tangent, wo), dot(sample_binormal, wo), dot(sample_normal, wo));
	
		s.is_valid = sheen::sample_brdf(sheen_wo, s.w, get_sheen_roughness(mtl), u0, u1);
		if(!s.is_valid)
			return s;
	}
	//coat
	else if(sample_type == 1)
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
		if(sample_type == 2)
		{
			s.is_valid = glint::sample_brdf(base_wo, s.w, get_specular_color0(mtl), get_specular_roughness(mtl), get_patch_center(mtl), get_patch_axis0(mtl), get_patch_axis1(mtl), get_cell_size(mtl), get_lod_bias(mtl), u0, u1);
		}
		//diffuse
		else
		{
			s.is_valid = oren_nayer::sample_brdf(base_wo, s.w, get_diffuse_color(mtl), get_diffuse_roughness(mtl), u0, u1);
			if(sample_type == 5){ s.w = -s.w; }
		}
		if(!s.is_valid)
			return s;
	}
	
	s.w = sample_tangent * s.w.x + sample_binormal * s.w.y + sample_normal * s.w.z;
	
	float nwi = dot(normal, s.w);
	if(((sample_type < 5) && (nwi < cosine_threshold)) || ((sample_type == 5) && (nwi > -cosine_threshold)))
	{
		s.is_valid = false;
		return s;
	}
	
	float3 diffuse, non_diffuse;
	calc_bsdf_pdf(wo, s.w, normal, mtl, diffuse, non_diffuse, s.pdf, dtid);
	
	float inv_pdf = 1 / s.pdf;
	s.diffuse_weight = diffuse * inv_pdf;
	s.non_diffuse_weight = non_diffuse * inv_pdf;
	s.weight = (diffuse + non_diffuse) * inv_pdf;
	return s;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

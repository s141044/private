
#pragma once

#ifndef NN_RENDER_MATERIAL_STANDARD_HPP
#define NN_RENDER_MATERIAL_STANDARD_HPP

#include"../render_type.hpp"
#include"..//utility/convert.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//standard_material
/*/////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
�Eresult = sheen * sheen_brdf + (1 - sheen * reflectance(sheen_brdf)) * coat_layer
�Ecoat_layer = coat * coat_brdf + lerp(1, coat_color * (1 - reflectance(coat_brdf)), coat) * (emission + metal_layer);
�Emetal_layer = metalness * metal_brdf + (1 - metalness) * specular_layer
�Especular_layer = specular * specular_brdf + (1 - specular * reflectance(specular_brdf)) * diffuse_layer
�Ediffuse_layer = diffuse * (1 - subsurface) * diffuse_brdf + subsurface * subsurface_layer
�Esubsurface_layer = two_side * diffuse_btdf + (1 - two_side) * bssrdf
/////////////////////////////////////////////////////////////////////////////////////////////////*/

class standard_material : public material
{
public:

	//�R���X�g���N�^
	standard_material() : material(material_type_standard)
	{
	}

	//�o�C���h���X�̊Ǘ�
	void register_bindless()
	{
		struct bindless_material : bindless_material_base
		{
			bindless_material(texture_resource_ptr &p) : bindless_material_base(material_type_standard, p ? p->srv().bindless_handle() : -1){}
			
			uint	coat_normal_map;
			uint	base_normal_map;
			uint	sheen_color_map;
			uint	sheen_roughness_map;
			uint	coat_scale_map;
			uint	coat_color0_map;
			uint	coat_roughness_map;
			uint	emission_scale_map;
			uint	emission_color_map;
			uint	specular_scale_map;
			uint	specular_color0_map;
			uint	specular_roughness_map;
			uint	diffuse_color_map;
			uint	diffuse_roughness_map;
			uint	subsurface_map;
			uint	emission_color;
			uint	subsurface_radius;
			uint	sheen_coat_color; //sheen.xyz,coat.x
			uint	coat_specular_color; //coat.yz,specular.xy
			uint	specular_diffuse_color; //specular.z,diffuse_xyz
		};

		auto set_texture_and_u8_unorm = [&](uint &dst, texture_resource_ptr &p, float f = 0)
		{
			assert((f >= 0) && (f <= 1));
			dst = (p != nullptr) ? p->srv().bindless_handle() : 0x00ffffff;
			dst |= f32_to_u8_unorm(f) << 24;
		};

		bindless_material params(mp_alpha_map);
		set_texture_and_u8_unorm(params.coat_normal_map, mp_coat_normal_map);
		set_texture_and_u8_unorm(params.base_normal_map, mp_base_normal_map);
		set_texture_and_u8_unorm(params.sheen_color_map, mp_sheen_color_map);
		set_texture_and_u8_unorm(params.sheen_roughness_map, mp_sheen_roughness_map, m_sheen_roughness);
		set_texture_and_u8_unorm(params.coat_scale_map, mp_coat_scale_map, m_coat_scale);
		set_texture_and_u8_unorm(params.coat_color0_map, mp_coat_color0_map);
		set_texture_and_u8_unorm(params.coat_roughness_map, mp_coat_roughness_map, m_coat_roughness);
		set_texture_and_u8_unorm(params.emission_scale_map, mp_emission_scale_map);
		set_texture_and_u8_unorm(params.emission_color_map, mp_emission_color_map);
		set_texture_and_u8_unorm(params.specular_scale_map, mp_specular_scale_map, m_specular_scale);
		set_texture_and_u8_unorm(params.specular_color0_map, mp_specular_color0_map);
		set_texture_and_u8_unorm(params.specular_roughness_map, mp_specular_roughness_map, m_specular_roughness);
		set_texture_and_u8_unorm(params.diffuse_color_map, mp_diffuse_color_map);
		set_texture_and_u8_unorm(params.diffuse_roughness_map, mp_diffuse_roughness_map, m_diffuse_roughness);
		set_texture_and_u8_unorm(params.subsurface_map, mp_subsurface_map, m_subsurface);

		params.emission_color = f32x3_to_r9g9b9e5(m_emission_color * m_emission_scale);
		params.subsurface_radius = f32x3_to_r9g9b9e5(m_subsurface_radius * m_subsurface_radius_scale);
		params.sheen_coat_color = f32x4_to_u8x4_unorm(float4(m_sheen_color, m_coat_color0.x));
		params.coat_specular_color = f32x4_to_u8x4_unorm(float4(m_coat_color0.yz, m_specular_color0.xy));
		params.specular_diffuse_color = f32x4_to_u8x4_unorm(float4(m_specular_color0.z, m_diffuse_color));

		mp_buf = gp_render_device->create_byteaddress_buffer(sizeof(params), resource_flag_allow_shader_resource, &params);
		mp_srv = gp_render_device->create_shader_resource_view(*mp_buf, buffer_srv_desc(*mp_buf));
		gp_render_device->register_bindless(*mp_srv);
	}

	//�p�����[�^��ݒ�
	void set_alpha_map(texture_resource_ptr t){ mp_alpha_map = std::move(t); }
	void set_coat_normal_map(texture_resource_ptr t){ mp_coat_normal_map = std::move(t); }
	void set_base_normal_map(texture_resource_ptr t){ mp_base_normal_map = std::move(t); }
	void set_sheen_color(const float3 &c){ m_sheen_color = c; }
	void set_sheen_color_map(texture_resource_ptr t){ mp_sheen_color_map = std::move(t); }
	void set_sheen_roughness(const float s){ m_sheen_roughness = s; }
	void set_sheen_roughness_map(texture_resource_ptr t){ mp_sheen_roughness_map = std::move(t); }
	void set_coat_scale(const float s){ m_coat_scale = s; }
	void set_coat_scale_map(texture_resource_ptr t){ mp_coat_scale_map = std::move(t); }
	void set_coat_color0(const float3 &c){ m_coat_color0 = c; }
	void set_coat_color0_map(texture_resource_ptr t){ mp_coat_color0_map = std::move(t); }
	void set_coat_roughness(const float s){ m_coat_roughness = s; }
	void set_coat_roughness_map(texture_resource_ptr t){ mp_coat_roughness_map = std::move(t); }
	void set_emission_scale(const float s){ m_emission_scale = s; }
	void set_emission_scale_map(texture_resource_ptr t){ mp_emission_scale_map = std::move(t); }
	void set_emission_color(const float3 &c){ m_emission_color = c; }
	void set_emission_color_map(texture_resource_ptr t){ mp_emission_color_map = std::move(t); }
	void set_specular_scale(const float s){ m_specular_scale = s; }
	void set_specular_scale_map(texture_resource_ptr t){ mp_specular_scale_map = std::move(t); }
	void set_specular_color0(const float3 &c){ m_specular_color0 = c; }
	void set_specular_color0_map(texture_resource_ptr t){ mp_specular_color0_map = std::move(t); }
	void set_specular_roughness(const float s){ m_specular_roughness = s; }
	void set_specular_roughness_map(texture_resource_ptr t){ mp_specular_roughness_map = std::move(t); }
	void set_diffuse_color(const float3 &c){ m_diffuse_color = c; }
	void set_diffuse_color_map(texture_resource_ptr t){ mp_diffuse_color_map = std::move(t); }
	void set_diffuse_roughness(const float s){ m_diffuse_roughness = s; }
	void set_diffuse_roughness_map(texture_resource_ptr t){ mp_diffuse_roughness_map = std::move(t); }
	void set_subsurface(const float s){ m_subsurface = s; }
	void set_subsurface_map(texture_resource_ptr t){ mp_subsurface_map = std::move(t); }
	void set_subsurface_radius(const float3 &c){ m_subsurface_radius = c; }
	void set_subsurface_radius_scale(const float s){ m_subsurface_radius_scale = s; }

private:

	texture_resource_ptr	mp_alpha_map;
	texture_resource_ptr	mp_coat_normal_map;
	texture_resource_ptr	mp_base_normal_map;

	float3					m_sheen_color = float3(0.0f);
	float					m_sheen_roughness = 0.3f;
	texture_resource_ptr	mp_sheen_color_map;
	texture_resource_ptr	mp_sheen_roughness_map;

	float					m_coat_scale = 0;
	float3					m_coat_color0 = float3(0.03f);
	float					m_coat_roughness = 0.1f;
	texture_resource_ptr	mp_coat_scale_map;
	texture_resource_ptr	mp_coat_color0_map;
	texture_resource_ptr	mp_coat_roughness_map;

	float					m_emission_scale = 0;
	float3					m_emission_color = float3(0.0f);
	texture_resource_ptr	mp_emission_color_map;
	texture_resource_ptr	mp_emission_scale_map;

	float					m_specular_scale = 0;
	float3					m_specular_color0 = float3(0.03f);
	float					m_specular_roughness = 0.2f;
	texture_resource_ptr	mp_specular_color0_map;
	texture_resource_ptr	mp_specular_scale_map;
	texture_resource_ptr	mp_specular_roughness_map;

	float3					m_diffuse_color = float3(1.0f);
	float					m_diffuse_roughness = 0;
	texture_resource_ptr	mp_diffuse_color_map;
	texture_resource_ptr	mp_diffuse_roughness_map;

	float					m_subsurface = 0;
	float3					m_subsurface_radius = float3(1);
	float					m_subsurface_radius_scale = 1;
	texture_resource_ptr	mp_subsurface_map;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//standard_material_resource
///////////////////////////////////////////////////////////////////////////////////////////////////

class standard_material_resource
{
public:

	//�R���X�g���N�^
	standard_material_resource()
	{
		m_shaders = gp_shader_manager->create(L"reflectance.sdf.json");
	}

	//�o�C���h
	bool bind(render_context &context)
	{
		bool update;
		if(update = m_shaders.has_update())
			m_shaders.update();
		else if(m_shaders.is_invalid())
			return false;

		if(mp_diffuse_reflectance_tex == nullptr)
		{
			update = true;
			mp_diffuse_reflectance_tex = gp_render_device->create_texture2d(texture_format_r16_unorm, 32, 32, 1, resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access));
			mp_diffuse_reflectance_srv = gp_render_device->create_shader_resource_view(*mp_diffuse_reflectance_tex, texture_srv_desc(*mp_diffuse_reflectance_tex));
			mp_diffuse_reflectance_uav = gp_render_device->create_unordered_access_view(*mp_diffuse_reflectance_tex, texture_uav_desc(*mp_diffuse_reflectance_tex));
			gp_render_device->set_name(*mp_diffuse_reflectance_tex, L"diffuse_reflectance");

			mp_specular_reflectance_tex = gp_render_device->create_texture2d(texture_format_r16g16_unorm, 32, 32, 1, resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access));
			mp_specular_reflectance_srv = gp_render_device->create_shader_resource_view(*mp_specular_reflectance_tex, texture_srv_desc(*mp_specular_reflectance_tex));
			mp_specular_reflectance_uav = gp_render_device->create_unordered_access_view(*mp_specular_reflectance_tex, texture_uav_desc(*mp_specular_reflectance_tex));
			gp_render_device->set_name(*mp_specular_reflectance_tex, L"specular_reflectance");

			mp_matte_reflectance_tex = gp_render_device->create_texture2d(texture_format_r32_float, 32, 32, 1, resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access));
			mp_matte_reflectance_srv = gp_render_device->create_shader_resource_view(*mp_matte_reflectance_tex, texture_srv_desc(*mp_matte_reflectance_tex));
			mp_matte_reflectance_uav = gp_render_device->create_unordered_access_view(*mp_matte_reflectance_tex, texture_uav_desc(*mp_matte_reflectance_tex));
			gp_render_device->set_name(*mp_matte_reflectance_tex, L"matte_reflectance");

#include"sheen_ltc_table.hpp"

			mp_sheen_reflectance_tex = gp_render_device->create_texture2d(texture_format_r16_unorm, 32, 32, 1, resource_flag_allow_shader_resource, sheen_reflectance_table);
			mp_sheen_reflectance_srv = gp_render_device->create_shader_resource_view(*mp_sheen_reflectance_tex, texture_srv_desc(*mp_sheen_reflectance_tex));
			gp_render_device->set_name(*mp_sheen_reflectance_tex, L"sheen_reflectance");

			mp_sheen_ltc_tex = gp_render_device->create_texture2d(texture_format_r16g16_snorm, 32, 32, 1, resource_flag_allow_shader_resource, sheen_ltc_table);
			mp_sheen_ltc_srv = gp_render_device->create_shader_resource_view(*mp_sheen_ltc_tex, texture_srv_desc(*mp_sheen_ltc_tex));
			gp_render_device->set_name(*mp_sheen_ltc_tex, L"sheen_ltc");
		}
		if(update)
		{
			context.push_priority(priority_initiaize);
			context.set_pipeline_resource("reflectance_table", *mp_diffuse_reflectance_uav);
			context.set_pipeline_state(*m_shaders.get("calc_diffuse_reflectance"));
			context.dispatch(32, 32, 1);
			context.set_pipeline_resource("reflectance_table", *mp_specular_reflectance_uav);
			context.set_pipeline_state(*m_shaders.get("calc_specular_reflectance"));
			context.dispatch(32, 32, 1);
			context.set_pipeline_resource("reflectance_table", *mp_specular_reflectance_srv);
			context.set_pipeline_resource("reflectance_table_matte", *mp_matte_reflectance_uav);
			context.set_pipeline_state(*m_shaders.get("calc_matte_reflectance"));
			context.dispatch(1, 1, 1);
			context.pop_priority();
		}
		context.set_pipeline_resource("diffuse_reflectance_table", *mp_diffuse_reflectance_srv, true);
		context.set_pipeline_resource("specular_reflectance_table", *mp_specular_reflectance_srv, true);
		context.set_pipeline_resource("matte_reflectance_table", *mp_matte_reflectance_srv, true);
		context.set_pipeline_resource("sheen_reflectance_table", *mp_sheen_reflectance_srv, true);
		context.set_pipeline_resource("sheen_ltc_table", *mp_sheen_ltc_srv, true);
		return true;
	}

private:

	shader_file_holder			m_shaders;
	texture_ptr					mp_diffuse_reflectance_tex;
	shader_resource_view_ptr	mp_diffuse_reflectance_srv;
	unordered_access_view_ptr	mp_diffuse_reflectance_uav;
	texture_ptr					mp_specular_reflectance_tex;
	shader_resource_view_ptr	mp_specular_reflectance_srv;
	unordered_access_view_ptr	mp_specular_reflectance_uav;
	texture_ptr					mp_matte_reflectance_tex;
	shader_resource_view_ptr	mp_matte_reflectance_srv;
	unordered_access_view_ptr	mp_matte_reflectance_uav;
	texture_ptr					mp_sheen_reflectance_tex;
	shader_resource_view_ptr	mp_sheen_reflectance_srv;
	texture_ptr					mp_sheen_ltc_tex;
	shader_resource_view_ptr	mp_sheen_ltc_srv;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

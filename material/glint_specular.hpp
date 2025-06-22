
#pragma once

#ifndef NN_RENDER_MATERIAL_GLINT_SPECULAR_HPP
#define NN_RENDER_MATERIAL_GLINT_SPECULAR_HPP

#include"../base.hpp"
#include"..//utility/convert.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//glint_specular_material
/*/////////////////////////////////////////////////////////////////////////////////////////////////
standardのspecularを置き換え
/////////////////////////////////////////////////////////////////////////////////////////////////*/

class glint_specular_material : public material
{
public:
	
	struct bindless_material : bindless_material_base
	{
		bindless_material(texture_resource_ptr& p_alpha_map, texture_resource_ptr& p_displacement_map, const float max_displacement) : bindless_material_base(material_type_glint_specular, p_alpha_map ? p_alpha_map->srv().bindless_handle() : -1, p_displacement_map ? p_displacement_map->srv().bindless_handle() : -1, max_displacement)
		{
		}

		uint	coat_normal_map;
		uint	base_normal_map;
		uint	sheen_color_map;
		uint	sheen_roughness_map;
		uint	coat_scale_map;
		uint	coat_color0_map;
		uint	coat_roughness_map;
		uint	specular_scale_map;
		uint	specular_color0_map;
		uint	specular_roughness_map;
		uint	diffuse_color_map;
		uint	diffuse_roughness_map;
		uint	subsurface_map;
		uint	subsurface_radius;
		uint	sheen_coat_color; //sheen.xyz,coat.x
		uint	coat_specular_color; //coat.yz,specular.xy
		uint	specular_diffuse_color; //specular.z,diffuse_xyz
		uint	flags_glint_cell_size;
	};

	//コンストラクタ
	glint_specular_material() : material(material_type_glint_specular)
	{
		m_has_update = true;
	}

	//バインドレスの管理
	void register_bindless()
	{
		mp_buf = gp_render_device->create_byteaddress_buffer(sizeof(bindless_material), resource_flag_allow_shader_resource);
		mp_srv = gp_render_device->create_shader_resource_view(*mp_buf, buffer_srv_desc(*mp_buf));
		gp_render_device->register_bindless(*mp_srv);
	}

	//パラメータを更新
	void update(render_context& context) override
	{
		auto set_texture_and_u8_unorm = [&](uint &dst, texture_resource_ptr &p, float f = 0)
		{
			assert((f >= 0) && (f <= 1));
			dst = (p != nullptr) ? p->srv().bindless_handle() : 0x00ffffff;
			dst |= f32_to_u8_unorm(f) << 24;
		};

		bindless_material params(mp_alpha_map, mp_displacement_map, m_max_displacement);
		set_texture_and_u8_unorm(params.coat_normal_map, mp_coat_normal_map);
		set_texture_and_u8_unorm(params.base_normal_map, mp_base_normal_map);
		set_texture_and_u8_unorm(params.sheen_color_map, mp_sheen_color_map);
		set_texture_and_u8_unorm(params.sheen_roughness_map, mp_sheen_roughness_map, m_sheen_roughness);
		set_texture_and_u8_unorm(params.coat_scale_map, mp_coat_scale_map, m_coat_scale);
		set_texture_and_u8_unorm(params.coat_color0_map, mp_coat_color0_map);
		set_texture_and_u8_unorm(params.coat_roughness_map, mp_coat_roughness_map, m_coat_roughness);
		set_texture_and_u8_unorm(params.specular_scale_map, mp_specular_scale_map, m_specular_scale);
		set_texture_and_u8_unorm(params.specular_color0_map, mp_specular_color0_map);
		set_texture_and_u8_unorm(params.specular_roughness_map, mp_specular_roughness_map, m_specular_roughness);
		set_texture_and_u8_unorm(params.diffuse_color_map, mp_diffuse_color_map);
		set_texture_and_u8_unorm(params.diffuse_roughness_map, mp_diffuse_roughness_map, m_diffuse_roughness);
		set_texture_and_u8_unorm(params.subsurface_map, mp_subsurface_map, m_subsurface);

		params.subsurface_radius = f32x3_to_r9g9b9e5(m_subsurface_radius * m_subsurface_radius_scale);
		params.sheen_coat_color = f32x4_to_u8x4_unorm(float4(m_sheen_color, m_coat_color0.x));
		params.coat_specular_color = f32x4_to_u8x4_unorm(float4(m_coat_color0.yz, m_specular_color0.xy));
		params.specular_diffuse_color = f32x4_to_u8x4_unorm(float4(m_specular_color0.z, m_diffuse_color));
		params.flags_glint_cell_size = m_is_twoside ? 1 : 0;
		params.flags_glint_cell_size |= reinterpret<uint16_t>(half(1 / m_specular_glint_density)) << 16;

		push_priority push_priority(context);
		context.set_priority(priority_initiaize);
		void* dst = context.update_buffer(*mp_buf, 0, sizeof(params));
		memcpy(dst, &params, sizeof(params));
		m_has_update = false;
	}

	//アルファマップがあるか
	bool has_alpha_map() const override { return (mp_alpha_map != nullptr); }

	//変位マップがあるか
	bool has_displacement_map() const override { return (mp_displacement_map != nullptr); }

	//SSSがあるか
	bool has_subsurface_scattering() const override { return not(m_is_twoside) && (m_subsurface > 0); }

	//パラメータ
	void set_alpha_map(texture_resource_ptr t){ set_impl(mp_alpha_map, t); }
	void set_coat_normal_map(texture_resource_ptr t){ set_impl(mp_coat_normal_map, t); }
	void set_base_normal_map(texture_resource_ptr t){ set_impl(mp_base_normal_map, t); }
	void set_displacement_map(texture_resource_ptr t){ set_impl(mp_displacement_map, t); }
	void set_sheen_color(const float3 &c){ set_impl(m_sheen_color, c); }
	void set_sheen_color_map(texture_resource_ptr t){ set_impl(mp_sheen_color_map, t); }
	void set_sheen_roughness(const float s){ set_impl(m_sheen_roughness, s); }
	void set_sheen_roughness_map(texture_resource_ptr t){ set_impl(mp_sheen_roughness_map, t); }
	void set_coat_scale(const float s){ set_impl(m_coat_scale, s); }
	void set_coat_scale_map(texture_resource_ptr t){ set_impl(mp_coat_scale_map, t); }
	void set_coat_color0(const float3 &c){ set_impl(m_coat_color0, c); }
	void set_coat_color0_map(texture_resource_ptr t){ set_impl(mp_coat_color0_map, t); }
	void set_coat_roughness(const float s){ set_impl(m_coat_roughness, s); }
	void set_coat_roughness_map(texture_resource_ptr t){ set_impl(mp_coat_roughness_map, t); }
	void set_specular_scale(const float s){ set_impl(m_specular_scale, s); }
	void set_specular_scale_map(texture_resource_ptr t){ set_impl(mp_specular_scale_map, t); }
	void set_specular_color0(const float3 &c){ set_impl(m_specular_color0, c); }
	void set_specular_color0_map(texture_resource_ptr t){ set_impl(mp_specular_color0_map, t); }
	void set_specular_roughness(const float s){ set_impl(m_specular_roughness, s); }
	void set_specular_roughness_map(texture_resource_ptr t){ set_impl(mp_specular_roughness_map, t); }
	void set_specular_glint_density(const float s){ set_impl(m_specular_glint_density, s); }
	void set_diffuse_color(const float3 &c){ set_impl(m_diffuse_color, c); }
	void set_diffuse_color_map(texture_resource_ptr t){ set_impl(mp_diffuse_color_map, t); }
	void set_diffuse_roughness(const float s){ set_impl(m_diffuse_roughness, s); }
	void set_diffuse_roughness_map(texture_resource_ptr t){ set_impl(mp_diffuse_roughness_map, t); }
	void set_subsurface(const float s){ set_impl(m_subsurface, s); }
	void set_subsurface_map(texture_resource_ptr t){ set_impl(mp_subsurface_map, t); }
	void set_subsurface_radius(const float3 &c){ set_impl(m_subsurface_radius, c); }
	void set_subsurface_radius_scale(const float s){ set_impl(m_subsurface_radius_scale, s); }
	void set_max_displacement(const float d){ set_impl(m_max_displacement, d); }
	void set_twoside(const bool b){ set_impl(m_is_twoside, b); }

	float3	sheen_color() const { return m_sheen_color; }
	float	sheen_roughness() const { return m_sheen_roughness; }
	float	coat_scale() const { return m_coat_scale; }
	float3	coat_color0() const { return m_coat_color0; }
	float	coat_roughness() const { return m_coat_roughness; }
	float	specular_glint_density() const { return m_specular_glint_density; }
	float	specular_scale() const { return m_specular_scale; }
	float3	specular_color0() const { return m_specular_color0; }
	float	specular_roughness() const { return m_specular_roughness; }
	float3	diffuse_color() const { return m_diffuse_color; }
	float	diffuse_roughness() const { return m_diffuse_roughness; }
	float	subsurface() const { return m_subsurface; }
	float3	subsurface_radius() const { return m_subsurface_radius; }
	float	subsurface_radius_scale() const { return m_subsurface_radius_scale; }
	bool	is_twoside() const { return m_is_twoside; }

private:

	template<class T, class U> void set_impl(T& dst, U& src)
	{
		if(dst != src)
		{
			dst = std::move(src);
			m_has_update = true;
		}
	}

private:

	texture_resource_ptr	mp_alpha_map;
	texture_resource_ptr	mp_coat_normal_map;
	texture_resource_ptr	mp_base_normal_map;
	texture_resource_ptr	mp_displacement_map;

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

	float					m_specular_scale = 0;
	float3					m_specular_color0 = float3(0.03f);
	float					m_specular_roughness = 0.2f;
	float					m_specular_glint_density = 1e3f;
	texture_resource_ptr	mp_specular_color0_map;
	texture_resource_ptr	mp_specular_scale_map;
	texture_resource_ptr	mp_specular_roughness_map;

	float3					m_diffuse_color = float3(1.0f);
	float					m_diffuse_roughness = 0;
	texture_resource_ptr	mp_diffuse_color_map;
	texture_resource_ptr	mp_diffuse_roughness_map;

	float					m_subsurface = 0;
	float3					m_subsurface_radius = float3(1);
	float					m_subsurface_radius_scale = 1e-3f;
	texture_resource_ptr	mp_subsurface_map;

	bool					m_is_twoside = false;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

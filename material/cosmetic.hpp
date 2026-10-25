
#pragma once

#ifndef NN_RENDER_MATERIAL_COSMETIC_HPP
#define NN_RENDER_MATERIAL_COSMETIC_HPP

#include"../base.hpp"
#include"..//utility/convert.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//cosmetic_material
/*/////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////*/

class cosmetic_material : public material
{
public:
	
	struct bindless_material : bindless_material_base
	{
		bindless_material() : bindless_material_base(material_type_cosmetic, -1, -1, 0)
		{
		}

		uint	normal_map;
		uint	coat_scale_map;
		uint	coat_color0_map;
		uint	coat_roughness_map;
		uint	diffuse_color_map;
		uint	specular_scale_map;
		uint	specular_color0_map;
		uint	specular_roughness_map;
		uint	subsurface_map;
		
		uint	thickness_map;
		uint	diffuser_ratio_map;
		uint	diffuser_albedo_map;
		uint	diffuser_anisotropy1_map;
		uint	diffuser_anisotropy2_map;
		uint	diffuser_lobe_weight_map;
		uint	platelet_albedo_map;
		uint	platelet_roughness_map;

		uint	coat_color0;
		uint	diffuse_color;
		uint	specular_color0;
		uint	subsurface_radius; //r9g9b9e5

		uint	thickness; //half
		uint	diffuser_albedo;
		uint	platelet_albedo;
	};

	//コンストラクタ
	cosmetic_material() : material(material_type_cosmetic)
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

		bindless_material params;
		set_texture_and_u8_unorm(params.normal_map, mp_normal_map);
		set_texture_and_u8_unorm(params.coat_scale_map, mp_coat_scale_map, m_coat_scale);
		set_texture_and_u8_unorm(params.coat_color0_map, mp_coat_color0_map);
		set_texture_and_u8_unorm(params.coat_roughness_map, mp_coat_roughness_map, m_coat_roughness);
		set_texture_and_u8_unorm(params.specular_scale_map, mp_specular_scale_map, m_specular_scale);
		set_texture_and_u8_unorm(params.specular_color0_map, mp_specular_color0_map);
		set_texture_and_u8_unorm(params.specular_roughness_map, mp_specular_roughness_map, m_specular_roughness);
		set_texture_and_u8_unorm(params.diffuse_color_map, mp_diffuse_color_map);
		set_texture_and_u8_unorm(params.subsurface_map, mp_subsurface_map, m_subsurface);

		params.coat_color0 = f32x4_to_u8x4_unorm(float4(m_coat_color0, 0));
		params.diffuse_color = f32x4_to_u8x4_unorm(float4(m_diffuse_color, 0));
		params.specular_color0 = f32x4_to_u8x4_unorm(float4(m_specular_color0, 0));
		params.subsurface_radius = f32x3_to_r9g9b9e5(m_subsurface_radius * m_subsurface_radius_scale);

		set_texture_and_u8_unorm(params.thickness_map, mp_thickness_map);
		set_texture_and_u8_unorm(params.diffuser_ratio_map, mp_diffuser_ratio_map, m_diffuser_ratio);
		set_texture_and_u8_unorm(params.diffuser_albedo_map, mp_diffuser_albedo_map);
		set_texture_and_u8_unorm(params.diffuser_anisotropy1_map, mp_diffuser_anisotropy1_map, m_diffuser_anisotropy1);
		set_texture_and_u8_unorm(params.diffuser_anisotropy2_map, mp_diffuser_anisotropy2_map, m_diffuser_anisotropy2);
		set_texture_and_u8_unorm(params.diffuser_lobe_weight_map, mp_diffuser_lobe_weight_map, m_diffuser_lobe_weight);
		set_texture_and_u8_unorm(params.platelet_albedo_map, mp_platelet_albedo_map);
		set_texture_and_u8_unorm(params.platelet_roughness_map, mp_platelet_roughness_map, m_platelet_roughness);

		params.thickness = f32x2_to_f16x2(float2(m_thickness, m_thickness_scale));
		params.diffuser_albedo = f32x4_to_u8x4_unorm(float4(m_diffuser_albedo, 0));
		params.platelet_albedo = f32x4_to_u8x4_unorm(float4(m_platelet_albedo, 0));

		push_priority push_priority(context);
		context.set_priority(priority_initiaize);
		void* dst = context.update_buffer(*mp_buf, 0, sizeof(params));
		memcpy(dst, &params, sizeof(params));
		m_has_update = false;
	}

	//SSSがあるか
	bool has_subsurface_scattering() const override { return (m_subsurface > 0); }

	//パラメータ
	void set_normal_map(texture_resource_ptr t){ set_impl(mp_normal_map, t); }
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
	void set_diffuse_color(const float3 &c){ set_impl(m_diffuse_color, c); }
	void set_diffuse_color_map(texture_resource_ptr t){ set_impl(mp_diffuse_color_map, t); }
	void set_subsurface(const float s){ set_impl(m_subsurface, s); }
	void set_subsurface_map(texture_resource_ptr t){ set_impl(mp_subsurface_map, t); }
	void set_subsurface_radius(const float3 &c){ set_impl(m_subsurface_radius, c); }
	void set_subsurface_radius_scale(const float s){ set_impl(m_subsurface_radius_scale, s); }
	
	void set_thickness(const float s){ set_impl(m_thickness, s); }
	void set_thickness_map(texture_resource_ptr t) { set_impl(mp_thickness_map, t); }
	void set_thickness_scale(const float s) { set_impl(m_thickness_scale, s); }
	void set_diffuser_ratio(const float s) { set_impl(m_diffuser_ratio, s); }
	void set_diffuser_ratio_map(texture_resource_ptr t) { set_impl(mp_diffuser_ratio_map, t); }
	void set_diffuser_albedo(const float3 &c) { set_impl(m_diffuser_albedo, c); }
	void set_diffuser_albedo_map(texture_resource_ptr t) { set_impl(mp_diffuser_albedo_map, t); }
	void set_diffuser_anisotropy1(const float s) { set_impl(m_diffuser_anisotropy1, s); }
	void set_diffuser_anisotropy1_map(texture_resource_ptr t) { set_impl(mp_diffuser_anisotropy1_map, t); }
	void set_diffuser_anisotropy2(const float s) { set_impl(m_diffuser_anisotropy2, s); }
	void set_diffuser_anisotropy2_map(texture_resource_ptr t) { set_impl(mp_diffuser_anisotropy2_map, t); }
	void set_diffuser_lobe_weight(const float s) { set_impl(m_diffuser_lobe_weight, s); }
	void set_diffuser_lobe_weight_map(texture_resource_ptr t) { set_impl(mp_diffuser_lobe_weight_map, t); }
	void set_platelet_albedo(const float3 &c) { set_impl(m_platelet_albedo, c); }
	void set_platelet_albedo_map(texture_resource_ptr t) { set_impl(mp_platelet_albedo_map, t); }
	void set_platelet_roughness(const float s) { set_impl(m_platelet_roughness, s); }
	void set_platelet_roughness_map(texture_resource_ptr t) { set_impl(mp_platelet_roughness_map, t); }

	float	coat_scale() const { return m_coat_scale; }
	float3	coat_color0() const { return m_coat_color0; }
	float	coat_roughness() const { return m_coat_roughness; }
	float	specular_scale() const { return m_specular_scale; }
	float3	specular_color0() const { return m_specular_color0; }
	float	specular_roughness() const { return m_specular_roughness; }
	float3	diffuse_color() const { return m_diffuse_color; }
	float	subsurface() const { return m_subsurface; }
	float3	subsurface_radius() const { return m_subsurface_radius; }
	float	subsurface_radius_scale() const { return m_subsurface_radius_scale; }
	
	float	thickness() const { return m_thickness; }
	float	thickness_scale() const  { return m_thickness_scale; }
	float	diffuser_ratio() const  { return m_diffuser_ratio; }
	float3	diffuser_albedo() const  { return m_diffuser_albedo; }
	float	diffuser_anisotropy1() const  { return m_diffuser_anisotropy1; }
	float	diffuser_anisotropy2() const  { return m_diffuser_anisotropy2; }
	float	diffuser_lobe_weight() const  { return m_diffuser_lobe_weight; }
	float3	platelet_albedo() const  { return m_platelet_albedo; }
	float	platelet_roughness() const  { return m_platelet_roughness; }

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

	texture_resource_ptr	mp_normal_map;

	float					m_coat_scale = 0;
	float3					m_coat_color0 = float3(0.03f);
	float					m_coat_roughness = 0.2f;
	texture_resource_ptr	mp_coat_color0_map;
	texture_resource_ptr	mp_coat_scale_map;
	texture_resource_ptr	mp_coat_roughness_map;

	float					m_specular_scale = 0;
	float3					m_specular_color0 = float3(0.03f);
	float					m_specular_roughness = 0.2f;
	texture_resource_ptr	mp_specular_color0_map;
	texture_resource_ptr	mp_specular_scale_map;
	texture_resource_ptr	mp_specular_roughness_map;

	float3					m_diffuse_color = float3(1.0f);
	texture_resource_ptr	mp_diffuse_color_map;

	float					m_subsurface = 0;
	float3					m_subsurface_radius = float3(1);
	float					m_subsurface_radius_scale = 1e-3f;
	texture_resource_ptr	mp_subsurface_map;

	float					m_thickness = 16;
	texture_resource_ptr	mp_thickness_map;
	float					m_thickness_scale = 1;

	float					m_diffuser_ratio = 1;

	texture_resource_ptr	mp_diffuser_ratio_map;
	float3					m_diffuser_albedo = float3(0.5f);
	texture_resource_ptr	mp_diffuser_albedo_map;
	float					m_diffuser_anisotropy1 = 0;
	texture_resource_ptr	mp_diffuser_anisotropy1_map;
	float					m_diffuser_anisotropy2 = 0;
	texture_resource_ptr	mp_diffuser_anisotropy2_map;
	float					m_diffuser_lobe_weight = 0;
	texture_resource_ptr	mp_diffuser_lobe_weight_map;

	float3					m_platelet_albedo = float3(0.5f);
	texture_resource_ptr	mp_platelet_albedo_map;
	float					m_platelet_roughness = 0.2f;
	texture_resource_ptr	mp_platelet_roughness_map;

};

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

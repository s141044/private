
#pragma once

#ifndef NN_RENDER_MATERIAL_EYEBALL_HPP
#define NN_RENDER_MATERIAL_EYEBALL_HPP

#include"../base.hpp"
#include"..//utility/convert.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//eyeball_material
/*/////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
・result = sheen * sheen_brdf + (1 - sheen * reflectance(sheen_brdf)) * coat_layer
・coat_layer = coat * coat_brdf + lerp(1, coat_color * (1 - reflectance(coat_brdf)), coat) * (emissive + metal_layer);
・metal_layer = metalness * metal_brdf + (1 - metalness) * specular_layer
・specular_layer = specular * specular_brdf + (1 - specular * reflectance(specular_brdf)) * diffuse_layer
・diffuse_layer = diffuse * (1 - subsurface) * diffuse_brdf + subsurface * subsurface_layer
・subsurface_layer = two_side * diffuse_btdf + (1 - two_side) * bssrdf
/////////////////////////////////////////////////////////////////////////////////////////////////*/

class eyeball_material : public material
{
public:
	
	struct bindless_material : bindless_material_base
	{
		bindless_material() : bindless_material_base(material_type_eyeball, -1, -1, 0)
		{
		}

		uint	height_map;
		uint	coat_normal_map;
		uint	base_normal_map;
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

		uint	data0; //height_scale, coat_color0.xy
		uint	data1; //coat_color0.z, specular_color0.xyz
		uint	data2; //diffuse_color.xyz, -
		
		uint	uv_offset;
	};

	//コンストラクタ
	eyeball_material() : material(material_type_eyeball)
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
		set_texture_and_u8_unorm(params.height_map, mp_height_map);
		set_texture_and_u8_unorm(params.coat_normal_map, mp_coat_normal_map);
		set_texture_and_u8_unorm(params.base_normal_map, mp_base_normal_map);
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
		params.uv_offset = f32x2_to_f16x2(m_uv_offset);

		params.data0 = f32_to_f16(m_height_scale) | f32x4_to_u8x4_unorm(float4(0, 0, m_coat_color0.xy));
		params.data1 = f32x4_to_u8x4_unorm(float4(m_coat_color0.z, m_specular_color0));
		params.data2 = f32x4_to_u8x4_unorm(float4(m_diffuse_color, 0));

		push_priority push_priority(context);
		context.set_priority(priority_initiaize);
		void* dst = context.update_buffer(*mp_buf, 0, sizeof(params));
		memcpy(dst, &params, sizeof(params));
		m_has_update = false;
	}

	//SSSがあるか
	bool has_subsurface_scattering() const override { return (m_subsurface > 0); }

	//パラメータ
	void set_coat_normal_map(texture_resource_ptr t){ set_impl(mp_coat_normal_map, t); }
	void set_base_normal_map(texture_resource_ptr t){ set_impl(mp_base_normal_map, t); }
	void set_height_map(texture_resource_ptr t){ set_impl(mp_height_map, t); }
	void set_height_scale(const float d){ set_impl(m_height_scale, d); }
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
	void set_diffuse_roughness(const float s){ set_impl(m_diffuse_roughness, s); }
	void set_diffuse_roughness_map(texture_resource_ptr t){ set_impl(mp_diffuse_roughness_map, t); }
	void set_subsurface(const float s){ set_impl(m_subsurface, s); }
	void set_subsurface_map(texture_resource_ptr t){ set_impl(mp_subsurface_map, t); }
	void set_subsurface_radius(const float3 &c){ set_impl(m_subsurface_radius, c); }
	void set_subsurface_radius_scale(const float s){ set_impl(m_subsurface_radius_scale, s); }
	void set_uv_offset(const float2 &o){ set_impl(m_uv_offset, o); }

	float	height_scale() const { return m_height_scale; }
	float	coat_scale() const { return m_coat_scale; }
	float3	coat_color0() const { return m_coat_color0; }
	float	coat_roughness() const { return m_coat_roughness; }
	float	specular_scale() const { return m_specular_scale; }
	float3	specular_color0() const { return m_specular_color0; }
	float	specular_roughness() const { return m_specular_roughness; }
	float3	diffuse_color() const { return m_diffuse_color; }
	float	diffuse_roughness() const { return m_diffuse_roughness; }
	float	subsurface() const { return m_subsurface; }
	float3	subsurface_radius() const { return m_subsurface_radius; }
	float	subsurface_radius_scale() const { return m_subsurface_radius_scale; }
	float2	uv_offset() const { return m_uv_offset; }

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

	texture_resource_ptr	mp_coat_normal_map;
	texture_resource_ptr	mp_base_normal_map;

	texture_resource_ptr	mp_height_map;
	float					m_height_scale = 0.01f;

	float					m_coat_scale = 0;
	float3					m_coat_color0 = float3(0.03f);
	float					m_coat_roughness = 0.1f;
	texture_resource_ptr	mp_coat_scale_map;
	texture_resource_ptr	mp_coat_color0_map;
	texture_resource_ptr	mp_coat_roughness_map;

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
	float					m_subsurface_radius_scale = 1e-3f;
	texture_resource_ptr	mp_subsurface_map;

	float2					m_uv_offset = 0;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

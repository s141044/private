
#pragma once

#ifndef NN_RENDER_MATERIAL_LAMBERT_HPP
#define NN_RENDER_MATERIAL_LAMBERT_HPP

#include"../utility.hpp"
#include"../render_type.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//lambert_material
///////////////////////////////////////////////////////////////////////////////////////////////////

class lambert_material : public material
{
public:

	//コンストラクタ
	lambert_material() : material(material_type_lambert)
	{
	}

	//バインドレスの管理
	void register_bindless() override
	{
		struct bindless_material : bindless_material_base
		{
			bindless_material() : bindless_material_base(material_type_lambert, -1){}
			uint albedo = -1; //r8g8b8a8
			uint albedo_map = -1;
			uint normal_map = -1;
		};
		bindless_material params;
		params.albedo = f32x4_to_u8x4_unorm(float4(m_albedo, 0));
		if(mp_albedo_map)
			params.albedo_map = mp_albedo_map->srv().bindless_handle();
		if(mp_normal_map)
			params.normal_map = mp_normal_map->srv().bindless_handle();

		mp_buf = gp_render_device->create_byteaddress_buffer(sizeof(params), resource_flag_allow_shader_resource, &params);
		mp_srv = gp_render_device->create_shader_resource_view(*mp_buf, buffer_srv_desc(*mp_buf));
		gp_render_device->register_bindless(*mp_srv);
	}

	//パラメータを設定
	void set_albedo(const float3 &albedo){ m_albedo = albedo; }
	void set_albedo_map(const string &filename){ mp_albedo_map = gp_texture_manager->create(filename, texture_type_srgb); }
	void set_normal_map(const string &filename){ mp_normal_map = gp_texture_manager->create(filename, texture_type_normal); }

private:

	float3					m_albedo;
	texture_resource_ptr	mp_albedo_map;
	texture_resource_ptr	mp_normal_map;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

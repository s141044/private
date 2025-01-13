
#pragma once

#ifndef NN_RENDER_RENDERER_PRIMARY_HPP
#define NN_RENDER_RENDERER_PRIMARY_HPP

#include"../core_impl.hpp"
#include"../render_type.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//direct_lighting
/*/////////////////////////////////////////////////////////////////////////////////////////////////
∫{L*F*G*V*dw} ≒ ∫{L*F*G*dw} * denoise(∫{L*F*G*V*dw}) / denoise(∫{L*F*G*dw})
/////////////////////////////////////////////////////////////////////////////////////////////////*/

class direct_lighting
{
public:

	//コンストラクタ
	direct_lighting()
	{
		m_shader_file = gp_shader_manager->create(L"direct_lighting.sdf.json");
	}

	//
	bool execute(render_context &context, const uint2 screen_size, const camera &camera)
	{
		if(m_shader_file.has_update())
			m_shader_file.update();
		else if(m_shader_file.is_invalid())
			return false;

		init_resources(screen_size);
		update_constant_buffer();


		//context.set_pipeline_resource(*mp_diffuse_result_tex);
		//context.set_pipeline_resource(*mp_specular_result_tex);
		//context.set_pipeline_resource(*mp_shadowed_result_tex[0]);
		//context.set_pipeline_resource(*mp_unshadowed_result_tex[0]);
		//context.set_pipeline_resource(*mp_subsurface_radius_tex);
		//context.set_pipeline_state(*m_shader_file.get("lighting"));


		return true;
	}

	bool execute_sssss(render_context &context)
	{
		return true;
	}

private:

	void init_resources(const uint2 screen_size)
	{
		if((mp_tex[0] != nullptr) && (mp_tex[0]->width() == screen_size.x) &&  (mp_tex[0]->height() == screen_size.y))
			return;

		for(uint i = 0; i < texture_type_count; i++)
		{
			texture_format format;
			switch(i)
			{
			case texture_type_diffuse_result:
				format = texture_format_r32_uint;
				break;
			case texture_type_specular_result:
				format = texture_format_r32_uint;
				break;
			case texture_type_shadowed_result_ping:
			case texture_type_shadowed_result_pong:
				break;

			}
			mp_tex[i] = gp_render_device->create_texture2d(format, screen_size.x, screen_size.y, 1, resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access));
			mp_srv[i] = gp_render_device->create_shader_resource_view(*mp_tex[i], texture_srv_desc(*mp_tex[i]));
			mp_uav[i] = gp_render_device->create_unordered_access_view(*mp_tex[i], texture_uav_desc(*mp_tex[i]));
		}
	}
	void update_constant_buffer()
	{
	}

private:

	enum texture_type
	{
		texture_type_diffuse_result, 
		texture_type_specular_result, 
		texture_type_shadowed_result_ping, 
		texture_type_shadowed_result_pong, 
		texture_type_unshadowed_result_ping, 
		texture_type_unshadowed_result_pong, 

		texture_type_subsurface_radius, 
		texture_type_subsurface_weight,
		texture_type_subsurface_misc,

		texture_type_depth, 
		texture_type_diffuse_albedo, 
		texture_type_specular_albedo, 
		texture_type_normal_roughness, 
		texture_type_velocity, 

		texture_type_count, 
	};

	shader_file_holder										m_shader_file;
	uint													m_pingpong_index = 0;
	array<texture_ptr, texture_type_count>					mp_tex;
	array<shader_resource_view_ptr, texture_type_count>		mp_srv;
	array<unordered_access_view_ptr, texture_type_count>	mp_uav;

	//texture_ptr					mp_diffuse_result_tex;		//r9g9b9e5 (アルベド乗算なし,SSSSS後にアルベド乗算)
	//texture_ptr					mp_specular_result_tex;		//r9g9b9e5
	//
	//texture_ptr					mp_shadowed_result_tex[2];		//
	//texture_ptr					mp_unshadowed_result_tex[2];	//
	//
	//
	////SSSSS
	//texture_ptr					mp_subsurface_radius_tex;	//r9g9b9e5
	//texture_ptr					mp_subsurface_weight_tex;	//r8_unorm
	//texture_ptr					mp_subsurface_misc_tex;		//r32_uint (depth: r24_unorm, cos: r8_unorm，SSSなしは0，SSSのグループは必要になったらdepthを削って追加)
	//
	////いろんな用途
	//texture_ptr					mp_depth_tex;				//r32_float
	//texture_ptr					mp_diffuse_albedo_tex;		//r10g10b10a2_unorm
	//texture_ptr					mp_specular_albedo_tex;		//r10g10b10a2_unorm
	//texture_ptr					mp_normal_roughness_tex;	//r10g10b10a2_unorm (specular roughness)
	//texture_ptr					mp_velocity_tex;			//r16g16g16a6_snorm (zはdepth差, wは余ってる)
};

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

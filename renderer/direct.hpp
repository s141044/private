
#pragma once

#ifndef NN_RENDER_RENDERER_DIRECT_HPP
#define NN_RENDER_RENDERER_DIRECT_HPP

#include"../core_impl.hpp"
#include"../render_type.hpp"
#include"../light/direction.hpp"
#include"../light/environment.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//direct_illumination
/*/////////////////////////////////////////////////////////////////////////////////////////////////
∫{L*F*G*V*dw} ≒ ∫{L*F*G*dw} * denoise(∫{L*F*G*V*dw}) / denoise(∫{L*F*G*dw})
/////////////////////////////////////////////////////////////////////////////////////////////////*/

class direct_illumination
{
public:

	struct params
	{
		uint2							screen_size;
		const camera*					p_camera;
		const environment_light*		p_environment_light;
		const environment_light_sampler*p_environment_light_sampler;
		const directional_light*		p_directional_light;
		unordered_access_view*			p_diffuse_result;
		unordered_access_view*			p_non_diffuse_result;
	};

	//コンストラクタ
	direct_illumination()
	{
		m_shader_file = gp_shader_manager->create(L"test/direct.sdf.json");
	}

	//実行
	bool execute(render_context &context, const params &params)
	{
		if(m_shader_file.has_update())
			m_shader_file.update();
		else if(m_shader_file.is_invalid())
			return false;

		init_resources(params);

		params.p_environment_light_sampler->bind(context);
		context.set_pipeline_resource("env_cube", params.p_environment_light->cube_srv());
		context.set_pipeline_resource("env_panorama", params.p_environment_light->src_srv());
		context.set_pipeline_resource("diffuse_result", *params.p_diffuse_result);
		context.set_pipeline_resource("non_diffuse_result", *params.p_non_diffuse_result);
		context.set_pipeline_resource("shadowed_result", *mp_uav[texture_type_shadowed_result_ping]);
		context.set_pipeline_resource("unshadowed_result", *mp_uav[texture_type_unshadowed_result_ping]);
		context.set_pipeline_resource("subsurface_radius", *mp_uav[texture_type_subsurface_radius]);
		context.set_pipeline_resource("subsurface_weight", *mp_uav[texture_type_subsurface_weight]);
		context.set_pipeline_resource("subsurface_misc", *mp_uav[texture_type_subsurface_misc]);
		context.set_pipeline_resource("diffuse_color", *mp_uav[texture_type_diffuse_color]);
		context.set_pipeline_resource("specular_color0", *mp_uav[texture_type_specular_color0]);
		context.set_pipeline_resource("normal_roughness", *mp_uav[texture_type_normal_roughness]);
		context.set_pipeline_resource("velocity", *mp_uav[texture_type_velocity]);
		context.set_pipeline_resource("depth", *mp_uav[texture_type_depth]);
		context.set_pipeline_state(*m_shader_file.get("trace_and_shade"));
		context.dispatch(ceil_div(params.screen_size.x, 8), ceil_div(params.screen_size.y, 4), 1);
		return true;
	}

	//SRVを返す
	shader_resource_view& depth_srv() const { return *mp_srv[texture_type_depth]; }
	shader_resource_view& velocity_srv() const { return *mp_srv[texture_type_velocity]; }
	shader_resource_view& normal_roughness_srv() const { return *mp_srv[texture_type_normal_roughness]; }
	shader_resource_view& diffuse_color_srv() const { return *mp_srv[texture_type_diffuse_color]; }
	shader_resource_view& specular_color0_srv() const { return *mp_srv[texture_type_specular_color0]; }
	shader_resource_view& subsurface_weight_srv() const { return *mp_srv[texture_type_subsurface_weight]; }
	shader_resource_view& subsurface_radius_srv() const { return *mp_srv[texture_type_subsurface_radius]; }
	shader_resource_view& subsurface_misc_srv() const { return *mp_srv[texture_type_subsurface_misc]; }

private:

	void init_resources(const params &params)
	{
		const auto &screen_size = params.screen_size;
		if((mp_tex[0] != nullptr) && (mp_tex[0]->width() == screen_size.x) &&  (mp_tex[0]->height() == screen_size.y))
			return;

		//とりま，デノイズのことは考えない

		for(uint i = 0; i < texture_type_count; i++)
		{
			texture_format format;
			switch(i)
			{
			case texture_type_shadowed_result_ping:
			case texture_type_shadowed_result_pong:
			case texture_type_unshadowed_result_ping:
			case texture_type_unshadowed_result_pong:
				format = texture_format_r16_float;
				break;
			case texture_type_subsurface_radius:
				format = texture_format_r32_uint; //r9g9b9e5
				break;
			case texture_type_subsurface_weight:
				format = texture_format_r8_unorm;
				break;
			case texture_type_subsurface_misc:
				format = texture_format_r32_uint; //r24_unorm depth, r8_unorm cos
				break;
			case texture_type_depth:
				format = texture_format_r32_float;
				break;
			case texture_type_diffuse_color:
			case texture_type_specular_color0:
			case texture_type_normal_roughness:
				format = texture_format_r10g10b10a2_unorm;
				break;
			case texture_type_velocity:
				format = texture_format_r16g16b16a16_snorm; //velocityはスクリーン空間[0,1], zはdepth差, wは未使用
				break;
			default:
				assert(false);
			}

			auto flags = resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access);
			if(true)
				flags = resource_flags(flags | resource_flag_scratch);

			mp_tex[i] = gp_render_device->create_texture2d(format, screen_size.x, screen_size.y, 1, flags);
			mp_srv[i] = gp_render_device->create_shader_resource_view(*mp_tex[i], texture_srv_desc(*mp_tex[i]));
			mp_uav[i] = gp_render_device->create_unordered_access_view(*mp_tex[i], texture_uav_desc(*mp_tex[i]));
		}
	}

private:

	enum texture_type
	{
		texture_type_shadowed_result_ping, 
		texture_type_shadowed_result_pong, 
		texture_type_unshadowed_result_ping, 
		texture_type_unshadowed_result_pong, 

		texture_type_subsurface_radius, 
		texture_type_subsurface_weight,
		texture_type_subsurface_misc,

		texture_type_depth, 
		texture_type_diffuse_color, 
		texture_type_specular_color0, 
		texture_type_normal_roughness, 
		texture_type_velocity, 

		texture_type_count, 
	};

	shader_file_holder										m_shader_file;
	uint													m_pingpong_index = 0;
	array<texture_ptr, texture_type_count>					mp_tex;
	array<shader_resource_view_ptr, texture_type_count>		mp_srv;
	array<unordered_access_view_ptr, texture_type_count>	mp_uav;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif


#pragma once

#ifndef NN_RENDER_RENDERER_REFERENCE_HPP
#define NN_RENDER_RENDERER_REFERENCE_HPP

#include"../base.hpp"
#include"../light/direction.hpp"
#include"../light/environment.hpp"
#include"realistic_camera.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//reference_renderer
///////////////////////////////////////////////////////////////////////////////////////////////////

class reference_renderer
{
public:

	struct params
	{
		uint2					screen_size;
		camera*					p_camera;
		environment_light*		p_envmap;
		unordered_access_view*	p_color_uav;
		unordered_access_view*	p_depth_uav;
		bool					reset_accumulation;
	};

	//コンストラクタ
	reference_renderer()
	{
		m_shader_file = gp_shader_manager->create(L"renderer/reference.sdf.json");
		m_realistic_camera.set_filename("lens/dgauss.dat");
	}

	//実行
	bool execute(render_context &context, const params &params)
	{
		if(m_shader_file.has_update())
			m_shader_file.update();
		else if(m_shader_file.is_invalid())
			return false;

		const auto screen_size = params.screen_size;
		if((mp_accum_tex == nullptr) || (mp_accum_tex->width() != screen_size.x) || (mp_accum_tex->height() != screen_size.y))
		{
			mp_accum_tex = gp_render_device->create_texture2d(texture_format_r32g32b32a32_float, screen_size.x, screen_size.y, 1, resource_flag_allow_unordered_access);
			mp_accum_uav = gp_render_device->create_unordered_access_view(*mp_accum_tex, texture_uav_desc(*mp_accum_tex));
		}

		const uint emissive_presample_count = 1024 * 64;
		const uint environment_presample_count = 1024 * 8;

		if(not(m_emissive_sampler.sample(context, emissive_presample_count)))
			return false;

		if(params.p_envmap)
		{
			if(not(m_environment_sampler.sample(context, *params.p_envmap, 1024, environment_presample_count)))
				return false;
		}

		if(params.reset_accumulation)
			context.clear_unordered_access_view(*mp_accum_uav, uint4(0, 0, 0, 0));

		if(m_use_realistic_camera)
		{
			if(!m_realistic_camera.initialize(context, *params.p_camera))
				return false;

			m_realistic_camera.bind(context);
		}

		struct cbuffer
		{
			uint	max_bounce;
			uint	max_accumulation;
			uint	emissive_presample_count;
			uint	environment_presample_count;
		};
		auto p_cbuf = gp_render_device->create_temporary_cbuffer(sizeof(cbuffer));
		auto& cbuf_data = *p_cbuf->data<cbuffer>();
		cbuf_data.max_bounce = m_max_bounce;
		cbuf_data.max_accumulation = m_infinite_accumulation ? UINT_MAX : m_max_accumulation;
		cbuf_data.emissive_presample_count = emissive_presample_count;
		cbuf_data.environment_presample_count = environment_presample_count;

		m_emissive_sampler.bind(context);
		m_environment_sampler.bind(context);

		static texture_ptr p_debug_tex[4];
		static unordered_access_view_ptr p_debug_uav[4];
		if(p_debug_tex[0] == nullptr)
		{
			p_debug_tex[0] = gp_render_device->create_texture2d(texture_format_r32g32b32a32_float, params.screen_size.x, params.screen_size.y, 1, resource_flag_allow_unordered_access);
			p_debug_tex[1] = gp_render_device->create_texture2d(texture_format_r32g32b32a32_float, params.screen_size.x, params.screen_size.y, 1, resource_flag_allow_unordered_access);
			p_debug_tex[2] = gp_render_device->create_texture2d(texture_format_r32g32b32a32_float, params.screen_size.x, params.screen_size.y, 1, resource_flag_allow_unordered_access);
			p_debug_tex[3] = gp_render_device->create_texture2d(texture_format_r32g32b32a32_float, params.screen_size.x, params.screen_size.y, 1, resource_flag_allow_unordered_access);
			p_debug_uav[0] = gp_render_device->create_unordered_access_view(*p_debug_tex[0], texture_uav_desc(*p_debug_tex[0]));
			p_debug_uav[1] = gp_render_device->create_unordered_access_view(*p_debug_tex[1], texture_uav_desc(*p_debug_tex[1]));
			p_debug_uav[2] = gp_render_device->create_unordered_access_view(*p_debug_tex[2], texture_uav_desc(*p_debug_tex[2]));
			p_debug_uav[3] = gp_render_device->create_unordered_access_view(*p_debug_tex[3], texture_uav_desc(*p_debug_tex[3]));
		}
		context.set_pipeline_resource("debug_uav0", *p_debug_uav[0]);
		context.set_pipeline_resource("debug_uav1", *p_debug_uav[1]);
		context.set_pipeline_resource("debug_uav2", *p_debug_uav[2]);
		context.set_pipeline_resource("debug_uav3", *p_debug_uav[3]);
		context.clear_unordered_access_view(*p_debug_uav[0], uint4(0,0,0,0));
		context.clear_unordered_access_view(*p_debug_uav[1], uint4(0,0,0,0));
		context.clear_unordered_access_view(*p_debug_uav[2], uint4(0,0,0,0));
		context.clear_unordered_access_view(*p_debug_uav[3], uint4(0,0,0,0));

		context.set_pipeline_resource("reference_cbuf", *p_cbuf);
		context.set_pipeline_resource("accum_uav", *mp_accum_uav);
		context.set_pipeline_resource("color_uav", *params.p_color_uav);
		context.set_pipeline_resource("depth_uav", *params.p_depth_uav);
	
		if(m_use_realistic_camera)
			context.set_pipeline_state(*m_shader_file.get("path_tracing_use_realistic_camera"));
		else
			context.set_pipeline_state(*m_shader_file.get("path_tracing"));
		
		context.dispatch(ceil_div(params.screen_size.x, 8), ceil_div(params.screen_size.y, 4), 1);
		return true;
	}

	//パラメータ
	uint max_bounce() const { return m_max_bounce; }
	void set_max_bounce(const uint max_bounce){ m_max_bounce = max_bounce; }
	uint max_accumulation() const { return m_max_accumulation; }
	void set_max_accumulation(const uint max_accumulation){ m_max_accumulation = max_accumulation; }
	bool infinite_accumulation() const { return m_infinite_accumulation; }
	void set_infinite_accumulation(const bool v) { m_infinite_accumulation = v; }
	bool use_realistic_camera() const { return m_use_realistic_camera; }
	void set_use_realistic_camera(const bool v) { m_use_realistic_camera = v; }
	realistic_camera& realistic_camera(){ return m_realistic_camera; }

private:

	shader_file_holder				m_shader_file;
	texture_ptr						mp_accum_tex;
	unordered_access_view_ptr		mp_accum_uav;
	render::realistic_camera		m_realistic_camera;
	emissive_sampler				m_emissive_sampler;
	environment_light_sampler		m_environment_sampler;

	uint							m_max_bounce = 3;
	uint							m_max_accumulation = 64;
	bool							m_use_realistic_camera = false;
	bool							m_infinite_accumulation = true;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

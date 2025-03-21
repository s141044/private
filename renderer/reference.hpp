
#pragma once

#ifndef NN_RENDER_RENDERER_REFERENCE_HPP
#define NN_RENDER_RENDERER_REFERENCE_HPP

#include"../base.hpp"
#include"../light/direction.hpp"
#include"../light/environment.hpp"

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
		environment_light*		p_envmap;
		unordered_access_view*	p_color_uav;
		unordered_access_view*	p_depth_uav;
		bool					reset_accumulation;
	};

	//コンストラクタ
	reference_renderer()
	{
		m_shader_file = gp_shader_manager->create(L"renderer/reference.sdf.json");
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

		if(not(m_use_accumulation) || m_force_reset || params.reset_accumulation)
		{
			m_force_reset = false;
			context.clear_unordered_access_view(*mp_accum_uav, uint4(0, 0, 0, 0));
		}

		struct cbuffer
		{
			uint	max_bounce;
			uint	emissive_presample_count;
			uint	environment_presample_count;
			uint	reserved;
		};
		auto p_cbuf = gp_render_device->create_temporary_cbuffer(sizeof(cbuffer));
		auto& cbuf_data = *p_cbuf->data<cbuffer>();
		cbuf_data.max_bounce = m_max_bounce;
		cbuf_data.emissive_presample_count = emissive_presample_count;
		cbuf_data.environment_presample_count = environment_presample_count;

		m_emissive_sampler.bind(context);
		m_environment_sampler.bind(context);

		context.set_pipeline_resource("reference_cbuf", *p_cbuf);
		context.set_pipeline_resource("accum_uav", *mp_accum_uav);
		context.set_pipeline_resource("color_uav", *params.p_color_uav);
		context.set_pipeline_resource("depth_uav", *params.p_depth_uav);
		context.set_pipeline_state(*m_shader_file.get("path_tracing"));
		context.dispatch(ceil_div(params.screen_size.x, 8), ceil_div(params.screen_size.y, 4), 1);
		return true;
	}

	//パラメータ
	uint max_bounce() const { return m_max_bounce; }
	void set_max_bounce(const uint max_bounce){ m_max_bounce = max_bounce; m_force_reset = true; }
	bool use_accumulation() const { return m_use_accumulation; }
	void set_use_accumulation(const bool v) { m_use_accumulation = v; }

private:

	shader_file_holder			m_shader_file;
	texture_ptr					mp_accum_tex;
	unordered_access_view_ptr	mp_accum_uav;
	emissive_sampler			m_emissive_sampler;
	environment_light_sampler	m_environment_sampler;

	uint						m_max_bounce = 3;
	bool						m_use_accumulation = true;
	bool						m_force_reset = false;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif


#pragma once

#ifndef NN_RENDER_POSTPROCESS_TAA_HPP
#define NN_RENDER_POSTPROCESS_TAA_HPP

#include"../core_impl.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//taa
///////////////////////////////////////////////////////////////////////////////////////////////////

class taa
{
public:

	struct params
	{
		uint2					screen_size;
		shader_resource_view*	src;
		shader_resource_view*	depth;
		shader_resource_view*	prev_depth;
		shader_resource_view*	velocity;
	};

	//コンストラクタ
	taa()
	{
		m_shader_file = gp_shader_manager->create(L"taa.sdf.json");
	}

	//適用
	bool apply(render_context& context, const params &params)
	{
		if(m_shader_file.has_update())
			m_shader_file.update();
		else if(m_shader_file.is_invalid())
			return false;

		const auto screen_size = params.screen_size;
		if((mp_history_tex[0] == nullptr) || (mp_history_tex[0]->width() != screen_size.x) || (mp_history_tex[0]->height() != screen_size.y))
		{
			mp_history_tex[0] = gp_render_device->create_texture2d(texture_format_r16g16b16a16_float, screen_size.x, screen_size.y, 1, resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access));
			mp_history_tex[1] = gp_render_device->create_texture2d(texture_format_r16g16b16a16_float, screen_size.x, screen_size.y, 1, resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access));
			mp_history_srv[0] = gp_render_device->create_shader_resource_view(*mp_history_tex[0], texture_srv_desc(*mp_history_tex[0]));
			mp_history_srv[1] = gp_render_device->create_shader_resource_view(*mp_history_tex[1], texture_srv_desc(*mp_history_tex[1]));
			mp_history_uav[0] = gp_render_device->create_unordered_access_view(*mp_history_tex[0], texture_uav_desc(*mp_history_tex[0]));
			mp_history_uav[1] = gp_render_device->create_unordered_access_view(*mp_history_tex[1], texture_uav_desc(*mp_history_tex[1]));
			gp_render_device->set_name(*mp_history_tex[0], L"taa: history_tex[0]");
			gp_render_device->set_name(*mp_history_tex[1], L"taa: history_tex[1]");
		}

		struct constant_buffer
		{
			float	update_rate;
			float	sigma_scale;
			float	depth_threshold;
			float	reserved;
		};
		auto p_cbuf = gp_render_device->create_temporary_cbuffer(sizeof(constant_buffer));
		auto &cbuf_data = *p_cbuf->data<constant_buffer>();
		cbuf_data.update_rate = m_update_rate;
		cbuf_data.sigma_scale = m_sigma_scale;
		cbuf_data.depth_threshold = m_depth_threshold;

		m_current_index = 1 - m_current_index;

		context.set_pipeline_resource("src", *params.src);
		context.set_pipeline_resource("dst", *mp_history_uav[m_current_index]);
		context.set_pipeline_resource("history", *mp_history_srv[1 - m_current_index]);
		context.set_pipeline_resource("depth", *params.depth);
		context.set_pipeline_resource("prev_depth", *params.prev_depth);
		context.set_pipeline_resource("velocity", *params.velocity);
		context.set_pipeline_resource("constant_buffer", *p_cbuf);
		context.set_pipeline_state(*m_shader_file.get("taa"));
		context.dispatch(ceil_div(params.screen_size.x, 16), ceil_div(params.screen_size.y, 16), 1);
		return true;
	}

	//結果を返す
	texture& result_tex() const { return *mp_history_tex[m_current_index]; }
	shader_resource_view& result_srv() const { return *mp_history_srv[m_current_index]; }

	//パラメータ
	float update_rate() const { return m_update_rate; }
	void set_update_rate(const float rate){ m_update_rate = rate; }
	float sigma_scale() const { return m_sigma_scale; }
	void set_sigma_scale(const float rate){ m_sigma_scale = rate; }
	float depth_threshold() const { return m_depth_threshold; }
	void set_depth_threshold(const float rate){ m_depth_threshold = rate; }

private:


	shader_file_holder			m_shader_file;
	texture_ptr					mp_history_tex[2];
	shader_resource_view_ptr	mp_history_srv[2];
	unordered_access_view_ptr	mp_history_uav[2];
	float						m_update_rate = 0.1f;
	float						m_sigma_scale = 0.5f;
	float						m_depth_threshold = 0.1f;
	uint						m_current_index = 0;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

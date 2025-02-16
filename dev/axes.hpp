
#pragma once

#ifndef NN_RENDER_DEV_AXES_HPP
#define NN_RENDER_DEV_AXES_HPP

#include"../base.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace dev{

///////////////////////////////////////////////////////////////////////////////////////////////////
//axes
///////////////////////////////////////////////////////////////////////////////////////////////////

class axes
{
public:

	//コンストラクタ
	axes()
	{
		m_shader_file = gp_shader_manager->create(L"dev/axes.sdf.json");
	}

	//描画
	bool draw(render_context& context, target_state& target_state, const camera& camera, const uint2 & screen_size)
	{
		if(m_shader_file.has_update())
			m_shader_file.update();
		else if(m_shader_file.is_invalid())
			return false;

		auto view_mat = float4x4(camera.view_mat());
		view_mat(0, 3) = 0;
		view_mat(1, 3) = 0;
		view_mat(2, 3) = 0;

		const float sz = -0.5f;
		const float tz = +0.5f;
		const float4x4 proj_mat(
			1, 0,  0,  0, 
			0, 1,  0,  0, 
			0, 0, sz, tz, 
			0, 0,  0,  1);

		const float3x4 view_proj_mat = float3x4(proj_mat * view_mat);
		auto p_cbuffer = gp_render_device->create_temporary_cbuffer(sizeof(view_proj_mat), &view_proj_mat);

		viewport viewport;
		viewport.left_top.x = screen_size.x - m_margin.x - m_size - 1;
		viewport.left_top.y = screen_size.y - m_margin.y - m_size - 1;
		viewport.size.x = m_size;
		viewport.size.y = m_size;

		push_viewport push_viewport(context);
		context.set_viewport(viewport);
		context.set_target_state(target_state);
		context.set_pipeline_state(*m_shader_file.get("draw_axes"));
		context.set_pipeline_resource("axes_cbuffer", *p_cbuffer);
		context.draw(2, 3, 0);
		return true;
	}

	//パラメータ
	const uint2& margin() const
	{
		return m_margin;
	}
	void set_margin(const uint2& margin)
	{
		m_margin = margin;
	}
	void set_size(const uint size)
	{
		m_size = size;
	}
	const uint2& size() const
	{
		return m_size;
	}

private:

	shader_file_holder	m_shader_file;
	uint2				m_margin		= 10;
	uint				m_size			= 64;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace dev

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

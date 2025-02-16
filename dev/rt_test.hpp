
#pragma once

#ifndef NN_RENDER_DEV_RT_TEST_HPP
#define NN_RENDER_DEV_RT_TEST_HPP

#include"../core.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace dev{

///////////////////////////////////////////////////////////////////////////////////////////////////
//rt_test
///////////////////////////////////////////////////////////////////////////////////////////////////

class rt_test
{
public:

	enum debug_view
	{
		debug_view_default, 
		debug_view_normal, 
		debug_view_tangent, 
		debug_view_binormal, 
		debug_view_uv,
	};

	//コンストラクタ
	rt_test()
	{
		m_shader_file = gp_shader_manager->create(L"dev/rt_test.sdf.json");
	}

	//描画
	bool draw(render_context& context, unordered_access_view& color_uav, unordered_access_view& depth_uav, const uint2 screen_size)
	{
		if(m_shader_file.has_update())
			m_shader_file.update();
		else if(m_shader_file.is_invalid())
			return false;

		const char* shader_names[] = {
			"view_default", 
			"view_normal", 
			"view_tangent", 
			"view_binormal", 
			"view_uv", 
		};

		context.set_pipeline_resource("color_uav", color_uav);
		context.set_pipeline_resource("depth_uav", depth_uav);
		context.set_pipeline_state(*m_shader_file.get(shader_names[m_debug_view]));
		context.dispatch_with_32bit_constant(ceil_div(screen_size.x, 8), ceil_div(screen_size.y, 4), 1, m_debug_view);
		return true;
	}

	//パラメータ
	void set_debug_view(const debug_view debug_view)
	{
		m_debug_view = debug_view;
	}
	debug_view debug_view() const
	{
		return decltype(debug_view_default)(m_debug_view);
	}

private:

	shader_file_holder	m_shader_file;
	int					m_debug_view = debug_view_default;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace dev

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

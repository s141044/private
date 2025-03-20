
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
		debug_view_geometry_normal, 
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

		if((mp_id_tex == nullptr) || (mp_id_tex->width() != screen_size.x) || (mp_id_tex->height() != screen_size.y))
		{
			mp_id_tex = gp_render_device->create_texture2d(texture_format_r32_uint, screen_size.x, screen_size.y, 1, resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access | resource_flag_scratch));
			mp_id_srv = gp_render_device->create_shader_resource_view(*mp_id_tex, texture_srv_desc(*mp_id_tex));
			mp_id_uav = gp_render_device->create_unordered_access_view(*mp_id_tex, texture_uav_desc(*mp_id_tex));
		}

		const char* shader_names[] = {
			"view_default", 
			"view_normal", 
			"view_geometry_normal", 
			"view_tangent", 
			"view_binormal", 
			"view_uv", 
		};

		context.set_pipeline_resource("id_uav", *mp_id_uav);
		context.set_pipeline_resource("color_uav", color_uav);
		context.set_pipeline_resource("depth_uav", depth_uav);
		context.set_pipeline_state(*m_shader_file.get(shader_names[m_debug_view]));
		context.dispatch_with_32bit_constant(ceil_div(screen_size.x, 8), ceil_div(screen_size.y, 4), 1, m_debug_view);

		if(m_debug_view == debug_view_default)
		{
			context.set_pipeline_resource("id_srv", *mp_id_srv);
			context.set_pipeline_state(*m_shader_file.get("draw_wire"));
			context.dispatch(ceil_div(screen_size.x, 16), ceil_div(screen_size.y, 16), 1);
		}
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

	shader_file_holder			m_shader_file;
	int							m_debug_view = debug_view_default;
	texture_ptr					mp_id_tex;
	shader_resource_view_ptr	mp_id_srv;
	unordered_access_view_ptr	mp_id_uav;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace dev

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

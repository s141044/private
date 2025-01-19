
#pragma once

#ifndef NN_RENDER_RENDERER_SSSSS_HPP
#define NN_RENDER_RENDERER_SSSSS_HPP

#include"../core_impl.hpp"
#include"../render_type.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//sssss
///////////////////////////////////////////////////////////////////////////////////////////////////

class sssss
{
public:

	struct params
	{
		uint2					screen_size;
		unordered_access_view*	p_result_uav; //これに結果を加算
		shader_resource_view*	p_diffuse_color_srv;
		shader_resource_view*	p_diffuse_result_srv;
		shader_resource_view*	p_subsurface_misc_srv;
		shader_resource_view*	p_subsurface_radius_srv;
		shader_resource_view*	p_subsurface_weight_srv;
		shader_resource_view*	p_normal_roughness_srv;
	};

	//コンストラクタ
	sssss()
	{
		m_shader_file = gp_shader_manager->create(L"test/sssss.sdf.json");
	}

	//実行
	bool execute(render_context &context, const params &params)
	{
		if(m_shader_file.has_update())
			m_shader_file.update();
		else if(m_shader_file.is_invalid())
			return false;

		context.set_pipeline_resource("result", *params.p_result_uav);
		context.set_pipeline_resource("diffuse_color", *params.p_diffuse_color_srv);
		context.set_pipeline_resource("diffuse_result", *params.p_diffuse_result_srv);
		context.set_pipeline_resource("subsurface_misc", *params.p_subsurface_misc_srv);
		context.set_pipeline_resource("normal_roughness", *params.p_normal_roughness_srv);
		context.set_pipeline_resource("subsurface_radius", *params.p_subsurface_radius_srv);
		context.set_pipeline_resource("subsurface_weight", *params.p_subsurface_weight_srv);
		context.set_pipeline_state(*m_shader_file.get("sssss"));
		context.dispatch(ceil_div(params.screen_size.x, 8), ceil_div(params.screen_size.y, 8), 1);
		return true;
	}

private:

	shader_file_holder	m_shader_file;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

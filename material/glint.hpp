
#pragma once

#ifndef NN_RENDER_MATERIAL_GLINT_HPP
#define NN_RENDER_MATERIAL_GLINT_HPP

#include"glint_coating.hpp"
#include"glint_specular.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//glint_material_resource
///////////////////////////////////////////////////////////////////////////////////////////////////

class glint_material_resource
{
public:

	//コンストラクタ
	glint_material_resource()
	{
		m_shaders = gp_shader_manager->create(L"material/glint.sdf.json");
	}

	//バインド
	bool bind(render_context &context)
	{
		bool update;
		if(update = m_shaders.has_update())
			m_shaders.update();
		else if(m_shaders.is_invalid())
			return false;

		if(mp_reflectance_tex == nullptr)
		{
			update = true;
			mp_reflectance_tex = gp_render_device->create_texture2d(texture_format_r16g16_unorm, 32, 32, 1, resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access));
			mp_reflectance_srv = gp_render_device->create_shader_resource_view(*mp_reflectance_tex, texture_srv_desc(*mp_reflectance_tex));
			mp_reflectance_uav = gp_render_device->create_unordered_access_view(*mp_reflectance_tex, texture_uav_desc(*mp_reflectance_tex));
			mp_sdf_dictionary_tex = gp_render_device->create_texture1d_array(texture_format_r16_float, 64, 16 * 128, 1, resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access));
			mp_sdf_dictionary_srv = gp_render_device->create_shader_resource_view(*mp_sdf_dictionary_tex, texture_srv_desc(*mp_sdf_dictionary_tex));
			mp_sdf_dictionary_uav = gp_render_device->create_unordered_access_view(*mp_sdf_dictionary_tex, texture_uav_desc(*mp_sdf_dictionary_tex));
			mp_cdf_dictionary_tex = gp_render_device->create_texture1d_array(texture_format_r16_unorm, 64 + 1, 16 * 128, 1, resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access));
			mp_cdf_dictionary_srv = gp_render_device->create_shader_resource_view(*mp_cdf_dictionary_tex, texture_srv_desc(*mp_cdf_dictionary_tex));
			mp_cdf_dictionary_uav = gp_render_device->create_unordered_access_view(*mp_cdf_dictionary_tex, texture_uav_desc(*mp_cdf_dictionary_tex));
			gp_render_device->set_name(*mp_reflectance_tex, L"glint_reflectance");
			gp_render_device->set_name(*mp_sdf_dictionary_tex, L"glint_sdf_dictionary");
			gp_render_device->set_name(*mp_cdf_dictionary_tex, L"glint_cdf_dictionary");
		}
		if(update)
		{
			push_priority push_priority(context);
			context.set_priority(priority_initiaize);
			context.set_pipeline_resource("reflectance_table", *mp_reflectance_uav);
			context.set_pipeline_state(*m_shaders.get("calc_reflectance"));
			context.dispatch(32, 32, 1);
			context.set_pipeline_resource("sdf_dictionary_uav", *mp_sdf_dictionary_uav);
			context.set_pipeline_resource("cdf_dictionary_uav", *mp_cdf_dictionary_uav);
			context.set_pipeline_state(*m_shaders.get("calc_dictionary"));
			context.dispatch(128, 1, 1);
		}
		context.set_pipeline_resource("glint_reflectance_table", *mp_reflectance_srv, true);
		context.set_pipeline_resource("glint_sdf_dictionary_srv", *mp_sdf_dictionary_srv, true);
		context.set_pipeline_resource("glint_cdf_dictionary_srv", *mp_cdf_dictionary_srv, true);
		return true;
	}

private:

	shader_file_holder			m_shaders;
	texture_ptr					mp_sdf_dictionary_tex;
	shader_resource_view_ptr	mp_sdf_dictionary_srv;
	unordered_access_view_ptr	mp_sdf_dictionary_uav;
	texture_ptr					mp_cdf_dictionary_tex;
	shader_resource_view_ptr	mp_cdf_dictionary_srv;
	unordered_access_view_ptr	mp_cdf_dictionary_uav;
	texture_ptr					mp_reflectance_tex;
	shader_resource_view_ptr	mp_reflectance_srv;
	unordered_access_view_ptr	mp_reflectance_uav;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

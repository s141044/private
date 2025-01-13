
#pragma once

#ifndef NN_RENDER_UTILITY_OUTLINE_HPP
#define NN_RENDER_UTILITY_OUTLINE_HPP

#include"../render_type.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//outline_renderer
///////////////////////////////////////////////////////////////////////////////////////////////////

class outline_renderer
{
public:

	//コンストラクタ
	outline_renderer()
	{
		m_shaders = gp_shader_manager->create(L"outline.sdf.json");
	}

	//描画
	void draw(render_context &context, render_entity &entity, unordered_access_view &dst, const uint2 dst_size, const float3 &color = float3(0.25f, 0.5f, 1.0f))
	{
		if(m_shaders.has_update())
			m_shaders.update();
		else if(m_shaders.is_invalid())
			return;

		if((mp_mask_tex[0] == nullptr) || (mp_mask_tex[0]->width() != dst_size.x) || (mp_mask_tex[0]->height() != dst_size.y))
		{
			mp_mask_tex[0] = gp_render_device->create_texture2d(texture_format_r8_unorm, dst_size.x, dst_size.y, 1, resource_flags(resource_flag_allow_render_target | resource_flag_allow_shader_resource | resource_flag_allow_unordered_access | resource_flag_scratch));
			mp_mask_rtv[0] = gp_render_device->create_render_target_view(*mp_mask_tex[0], texture_rtv_desc(*mp_mask_tex[0]));
			mp_mask_srv[0] = gp_render_device->create_shader_resource_view(*mp_mask_tex[0], texture_srv_desc(*mp_mask_tex[0]));
			mp_mask_uav[0] = gp_render_device->create_unordered_access_view(*mp_mask_tex[0], texture_uav_desc(*mp_mask_tex[0]));
			gp_render_device->set_name(*mp_mask_tex[0], L"outline_renderer: mask_tex[0]");

			mp_mask_tex[1] = gp_render_device->create_texture2d(texture_format_r8_unorm, dst_size.x, dst_size.y, 1, resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access | resource_flag_scratch));
			mp_mask_srv[1] = gp_render_device->create_shader_resource_view(*mp_mask_tex[1], texture_srv_desc(*mp_mask_tex[1]));
			mp_mask_uav[1] = gp_render_device->create_unordered_access_view(*mp_mask_tex[1], texture_uav_desc(*mp_mask_tex[1]));
			gp_render_device->set_name(*mp_mask_tex[1], L"outline_renderer: mask_tex[1]");

			render_target_view *p_rtv = mp_mask_rtv[0].get();
			mp_target = gp_render_device->create_target_state(1, &p_rtv, nullptr);

			mp_tiles_buf = gp_render_device->create_byteaddress_buffer(sizeof(uint) * ceil_div(dst_size.x, 16) * ceil_div(dst_size.y, 16), resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access));
			mp_tiles_srv = gp_render_device->create_shader_resource_view(*mp_tiles_buf, buffer_srv_desc(*mp_tiles_buf));
			mp_tiles_uav = gp_render_device->create_unordered_access_view(*mp_tiles_buf, buffer_uav_desc(*mp_tiles_buf));
			gp_render_device->set_name(*mp_tiles_buf, L"outline_renderer: tiles");

			uint4 vals[2] = { uint4(1, 1, 0, 0), uint4(1, 1, 0, 0) };
			mp_counter_buf = gp_render_device->create_byteaddress_buffer(sizeof(uint4) * 2, resource_flags(resource_flag_allow_unordered_access | resource_flag_allow_indirect_argument), vals);
			mp_counter_uav[0] = gp_render_device->create_unordered_access_view(*mp_counter_buf, buffer_uav_desc(*mp_counter_buf, 0));
			mp_counter_uav[1] = gp_render_device->create_unordered_access_view(*mp_counter_buf, buffer_uav_desc(*mp_counter_buf, 4));
			gp_render_device->set_name(*mp_counter_buf, L"outline_renderer: counter");
		}

		const float clear_color[4] = {};
		context.clear_render_target_view(*mp_mask_rtv[0], clear_color);

		context.set_target_state(*mp_target);
		entity.draw(context, draw_type_mask);

		struct constant_buffer
		{
			uint2	size;
			float2	inv_size;
			float3	color;
			int		offset;
		};
		auto p_cb = gp_render_device->create_temporary_cbuffer(sizeof(constant_buffer));
		auto *p_cb_data = p_cb->data<constant_buffer>();
		p_cb_data->size = dst_size;
		p_cb_data->inv_size.x = 1 / float(dst_size.x);
		p_cb_data->inv_size.y = 1 / float(dst_size.y);
		p_cb_data->color = color;
		p_cb_data->offset = 3;

		context.set_pipeline_resource("outline_cb", *p_cb);
		context.set_pipeline_resource("src", *mp_mask_srv[0]);
		context.set_pipeline_resource("dst", *mp_mask_uav[1]);
		context.set_pipeline_resource("tiles", *mp_tiles_uav);
		context.set_pipeline_resource("counter0", *mp_counter_uav[m_counter_index]);
		context.set_pipeline_resource("counter1", *mp_counter_uav[1 - m_counter_index]);
		context.set_pipeline_state(*m_shaders.get("draw_outline"));
		context.dispatch(ceil_div(dst_size.x, 16), ceil_div(dst_size.y, 16), 1);

		context.set_pipeline_resource("tiles", *mp_tiles_srv);
		context.set_pipeline_resource("src", *mp_mask_srv[1]);
		context.set_pipeline_resource("dst", dst);
		context.set_pipeline_state(*m_shaders.get("anti_aliasing"));
		context.dispatch(*mp_counter_buf, sizeof(uint4) * m_counter_index);
		m_counter_index = 1 - m_counter_index;
	}

private:

	shader_file_holder			m_shaders;
	target_state_ptr			mp_target;
	texture_ptr					mp_mask_tex[2];
	render_target_view_ptr		mp_mask_rtv[2];
	shader_resource_view_ptr	mp_mask_srv[2];
	unordered_access_view_ptr	mp_mask_uav[2];

	buffer_ptr					mp_tiles_buf;
	shader_resource_view_ptr	mp_tiles_srv;
	unordered_access_view_ptr	mp_tiles_uav;
	buffer_ptr					mp_counter_buf;
	unordered_access_view_ptr	mp_counter_uav[2];
	uint						m_counter_index = 0;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

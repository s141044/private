
#pragma once

#ifndef NN_RENDER_POSTPROCESS_TONEMAP_HPP
#define NN_RENDER_POSTPROCESS_TONEMAP_HPP

#include"../core_impl.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//tonemap
/*/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////*/

class tonemap
{
public:

	//コンストラクタ
	tonemap(const uint N = 3, const uint mip_levels = 5, const float base_key = 0.18f) : m_N(N), m_mip_levels(mip_levels), m_base_key(base_key)
	{
		assert(mip_levels > 0);
	}

	//
	float get_base_key() const { return m_base_key; }
	void set_base_key(const float base_key){ m_base_key = base_key; }

	//
	bool apply(render_context &context, shader_resource_view &src, unordered_access_view &dst, const uint width, const uint height, const float dt)
	{
		if(m_shader_file.is_empty())
			m_shader_file = gp_shader_manager->create(L"tonemap.sdf.json");
		if(m_shader_file.has_update())
			m_shader_file.update();
		else if(m_shader_file.is_invalid())
			return false;

		if((mp_tex[0][0] == nullptr) || (mp_tex[0][0]->width() != width) || (mp_tex[0][0]->height() != height))
		{
			for(uint i = 0, w = ceil_div(width, 32), h = ceil_div(height, 32); ; i++)
			{
				auto format = ((w == 1) && (h == 1)) ? texture_format_r16g16_float : texture_format_r32g32_float;
				mp_L_avg_max_tex[i] = gp_render_device->create_texture2d(format, w, h, 1, resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access));
				mp_L_avg_max_srv[i] = gp_render_device->create_shader_resource_view(*mp_L_avg_max_tex[i], texture_srv_desc(*mp_L_avg_max_tex[i]));
				mp_L_avg_max_uav[i] = gp_render_device->create_unordered_access_view(*mp_L_avg_max_tex[i], texture_uav_desc(*mp_L_avg_max_tex[i]));
				gp_render_device->set_name(*mp_L_avg_max_tex[i], L"tonemap: L_avg_max");

				if((w == 1) && (h == 1))
				{
					m_num_phases = i + 1;
					break;
				}

				w = ceil_div(w, 32);
				h = ceil_div(h, 32);
			}

			for(uint i = 0, w = width, h = height; i < m_mip_levels; i++)
			{
				if((w == 1) && (h == 1))
					m_mip_levels = i + 1;

				mp_tex[0][i] = gp_render_device->create_texture2d_array(texture_format_r16g16b16a16_float, w, h, m_N, 1, resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access | resource_flag_scratch));
				mp_srv[0][i] = gp_render_device->create_shader_resource_view(*mp_tex[0][i], texture_srv_desc(*mp_tex[0][i]));
				mp_uav[0][i] = gp_render_device->create_unordered_access_view(*mp_tex[0][i], texture_uav_desc(*mp_tex[0][i]));
				gp_render_device->set_name(*mp_tex[0][i], L"tonemap::tex[0]");

				if(i + 1 != m_mip_levels)
				{
					mp_tex[1][i] = gp_render_device->create_texture2d_array(texture_format_r16g16b16a16_float, w, h, m_N, 1, resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access | resource_flag_scratch));
					mp_srv[1][i] = gp_render_device->create_shader_resource_view(*mp_tex[1][i], texture_srv_desc(*mp_tex[1][i]));
					mp_uav[1][i] = gp_render_device->create_unordered_access_view(*mp_tex[1][i], texture_uav_desc(*mp_tex[1][i]));
					gp_render_device->set_name(*mp_tex[1][i], L"tonemap::tex[1]");
				}
				else
				{
					mp_tex[1][i] = mp_tex[0][i];
					mp_srv[1][i] = mp_srv[0][i];
					mp_uav[1][i] = mp_uav[0][i];
				}

				mp_tex[2][i] = gp_render_device->create_texture2d_array(texture_format_r16g16b16a16_float, w, h, 1, 1, resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access | resource_flag_scratch));
				mp_srv[2][i] = gp_render_device->create_shader_resource_view(*mp_tex[2][i], texture_srv_desc(*mp_tex[2][i]));
				mp_uav[2][i] = gp_render_device->create_unordered_access_view(*mp_tex[2][i], texture_uav_desc(*mp_tex[2][i]));
				gp_render_device->set_name(*mp_tex[1][i], L"tonemap::tex[2]");

				w = ceil_div(w, 2);
				h = ceil_div(h, 2);
			}
		}

		{
			constant_buffer cb;
			cb.base_key = m_base_key;
			cb.ws = 1;
			cb.wc = 1;
			cb.we = 0;
			cb.N = m_N;

			//1秒後にaだけ現在の値が失われる
			const float a = 0.1f;
			cb.blend_ratio = pow(1 / a, -dt / 1000);

			auto p_cb = gp_render_device->create_temporary_cbuffer(sizeof(constant_buffer), &cb);
			context.set_pipeline_resource("tonemap_cbuffer", *p_cb);
		}

		shader_resource_view *p_L_avg_max_srv;
		for(uint i = 0, w = width, h = height; ; i++)
		{
			if(i == 0)
			{
				context.set_pipeline_state(*m_shader_file.get("calc_scene_info_first"));
				context.set_pipeline_resource("src", src);
				context.set_pipeline_resource("dst", *mp_L_avg_max_uav[i]);
			}
			else if((w > 32) || (h > 32))
			{
				context.set_pipeline_state(*m_shader_file.get("calc_scene_info"));
				context.set_pipeline_resource("src", *mp_L_avg_max_srv[i - 1]);
				context.set_pipeline_resource("dst", *mp_L_avg_max_uav[i]);
			}
			else
			{
				context.set_pipeline_state(*m_shader_file.get("calc_scene_info_last"));
				context.set_pipeline_resource("src", *mp_L_avg_max_srv[i - 1]);
				context.set_pipeline_resource("dst", *mp_L_avg_max_uav[i]);
			}
			
			context.dispatch(w = ceil_div(w, 32), h = ceil_div(h, 32), 1);
			if((w == 1) && (h == 1))
			{
				p_L_avg_max_srv = mp_L_avg_max_srv[i].get();
				break;
			}
		}

		//グローバルトーンマッピング
		if(m_N == 1)
		{
			context.set_pipeline_resource("src_dst", dst);
			context.set_pipeline_resource("L_avg_max", *p_L_avg_max_srv);
			context.set_pipeline_state(*m_shader_file.get("global_tonemapping"));
			context.dispatch(ceil_div(width, 16), ceil_div(height, 16), m_N);
			return true;
		}

		//トーンマッピング＆重み計算
		context.set_pipeline_resource("src", src);
		context.set_pipeline_resource("dst", *mp_uav[0][0]);
		context.set_pipeline_resource("L_avg_max", *p_L_avg_max_srv);
		context.set_pipeline_state(*m_shader_file.get("tonemapping_and_weighting"));
		context.dispatch(ceil_div(width, 16), ceil_div(height, 16), m_N);

		//ダウンサンプリング
		for(uint i = 1; i < m_mip_levels; i++)
		{
			context.set_pipeline_resource("src", *mp_srv[0][i - 1]);
			context.set_pipeline_resource("dst", *mp_uav[0][i]);
			context.set_pipeline_state(*m_shader_file.get("downward"));
			context.dispatch(ceil_div(width >> (i - 1), 16), ceil_div(height >> (i - 1), 16), m_N);
		}

		//アップサンプリング
		for(uint i = 0; i < m_mip_levels - 1; i++)
		{
			context.set_pipeline_resource("src", *mp_srv[0][i]);
			context.set_pipeline_resource("dst", *mp_uav[1][i]);
			context.set_pipeline_resource("up_src", *mp_srv[0][i + 1]);
			context.set_pipeline_state(*m_shader_file.get("upward"));
			context.dispatch(ceil_div(width >> i, 16), ceil_div(height >> i, 16), m_N);
		}

		//ブレンド
		for(uint i = 0; i < m_mip_levels; i++)
		{
			context.set_pipeline_resource("src", *mp_srv[1][i]);
			context.set_pipeline_resource("dst", *mp_uav[2][i]);
			context.set_pipeline_state(*m_shader_file.get("blend"));
			context.dispatch(ceil_div(width >> i, 16), ceil_div(height >> i, 16), 1);
		}

		//コラプス
		for(uint i = m_mip_levels - 1; i > 0; i--)
		{
			if(i == 1)
				context.set_pipeline_resource("dst", dst);
			else
				context.set_pipeline_resource("dst", *mp_uav[2][i - 1]);
	
			context.set_pipeline_resource("up_src", *mp_srv[2][i]);
			context.set_pipeline_state(*m_shader_file.get("collapse"));
			context.dispatch(ceil_div(width >> (i - 1), 16), ceil_div(height >> (i - 1), 16), 1);
		}
		return true;
	}

private:

	struct constant_buffer
	{
		uint2	size;
		float2	inv_size;

		float	base_key;
		float	ws;
		float	wc;
		float	we;

		uint	N;
		float	blend_ratio;
		uint2	reserved;
	};

private:

	uint						m_N;
	uint						m_mip_levels;
	float						m_base_key;

	uint						m_phase;
	uint						m_num_phases;
	texture_ptr					mp_L_avg_max_tex[4];
	shader_resource_view_ptr	mp_L_avg_max_srv[4];
	unordered_access_view_ptr	mp_L_avg_max_uav[4];

	texture_ptr					mp_tex[3][12];
	shader_resource_view_ptr	mp_srv[3][12];
	unordered_access_view_ptr	mp_uav[3][12];
	shader_file_holder			m_shader_file;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

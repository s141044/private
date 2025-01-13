
#pragma once

#ifndef NN_RENDER_UTILITY_SORT_HPP
#define NN_RENDER_UTILITY_SORT_HPP

#include"../core_impl.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//sorter
/*/////////////////////////////////////////////////////////////////////////////////////////////////
Onesweep: A Faster Least Significant Digit Radix Sort for GPUs
/////////////////////////////////////////////////////////////////////////////////////////////////*/

class sorter
{
public:

	//コンストラクタ
	sorter()
	{
		m_shaders = gp_shader_manager->create(L"sort.sdf.json");
	}

	//ソート
	bool sort(render_context &context, shader_resource_view &src, unordered_access_view &dst, const uint num_elems, const uint bit_count = 32)
	{
		if(m_shaders.has_update())
			m_shaders.update();
		else if(m_shaders.is_invalid())
			return false;

		const uint K = 5;	//1ループで処理するビット数.1<<Kが32を超えたらダメ
		const uint M = 16;	//1スレッドが処理する数.どれくらいが適切か分からん...
		const uint radix = 1 << K;
		const uint num_threads = 256;
		const uint tile_size = num_threads * M;
		const uint num_tiles = ceil_div(num_elems, tile_size);
		const uint num_loops = ceil_div(bit_count, K);

		if(mp_tile_index_buf == nullptr)
		{
			mp_tile_index_buf = gp_render_device->create_byteaddress_buffer(sizeof(uint), resource_flags(resource_flag_allow_unordered_access | resource_flag_scratch));
			mp_tile_index_uav = gp_render_device->create_unordered_access_view(*mp_tile_index_buf, buffer_uav_desc(*mp_tile_index_buf));
		}
		if((mp_tile_info_buf == nullptr) || (mp_tile_info_buf->num_elements() != num_tiles * radix))
		{
			mp_tile_info_buf = gp_render_device->create_byteaddress_buffer(sizeof(uint) * num_tiles * radix, resource_flags(resource_flag_allow_unordered_access | resource_flag_scratch));
			mp_tile_info_uav = gp_render_device->create_unordered_access_view(*mp_tile_info_buf, buffer_uav_desc(*mp_tile_info_buf));
		}
		if((mp_global_count_buf == nullptr) || (mp_global_count_buf->num_elements() != num_loops * radix))
		{
			mp_global_count_buf = gp_render_device->create_byteaddress_buffer(sizeof(uint) * num_loops * radix, resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access | resource_flag_scratch));
			mp_global_count_srv = gp_render_device->create_shader_resource_view(*mp_global_count_buf, buffer_srv_desc(*mp_global_count_buf));
			mp_global_count_uav = gp_render_device->create_unordered_access_view(*mp_global_count_buf, buffer_uav_desc(*mp_global_count_buf));
		}

		static buffer_ptr debug_buf_ptr[4];
		static unordered_access_view_ptr debug_uav_ptr[4];
		if(debug_buf_ptr[0] == nullptr)
		{
			debug_buf_ptr[0] = gp_render_device->create_byteaddress_buffer(sizeof(uint) * 1024 * 256, resource_flags(resource_flag_allow_unordered_access | resource_flag_scratch));
			debug_uav_ptr[0] = gp_render_device->create_unordered_access_view(*debug_buf_ptr[0], buffer_uav_desc(*debug_buf_ptr[0]));
			debug_buf_ptr[1] = gp_render_device->create_byteaddress_buffer(sizeof(uint) * 1024 * 256, resource_flags(resource_flag_allow_unordered_access | resource_flag_scratch));
			debug_uav_ptr[1] = gp_render_device->create_unordered_access_view(*debug_buf_ptr[1], buffer_uav_desc(*debug_buf_ptr[1]));
			debug_buf_ptr[2] = gp_render_device->create_byteaddress_buffer(sizeof(uint) * 1024 * 256, resource_flags(resource_flag_allow_unordered_access | resource_flag_scratch));
			debug_uav_ptr[2] = gp_render_device->create_unordered_access_view(*debug_buf_ptr[2], buffer_uav_desc(*debug_buf_ptr[2]));
			debug_buf_ptr[3] = gp_render_device->create_byteaddress_buffer(sizeof(uint) * 1024 * 256, resource_flags(resource_flag_allow_unordered_access | resource_flag_scratch));
			debug_uav_ptr[3] = gp_render_device->create_unordered_access_view(*debug_buf_ptr[3], buffer_uav_desc(*debug_buf_ptr[3]));
		}
		context.set_pipeline_resource("debug0", *debug_uav_ptr[0]);
		context.set_pipeline_resource("debug1", *debug_uav_ptr[1]);
		context.set_pipeline_resource("debug2", *debug_uav_ptr[2]);
		context.set_pipeline_resource("debug3", *debug_uav_ptr[3]);
		{
			uint clear_value[4] = {};
			context.clear_unordered_access_view(*debug_uav_ptr[0], clear_value);
			context.clear_unordered_access_view(*debug_uav_ptr[1], clear_value);
			context.clear_unordered_access_view(*debug_uav_ptr[2], clear_value);
			context.clear_unordered_access_view(*debug_uav_ptr[3], clear_value);
		}

		struct cbuffer
		{
			uint	num_elems;
			uint	num_tiles;
			uint2	reserved;
		};
		cbuffer cbuffer = { num_elems, num_tiles };
		context.set_pipeline_resource("sort_cb", *gp_render_device->create_temporary_cbuffer(sizeof(cbuffer), &cbuffer));

		uint clear_value[4] = {};
		context.clear_unordered_access_view(*mp_tile_info_uav, clear_value);
		context.clear_unordered_access_view(*mp_tile_index_uav, clear_value);
		context.clear_unordered_access_view(*mp_global_count_uav, clear_value);

		context.set_pipeline_resource("src", src);
		context.set_pipeline_resource("counter", *mp_global_count_uav);
		context.set_pipeline_state(*m_shaders.get("global_count"));
		context.dispatch(num_tiles, 1, 1);

		context.set_pipeline_resource("counter", *mp_global_count_uav);
		context.set_pipeline_state(*m_shaders.get("global_scan"));
		context.dispatch(1, num_loops, 1);
		
		for(uint i = 0; i < num_loops; i++)
		{
			context.set_pipeline_resource("elems", dst);
			context.set_pipeline_resource("counter", *mp_global_count_srv);
			context.set_pipeline_resource("tile_info", *mp_tile_info_uav);
			context.set_pipeline_resource("tile_index", *mp_tile_index_uav);
			context.set_pipeline_state(*m_shaders.get("reorder"));
			context.dispatch_with_32bit_constant(num_tiles, 1, 1, i);
		}
		return true;
	}

private:

	shader_file_holder			m_shaders;
	buffer_ptr					mp_tile_index_buf;
	unordered_access_view_ptr	mp_tile_index_uav;
	buffer_ptr					mp_tile_info_buf;
	shader_resource_view_ptr	mp_tile_info_srv;
	unordered_access_view_ptr	mp_tile_info_uav;
	buffer_ptr					mp_global_count_buf;
	shader_resource_view_ptr	mp_global_count_srv;
	unordered_access_view_ptr	mp_global_count_uav;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

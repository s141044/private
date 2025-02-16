
#pragma once

#ifndef NN_RENDER_CORE_COMMAND_HPP
#define NN_RENDER_CORE_COMMAND_HPP

#include<memory>
#include<vector>
#include<cassert>
#include<functional>

#include"utility.hpp"
#include"allocator.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//前方宣言
///////////////////////////////////////////////////////////////////////////////////////////////////

class buffer;
class texture;
class upload_buffer;
class readback_buffer;
class constant_buffer;
class render_target_view;
class depth_stencil_view;
class shader_resource_view;
class unordered_access_view;
class target_state;
class geometry_state;
class pipeline_state;
class top_level_acceleration_structure;
class bottom_level_acceleration_structure;

///////////////////////////////////////////////////////////////////////////////////////////////////

using uint = std::uint32_t;

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace command{

///////////////////////////////////////////////////////////////////////////////////////////////////
//type
///////////////////////////////////////////////////////////////////////////////////////////////////

enum type
{
	type_copy_buffer,
	type_copy_texture,
	type_copy_resource,
	type_update_buffer, 
	type_draw,
	type_draw_indirect,
	type_draw_indexed,
	type_draw_indexed_indirect,
	type_dispatch,
	type_dispatch_indirect,
	type_clear_render_target_view,
	type_clear_depth_stencil_view,
	type_clear_unordered_access_view_float,
	type_clear_unordered_access_view_uint,
	type_build_top_level_acceleration_structure,
	type_refit_top_level_acceleration_structure,
	type_build_bottom_level_acceleration_structure,
	type_refit_bottom_level_acceleration_structure,
	type_compact_bottom_level_acceleration_structure,
	type_present,

	//外部ライブラリ用
	type_call_function,
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//base
/*/////////////////////////////////////////////////////////////////////////////////////////////////
デストラクタ呼び出しは行われない
/////////////////////////////////////////////////////////////////////////////////////////////////*/

struct base
{
	base(type type, uint priority) : type(type), priority(priority)
	{
	}
	type	type;
	uint	priority;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//manager
/*/////////////////////////////////////////////////////////////////////////////////////////////////
8byteアライメントにしとく．
/////////////////////////////////////////////////////////////////////////////////////////////////*/

class manager
{
public:

	//コンストラクタ
	manager(const size_t size) : m_buf(ceil_div(size, 8)), m_allocator(m_buf.size()), m_cur(), m_size(size)
	{
		m_commands[0].resize(ceil_div(size, sizeof(base))); //必要以上に確保してるけどまあいいか...
		m_commands[1].resize(ceil_div(size, sizeof(base)));
	}

	//コマンド作成
	template<class Command, class... Args> Command* construct(Args&&... args)
	{
		static_assert(alignof(Command) == 8);
		const size_t pos = m_allocator.allocate(sizeof(Command));
		auto* p_command = new (&m_buf[pos]) Command(std::forward<Args>(args)...);
		m_commands[m_cur][m_idx++] = p_command;
		return p_command;
	}

	//任意長の割り当て
	void* allocate(const size_t size)
	{
		const size_t pos = m_allocator.allocate(ceil_div(size, 8));
		return &m_buf[pos];
	}
	template<class T, class... Args> T* allocate(Args&&... args)
	{
		return new(allocate(sizeof(T))) T(std::forward<Args>(args)...);
	}

	//中間コマンドリストをソートして返す
	const std::vector<base*>& sort_and_get()
	{
		auto& commands = m_commands[m_cur];
		m_cur = 1 - m_cur;

		commands.resize(m_idx);
		std::stable_sort(commands.begin(), commands.end(), [&](const base* a, const base* b){ return (a->priority < b->priority); });

		m_idx = 0;
		m_allocator.dellocate();
		m_commands[m_cur].resize(ceil_div(m_size, sizeof(base)));
		return commands;
	}

private:

	std::vector<uint64_t>	m_buf;
	lockfree_ring_allocator	m_allocator;
	std::vector<base*>		m_commands[2];
	std::atomic_size_t		m_idx;
	size_t					m_cur;
	size_t					m_size;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//draw
///////////////////////////////////////////////////////////////////////////////////////////////////

struct draw : base
{
	draw(const uint priority, const uint vertex_count, const uint instance_count, const uint start_vertex_location, const uint constant, const bool uav_barrier, const uint stencil, const float* viewport, const pipeline_state& pipeline_state, const target_state& target_state, const geometry_state* p_geometry_state, void* p_optional) : base(type_draw, priority), vertex_count(vertex_count), instance_count(instance_count), start_vertex_location(start_vertex_location), constant(constant), uav_barrier(uav_barrier), stencil(stencil), viewport{viewport[0], viewport[1], viewport[2], viewport[3]}, p_target_state(&target_state), p_geometry_state(p_geometry_state), p_pipeline_state(&pipeline_state), p_optional(p_optional)
	{
	}
	uint					vertex_count;
	uint					instance_count;
	uint					start_vertex_location;
	uint					constant;
	uint					stencil;
	float					viewport[4];
	bool					uav_barrier;
	const target_state*		p_target_state;
	const geometry_state*	p_geometry_state;
	const pipeline_state*	p_pipeline_state;
	void*					p_optional;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//draw_indexed
///////////////////////////////////////////////////////////////////////////////////////////////////

struct draw_indexed : base
{
	draw_indexed(const uint priority, const uint index_count, const uint instance_count, const uint start_index_location, const uint base_vertex_location, const uint constant, const bool uav_barrier, const uint stencil, const float* viewport, const pipeline_state& pipeline_state, const target_state& target_state, const geometry_state* p_geometry_state, void* p_optional) : base(type_draw_indexed, priority), index_count(index_count), instance_count(instance_count), start_index_location(start_index_location), base_vertex_location(base_vertex_location), constant(constant), uav_barrier(uav_barrier), stencil(stencil), viewport{viewport[0], viewport[1], viewport[2], viewport[3]}, p_target_state(&target_state), p_geometry_state(p_geometry_state), p_pipeline_state(&pipeline_state), p_optional(p_optional)
	{
	}
	uint					index_count;
	uint					instance_count;
	uint					start_index_location;
	uint					base_vertex_location;
	uint					constant;
	uint					stencil;
	float					viewport[4];
	bool					uav_barrier;
	const target_state*		p_target_state;
	const geometry_state*	p_geometry_state;
	const pipeline_state*	p_pipeline_state;
	void*					p_optional;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//dispatch
///////////////////////////////////////////////////////////////////////////////////////////////////

struct dispatch : base
{
	dispatch(const uint priority, const uint x, const uint y, const uint z, const uint constant, const bool uav_barrier, const pipeline_state& pipeline_state, void* p_optional) : base(type_dispatch, priority), x(x), y(y), z(z), constant(constant), uav_barrier(uav_barrier), p_pipeline_state(&pipeline_state), p_optional(p_optional)
	{
	}
	uint					x;
	uint					y;
	uint					z;
	uint					constant;
	bool					uav_barrier;
	const pipeline_state*	p_pipeline_state;
	void*					p_optional;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//draw_indirect
///////////////////////////////////////////////////////////////////////////////////////////////////

struct draw_indirect : base
{
	draw_indirect(const uint priority, buffer& buf, const size_t offset, const uint constant, const bool uav_barrier, const uint stencil, const float* viewport, const pipeline_state& pipeline_state, const target_state& target_state, const geometry_state* p_geometry_state, void* p_optional) : base(type_draw_indirect, priority), p_buf(&buf), offset(offset), constant(constant), uav_barrier(uav_barrier), stencil(stencil), viewport{viewport[0], viewport[1], viewport[2], viewport[3]}, p_target_state(&target_state), p_geometry_state(p_geometry_state), p_pipeline_state(&pipeline_state), p_optional(p_optional)
	{
	}
	buffer*					p_buf;
	size_t					offset;
	uint					constant;
	uint					stencil;
	float					viewport[4];
	bool					uav_barrier;
	const target_state*		p_target_state;
	const geometry_state*	p_geometry_state;
	const pipeline_state*	p_pipeline_state;
	void*					p_optional;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//draw_indexed_indirect
///////////////////////////////////////////////////////////////////////////////////////////////////

struct draw_indexed_indirect : base
{
	draw_indexed_indirect(const uint priority, buffer& buf, const size_t offset, const uint constant, const bool uav_barrier, const uint stencil, const float* viewport, const pipeline_state& pipeline_state, const target_state& target_state, const geometry_state* p_geometry_state, void* p_optional) : base(type_draw_indexed_indirect, priority), p_buf(&buf), offset(offset), constant(constant), uav_barrier(uav_barrier), stencil(stencil), viewport{viewport[0], viewport[1], viewport[2], viewport[3]}, p_target_state(&target_state), p_geometry_state(p_geometry_state), p_pipeline_state(&pipeline_state), p_optional(p_optional)
	{
	}
	buffer*					p_buf;
	size_t					offset;
	uint					constant;
	uint					stencil;
	float					viewport[4];
	bool					uav_barrier;
	const target_state*		p_target_state;
	const geometry_state*	p_geometry_state;
	const pipeline_state*	p_pipeline_state;
	void*					p_optional;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//dispatch_indirect
///////////////////////////////////////////////////////////////////////////////////////////////////

struct dispatch_indirect : base
{
	dispatch_indirect(const uint priority, buffer& buf, const size_t offset, const uint constant, const bool uav_barrier, const pipeline_state& pipeline_state, void* p_optional) : base(type_dispatch_indirect, priority), p_buf(&buf), offset(offset), constant(constant), uav_barrier(uav_barrier), p_pipeline_state(&pipeline_state), p_optional(p_optional)
	{
	}
	buffer*					p_buf;
	size_t					offset;
	uint					constant;
	bool					uav_barrier;
	const pipeline_state*	p_pipeline_state;
	void*					p_optional;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//copy_buffer
///////////////////////////////////////////////////////////////////////////////////////////////////

struct copy_buffer : base
{
	copy_buffer(const uint priority, buffer& dst, const size_t dst_offset, buffer& src, const uint src_offset, const uint size) : base(type_copy_buffer, priority), p_dst(&dst), p_src(&src), dst_offset(dst_offset), src_offset(src_offset), size(size), dst_is_readback(0), src_is_upload(0)
	{
	}
	copy_buffer(const uint priority, buffer& dst, const size_t dst_offset, upload_buffer& src, const uint src_offset, const uint size) : base(type_copy_buffer, priority), p_dst(&dst), p_src(&src), dst_offset(dst_offset), src_offset(src_offset), size(size), dst_is_readback(0), src_is_upload(1)
	{
	}
	copy_buffer(const uint priority, readback_buffer& dst, const size_t dst_offset, buffer& src, const uint src_offset, const uint size) : base(type_copy_buffer, priority), p_dst(&dst), p_src(&src), dst_offset(dst_offset), src_offset(src_offset), size(size), dst_is_readback(1), src_is_upload(0)
	{
	}
	void*	p_dst;
	void*	p_src;
	size_t	dst_offset;
	size_t	src_offset;
	size_t	size : 62;
	size_t	dst_is_readback : 1;
	size_t	src_is_upload : 1;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//copy_texture
///////////////////////////////////////////////////////////////////////////////////////////////////

struct copy_texture : base
{
	copy_texture(const uint priority, texture& dst, texture& src) : base(type_copy_texture, priority), p_dst(&dst), p_src(&src)
	{
	}
	texture*	p_dst;
	texture*	p_src;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//copy_resource
///////////////////////////////////////////////////////////////////////////////////////////////////

struct copy_resource : base
{
	copy_resource(const uint priority, buffer& dst, buffer& src) : base(type_copy_buffer, priority), p_dst(&dst), p_src(&src)
	{
	}
	copy_resource(const uint priority, buffer& dst, upload_buffer& src) : base(type_copy_buffer, priority), p_dst(&dst), p_src(&src)
	{
	}
	void*	p_dst;
	void*	p_src;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//update_buffer
///////////////////////////////////////////////////////////////////////////////////////////////////

struct update_buffer : base
{
	update_buffer(const uint priority, buffer& buf, const size_t offset, const size_t size, const void* p_update) : base(type_update_buffer, priority), p_buf(&buf), p_update(p_update), offset(offset), size(size)
	{
	}
	buffer*		p_buf;
	size_t		offset;
	size_t		size;
	const void*	p_update;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//clear_render_target_view
///////////////////////////////////////////////////////////////////////////////////////////////////

struct clear_render_target_view : base
{
	clear_render_target_view(const uint priority, render_target_view& rtv, const float color[4]) : base(type_clear_render_target_view, priority), p_rtv(&rtv)
	{
		memcpy(this->color, color, sizeof(float) * 4);
	}
	render_target_view*	p_rtv;
	float				color[4];
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//clear_flags
///////////////////////////////////////////////////////////////////////////////////////////////////

enum clear_flags
{
	clear_flag_depth = 1, 
	clear_flag_stencil = 2,
	clear_flag_depth_stencil = 3,
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//clear_depth_stencil_view
///////////////////////////////////////////////////////////////////////////////////////////////////

struct clear_depth_stencil_view : base
{
	clear_depth_stencil_view(const uint priority, depth_stencil_view& dsv, const clear_flags flags, const float depth, const uint stencil) : base(type_clear_depth_stencil_view, priority), p_dsv(&dsv), flags(flags), depth(depth), stencil(stencil)
	{
	}
	depth_stencil_view*	p_dsv;
	float				depth;
	uint				stencil;
	clear_flags			flags;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//clear_unordered_access_view_float
///////////////////////////////////////////////////////////////////////////////////////////////////

struct clear_unordered_access_view_float : base
{
	clear_unordered_access_view_float(const uint priority, unordered_access_view& uav, const float value[4], const bool uav_barrier) : base(type_clear_unordered_access_view_float, priority), p_uav(&uav), uav_barrier(uav_barrier)
	{
		memcpy(this->value, value, sizeof(float) * 4);
	}
	unordered_access_view*	p_uav;
	float					value[4];
	bool					uav_barrier;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//clear_unordered_access_view_uint
///////////////////////////////////////////////////////////////////////////////////////////////////

struct clear_unordered_access_view_uint : base
{
	clear_unordered_access_view_uint(const uint priority, unordered_access_view& uav, const uint value[4], const bool uav_barrier) : base(type_clear_unordered_access_view_uint, priority), p_uav(&uav), uav_barrier(uav_barrier)
	{
		memcpy(this->value, value, sizeof(uint) * 4);
	}
	unordered_access_view*	p_uav;
	uint					value[4];
	bool					uav_barrier;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//build_top_level_acceleration_structure
///////////////////////////////////////////////////////////////////////////////////////////////////

struct build_top_level_acceleration_structure : base
{
	build_top_level_acceleration_structure(const uint priority, top_level_acceleration_structure& tlas, const uint num_instances) : base(type_build_top_level_acceleration_structure, priority), p_tlas(&tlas), num_instances(num_instances)
	{
	}
	top_level_acceleration_structure*	p_tlas;
	uint								num_instances;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//build_bottom_level_acceleration_structure
///////////////////////////////////////////////////////////////////////////////////////////////////

struct build_bottom_level_acceleration_structure : base
{
	build_bottom_level_acceleration_structure(const uint priority, bottom_level_acceleration_structure& blas, const geometry_state* p_gs) : base(type_build_bottom_level_acceleration_structure, priority), p_blas(&blas), p_gs(p_gs)
	{
	}
	bottom_level_acceleration_structure*	p_blas;
	const geometry_state*					p_gs;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//refit_bottom_level_acceleration_structure
///////////////////////////////////////////////////////////////////////////////////////////////////

struct refit_bottom_level_acceleration_structure : base
{
	refit_bottom_level_acceleration_structure(const uint priority, bottom_level_acceleration_structure& blas, const geometry_state* p_gs) : base(type_refit_bottom_level_acceleration_structure, priority), p_blas(&blas), p_gs(p_gs)
	{
	}
	bottom_level_acceleration_structure*	p_blas;
	const geometry_state*					p_gs;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//compact_bottom_level_acceleration_structure
///////////////////////////////////////////////////////////////////////////////////////////////////

struct compact_bottom_level_acceleration_structure : base
{
	compact_bottom_level_acceleration_structure(const uint priority, bottom_level_acceleration_structure& blas) : base(type_compact_bottom_level_acceleration_structure, priority), p_blas(&blas)
	{
	}
	bottom_level_acceleration_structure* p_blas;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//present
///////////////////////////////////////////////////////////////////////////////////////////////////

struct present : base
{
	present(texture& tex) : base(type_present, UINT_MAX), p_tex(&tex)
	{
	}
	texture* p_tex;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//call_function
///////////////////////////////////////////////////////////////////////////////////////////////////

struct call_function : base
{
	call_function(const uint priority, std::function<void()> func, const target_state* p_target_state = nullptr) : base(type_call_function, priority), p_target_state(p_target_state), func(std::move(func))
	{
	}
	const target_state*		p_target_state;
	std::function<void()>	func;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace command

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

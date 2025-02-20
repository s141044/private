
#pragma once

#ifndef NN_RENDER_CORE_RENDER_CONTEXT_HPP
#define NN_RENDER_CORE_RENDER_CONTEXT_HPP

#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX

#include"render_device.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//render_context
/*/////////////////////////////////////////////////////////////////////////////////////////////////
中間コマンドをためる.
///////////////////////////////////////////////////////////////////////////////////////////////////
リソースのバインド数が一定数を超えると古いものから削除.set_global_...で設定したものは削除されない.
/////////////////////////////////////////////////////////////////////////////////////////////////*/

class render_context
{
public:

	//コンストラクタ
	render_context(iface::render_device& device);

	//優先度
	uint get_priority() const;
	void set_priority(uint priority);

	//リソースを設定
	template<class Resource>
	void set_pipeline_resource(string name, Resource& resource, const bool global = false);

	//ステート
	const target_state* get_target_state() const;
	const pipeline_state* get_pipeline_state() const;
	const geometry_state* get_geometry_state() const;
	void set_target_state(const render::target_state& ts);
	void set_pipeline_state(const render::pipeline_state& ps);
	void set_geometry_state(const render::geometry_state& gs);
	void set_geometry_state(std::nullptr_t);

	//ステンシル値
	uint get_stencil_value() const;
	void set_stencil_value(const uint stencil);

	//ビューポート
	const viewport& get_viewport() const;
	void set_viewport(const render::viewport& viewport);

	//描画
	void draw(const uint vertex_count, const uint instance_count, const uint start_vertex_location, const bool uav_barrier = true);
	void draw(buffer& buf, const uint offset, const bool uav_barrier = true);
	void draw_with_32bit_constant(const uint vertex_count, const uint instance_count, const uint start_vertex_location, const uint constant, const bool uav_barrier = true);
	void draw_with_32bit_constant(buffer& buf, const uint offset, const uint constant, const bool uav_barrier = true);
	void draw_indexed(const uint index_count, const uint instance_count, const uint start_index_location, const uint base_vertex_location, const bool uav_barrier = true);
	void draw_indexed(buffer& buf, const uint offset, const bool uav_barrier = true);
	void draw_indexed_with_32bit_constant(const uint index_count, const uint instance_count, const uint start_index_location, const uint base_vertex_location, const uint constant, const bool uav_barrier = true);
	void draw_indexed_with_32bit_constant(buffer& buf, const uint offset, const uint constant, const bool uav_barrier = true);

	//ディスパッチ
	void dispatch(const uint x, const uint y, const uint z, const bool uav_barrier = true);
	void dispatch(buffer& buf, const uint offset, const bool uav_barrier = true);
	void dispatch_with_32bit_constant(const uint x, const uint y, const uint z, const uint constant, const bool uav_barrier = true);
	void dispatch_with_32bit_constant(buffer& buf, const uint offset, const uint constant, const bool uav_barrier = true);

	//クリア
	void clear_render_target_view(render_target_view& rtv, const float color[4]);
	void clear_depth_stencil_view(depth_stencil_view& dsv, const float depth, const uint stencil);
	void clear_depth_stencil_view(depth_stencil_view& dsv, const float depth);
	void clear_depth_stencil_view(depth_stencil_view& dsv, const uint stencil);
	void clear_unordered_access_view(unordered_access_view& uav, const float value[4], const bool uav_barrier = true);
	void clear_unordered_access_view(unordered_access_view& uav, const uint value[4], const bool uav_barrier = true);

	//バッファを更新
	void *update_buffer(buffer& buf, const uint offset = 0, const uint size = -1);

	//バッファをコピー
	void copy_buffer(buffer& dst, const uint dst_offset, buffer& src, const uint src_offset, const uint size);
	void copy_buffer(buffer& dst, const uint dst_offset, upload_buffer& src, const uint src_offset, const uint size);
	void copy_buffer(readback_buffer& dst, const uint dst_offset, buffer& src, const uint src_offset, const uint size);

	//テクスチャをコピー
	void copy_texture(texture& dst, texture& src);

	//リソースをコピー
	void copy_resource(buffer& dst, buffer& src);
	void copy_resource(buffer& dst, upload_buffer& src);

	//TLAS/BLASの構築
	void build_top_level_acceleration_structure(top_level_acceleration_structure& tlas, const uint num_instances);
	void build_bottom_level_acceleration_structure(bottom_level_acceleration_structure& blas, const render::geometry_state* p_gs = nullptr);
	void refit_bottom_level_acceleration_structure(bottom_level_acceleration_structure& blas, const render::geometry_state* p_gs = nullptr);

	//BLASのコンパクション
	void compact_bottom_level_acceleration_structure(bottom_level_acceleration_structure& blas);

	//任意の関数を呼ぶ
	void call_function(std::function<void()> func, const bool use_target_state = false);

private:

	static constexpr uint	m_max_bind_resource_count = 128;

	struct erase_queue_elem
	{
		unordered_map<string, bind_resource>::iterator	iterator;
		uint											timestamp;
	};

	uint									m_priority;
	iface::render_device&					m_device;

	uint									m_stencil;
	const render::target_state*				mp_ts;
	const render::pipeline_state*			mp_ps;
	const render::geometry_state*			mp_gs;
	render::viewport						m_viewport;
	unordered_map<string, bind_resource>	m_bind_resources;
	queue<erase_queue_elem>					m_bind_resource_erase_queue;
	uint									m_timestamp;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//push_priority
///////////////////////////////////////////////////////////////////////////////////////////////////

class push_priority
{
public:

	//コンストラクタ
	push_priority(render_context& context) : m_context(context), m_priority(context.get_priority()){}

	//デストラクタ
	~push_priority(){ m_context.set_priority(m_priority); }

private:

	render_context& m_context;
	uint			m_priority;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//push_viewport
///////////////////////////////////////////////////////////////////////////////////////////////////

class push_viewport
{
public:

	//コンストラクタ
	push_viewport(render_context& context) : m_context(context), m_viewport(context.get_viewport()){}

	//デストラクタ
	~push_viewport(){ m_context.set_viewport(m_viewport); }

private:

	render_context& m_context;
	viewport		m_viewport;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#include"render_context/render_context-impl.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

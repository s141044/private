
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//render_context
///////////////////////////////////////////////////////////////////////////////////////////////////

//コンストラクタ
inline render_context::render_context(iface::render_device& device) : m_device(device)
{
	m_stencil = 0;
	m_priority = 0;
	m_timestamp = 0;
	mp_ts = nullptr;
	mp_gs = nullptr;
	mp_ps = nullptr;
}
	
///////////////////////////////////////////////////////////////////////////////////////////////////

//優先度を返す
inline uint render_context::get_priority() const
{
	return m_priority;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//優先度を設定
inline void render_context::set_priority(uint priority)
{
	m_priority = priority;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//リソースを設定
template<class Resource> inline void render_context::set_pipeline_resource(string name, Resource& resource, const bool global)
{
	auto it = m_bind_resources.try_emplace(std::move(name)).first;
	it->second = bind_resource(resource, m_timestamp);
	if(not(global)){ m_bind_resource_erase_queue.emplace(it, m_timestamp); } //globalのときはqueueに入れないので削除されない
	m_timestamp++;

	if(m_bind_resources.size() < m_max_bind_resource_count)
		return;

	while(true)
	{
		auto erase_elem = m_bind_resource_erase_queue.front();
		m_bind_resource_erase_queue.pop();

		if(erase_elem.timestamp == erase_elem.iterator->second.timestamp())
		{
			m_bind_resources.erase(erase_elem.iterator);
			break;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//ステートを返す
inline const target_state* render_context::get_target_state() const
{
	return mp_ts;
}

inline const pipeline_state* render_context::get_pipeline_state() const
{
	return mp_ps;
}

inline const geometry_state* render_context::get_geometry_state() const
{
	return mp_gs;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//ステートを設定
inline void render_context::set_target_state(const render::target_state& ts)
{
	assert(&ts != nullptr);
	mp_ts = &ts;
}

inline void render_context::set_pipeline_state(const render::pipeline_state& ps)
{
	assert(&ps != nullptr);
	mp_ps = &ps;
}

inline void render_context::set_geometry_state(const render::geometry_state& gs)
{
	assert(&gs != nullptr);
	mp_gs = &gs;
}

inline void render_context::set_geometry_state(std::nullptr_t)
{
	mp_gs = nullptr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//ステンシル値を返す
inline uint render_context::get_stencil_value() const
{
	return m_stencil;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//ステンシル値を設定
inline void render_context::set_stencil_value(const uint stencil)
{
	m_stencil = stencil;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//ビューポートを返す
inline const viewport& render_context::get_viewport() const
{
	return m_viewport;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//ビューポートを設定
inline void render_context::set_viewport(const render::viewport& viewport)
{
	m_viewport = viewport;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//描画
inline void render_context::draw(const uint vertex_count, const uint instance_count, const uint start_vertex_location, const bool uav_barrier)
{
	draw_with_32bit_constant(vertex_count, instance_count, start_vertex_location, 0, uav_barrier);
}

inline void render_context::draw(buffer& buf, const uint offset, const bool uav_barrier)
{
	draw_with_32bit_constant(buf, offset, 0, uav_barrier);
}

inline void render_context::draw_with_32bit_constant(const uint vertex_count, const uint instance_count, const uint start_vertex_location, const uint constant, const bool uav_barrier)
{
	void *p_optional = m_device.validate(*mp_ps, *mp_ts, mp_gs, m_bind_resources);
	if(p_optional){ m_device.m_command_manager.construct<command::draw>(m_priority, vertex_count, instance_count, start_vertex_location, constant, uav_barrier, m_stencil, reinterpret_cast<float*>(&m_viewport), *mp_ps, *mp_ts, mp_gs, p_optional); }
}

inline void render_context::draw_with_32bit_constant(buffer& buf, const uint offset, const uint constant, const bool uav_barrier)
{
	void *p_optional = m_device.validate(*mp_ps, *mp_ts, mp_gs, m_bind_resources);
	if(p_optional){ m_device.m_command_manager.construct<command::draw_indirect>(m_priority, buf, offset, constant, uav_barrier, m_stencil, reinterpret_cast<float*>(&m_viewport), *mp_ps, *mp_ts, mp_gs, p_optional); }
}

inline void render_context::draw_indexed(const uint index_count, const uint instance_count, const uint start_index_location, const uint base_vertex_location, const bool uav_barrier)
{
	draw_indexed_with_32bit_constant(index_count, instance_count, start_index_location, base_vertex_location, 0, uav_barrier);
}

inline void render_context::draw_indexed(buffer& buf, const uint offset, const bool uav_barrier)
{
	draw_with_32bit_constant(buf, offset, 0, uav_barrier);
}

inline void render_context::draw_indexed_with_32bit_constant(const uint index_count, const uint instance_count, const uint start_index_location, const uint base_vertex_location, const uint constant, const bool uav_barrier)
{
	void *p_optional = m_device.validate(*mp_ps, *mp_ts, mp_gs, m_bind_resources);
	if(p_optional){ m_device.m_command_manager.construct<command::draw_indexed>(m_priority, index_count, instance_count, start_index_location, base_vertex_location, constant, uav_barrier, m_stencil, reinterpret_cast<float*>(&m_viewport), *mp_ps, *mp_ts, mp_gs, p_optional); }
}

inline void render_context::draw_indexed_with_32bit_constant(buffer& buf, const uint offset, const uint constant, const bool uav_barrier)
{
	void *p_optional = m_device.validate(*mp_ps, *mp_ts, mp_gs, m_bind_resources);
	if(p_optional){ m_device.m_command_manager.construct<command::draw_indexed_indirect>(m_priority, buf, offset, constant, uav_barrier, m_stencil, reinterpret_cast<float*>(&m_viewport), *mp_ps, *mp_ts, mp_gs, p_optional); }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//ディスパッチ
inline void render_context::dispatch(const uint x, const uint y, const uint z, const bool uav_barrier)
{
	dispatch_with_32bit_constant(x, y, z, 0, uav_barrier);
}

inline void render_context::dispatch(buffer& buf, const uint offset, const bool uav_barrier)
{
	dispatch_with_32bit_constant(buf, offset, 0, uav_barrier);
}

inline void render_context::dispatch_with_32bit_constant(const uint x, const uint y, const uint z, const uint constant, const bool uav_barrier)
{
	void *p_optional = m_device.validate(*mp_ps, m_bind_resources);
	if(p_optional){ m_device.m_command_manager.construct<command::dispatch>(m_priority, x, y, z, constant, uav_barrier, *mp_ps, p_optional); }
}

inline void render_context::dispatch_with_32bit_constant(buffer& buf, const uint offset, const uint constant, const bool uav_barrier)
{
	void *p_optional = m_device.validate(*mp_ps, m_bind_resources);
	if(p_optional){ m_device.m_command_manager.construct<command::dispatch_indirect>(m_priority, buf, offset, constant, uav_barrier, *mp_ps, p_optional); }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//クリア
inline void render_context::clear_render_target_view(render_target_view& rtv, const float color[4])
{
	m_device.m_command_manager.construct<command::clear_render_target_view>(m_priority, rtv, color);
}

inline void render_context::clear_depth_stencil_view(depth_stencil_view& dsv, const float depth, const uint stencil)
{
	m_device.m_command_manager.construct<command::clear_depth_stencil_view>(m_priority, dsv, command::clear_flag_depth_stencil, depth, stencil);
}

inline void render_context::clear_depth_stencil_view(depth_stencil_view& dsv, const float depth)
{
	m_device.m_command_manager.construct<command::clear_depth_stencil_view>(m_priority, dsv, command::clear_flag_depth, depth, 0);
}

inline void render_context::clear_depth_stencil_view(depth_stencil_view& dsv, const uint stencil)
{
	m_device.m_command_manager.construct<command::clear_depth_stencil_view>(m_priority, dsv, command::clear_flag_stencil, float(0), stencil);
}

inline void render_context::clear_unordered_access_view(unordered_access_view& uav, const float value[4], const bool uav_barrier)
{
	m_device.m_command_manager.construct<command::clear_unordered_access_view_float>(m_priority, uav, value, uav_barrier);
}

inline void render_context::clear_unordered_access_view(unordered_access_view& uav, const uint value[4], const bool uav_barrier)
{
	m_device.m_command_manager.construct<command::clear_unordered_access_view_uint>(m_priority, uav, value, uav_barrier);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//バッファを更新
inline void *render_context::update_buffer(buffer& buf, const uint offset, uint size)
{
	size = std::min(size, buf.size() - offset);
	void *p_update = m_device.get_update_buffer_pointer(size);
	auto* p_command = m_device.m_command_manager.construct<command::update_buffer>(m_priority, buf, offset, size, p_update);
	return p_update;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//バッファをコピー
inline void render_context::copy_buffer(buffer& dst, const uint dst_offset, buffer& src, const uint src_offset, const uint size)
{
	m_device.m_command_manager.construct<command::copy_buffer>(m_priority, dst, dst_offset, src, src_offset, size);
}

inline void render_context::copy_buffer(buffer& dst, const uint dst_offset, upload_buffer& src, const uint src_offset, const uint size)
{
	m_device.m_command_manager.construct<command::copy_buffer>(m_priority, dst, dst_offset, src, src_offset, size);
}

inline void render_context::copy_buffer(readback_buffer& dst, const uint dst_offset, buffer& src, const uint src_offset, const uint size)
{
	m_device.m_command_manager.construct<command::copy_buffer>(m_priority, dst, dst_offset, src, src_offset, size);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//テクスチャをコピー
inline void render_context::copy_texture(texture& dst, texture& src)
{
	m_device.m_command_manager.construct<command::copy_texture>(m_priority, dst, src);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//リソースをコピー
inline void render_context::copy_resource(buffer& dst, buffer& src)
{
	m_device.m_command_manager.construct<command::copy_resource>(m_priority, dst, src);
}

inline void render_context::copy_resource(buffer& dst, upload_buffer& src)
{
	m_device.m_command_manager.construct<command::copy_resource>(m_priority, dst, src);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//TLASの構築
inline void render_context::build_top_level_acceleration_structure(top_level_acceleration_structure& tlas, const uint num_instances)
{
	m_device.m_command_manager.construct<command::build_top_level_acceleration_structure>(m_priority, tlas, num_instances);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//BLASの構築
inline void render_context::build_bottom_level_acceleration_structure(bottom_level_acceleration_structure& blas, const render::geometry_state* p_gs)
{
	m_device.m_command_manager.construct<command::build_bottom_level_acceleration_structure>(m_priority, blas, p_gs);
}

inline void render_context::refit_bottom_level_acceleration_structure(bottom_level_acceleration_structure& blas, const render::geometry_state* p_gs)
{
	m_device.m_command_manager.construct<command::refit_bottom_level_acceleration_structure>(m_priority, blas, p_gs);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//BLASのコンパクション
inline void render_context::compact_bottom_level_acceleration_structure(bottom_level_acceleration_structure& blas)
{
	m_device.m_command_manager.construct<command::compact_bottom_level_acceleration_structure>(m_priority, blas);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//任意の関数を呼ぶ
inline void render_context::call_function(std::function<void()> func, bool use_target_state)
{
	const render::target_state* p_target_state = nullptr;
	if(use_target_state){ p_target_state = mp_ts; }
	m_device.m_command_manager.construct<command::call_function>(m_priority, std::move(func), p_target_state);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//debug_draw::vertex
///////////////////////////////////////////////////////////////////////////////////////////////////

//コンストラクタ
inline debug_draw::vertex::vertex(const float3& p, const float3& c)
{
	position = p;
	auto r = uint(c[0] * 255 + 0.5f);
	auto g = uint(c[1] * 255 + 0.5f);
	auto b = uint(c[2] * 255 + 0.5f);
	color = r | (g << 8) | (b << 16);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//debug_draw
///////////////////////////////////////////////////////////////////////////////////////////////////

//コンストラクタ
inline debug_draw::debug_draw(const uint capacity) : m_front(), m_back(capacity), m_capacity(capacity)
{
	m_shader_file = gp_shader_manager->create(L"debug_draw.sdf.json");
	mp_vertex_buf = gp_render_device->create_structured_buffer(sizeof(vertex), m_capacity, resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access));
	mp_vertex_uav = gp_render_device->create_unordered_access_view(*mp_vertex_buf, buffer_uav_desc(*mp_vertex_buf));
	mp_vertex_ubuf = gp_render_device->create_upload_buffer(sizeof(vertex) * m_capacity);

	buffer* p_vb = mp_vertex_buf.get();
	uint offset = 0;
	uint stride = sizeof(vertex);
	mp_geometry_state = gp_render_device->create_geometry_state(1, &p_vb, &offset, &stride, nullptr);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//ライン描画
inline uint debug_draw::draw_lines(const uint num_lines)
{
	return draw_xxx(2 * num_lines, draw_type_line);
}

inline uint debug_draw::draw_line_strip(const uint num_lines)
{
	return draw_xxx(num_lines + 1, draw_type_line_strip);
}

inline debug_draw::vertex* debug_draw::draw_lines(const uint num_lines, const bool cpu_write)
{
	assert(cpu_write);
	return draw_xxx(2 * num_lines, draw_type_line, cpu_write);
}

inline debug_draw::vertex* debug_draw::draw_line_strip(const uint num_lines, const bool cpu_write)
{
	assert(cpu_write);
	return draw_xxx(num_lines + 1, draw_type_line_strip, cpu_write);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//ポイント描画
inline uint debug_draw::draw_points(const uint num_lines)
{
	return draw_xxx(num_lines, draw_type_point);
}

inline debug_draw::vertex* debug_draw::draw_points(const uint num_points, const bool cpu_write)
{
	assert(cpu_write);
	return draw_xxx(num_points, draw_type_point, cpu_write);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//描画
inline uint debug_draw::draw_xxx(const uint vertex_count, const draw_type type)
{
	if(m_front + vertex_count >= m_back)
		return 0xffffffff;

	m_back -= vertex_count;

	auto& info = m_draw_infos[type].emplace_back();
	info.vertex_count = vertex_count;
	info.start_vertex_location = m_back;
	return m_back;
}

inline debug_draw::vertex* debug_draw::draw_xxx(const uint vertex_count, const draw_type type, const bool cpu_write)
{
	if(m_front + vertex_count >= m_back)
		return nullptr;

	auto& info = m_draw_infos[type].emplace_back();
	info.vertex_count = vertex_count;
	info.start_vertex_location = m_front;

	auto* ret = mp_vertex_ubuf->data<vertex>() + m_front;
	m_front += vertex_count;
	return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//描画
inline bool debug_draw::draw(render_context& context, target_state& target_state)
{
	if(m_shader_file.has_update())
		m_shader_file.update();

	if(m_shader_file.is_valid())
	{
		if(m_front > 0)
			context.copy_buffer(*mp_vertex_buf, 0, *mp_vertex_ubuf, 0, sizeof(vertex) * m_front);

		context.set_target_state(target_state);
		context.set_geometry_state(*mp_geometry_state);

		context.set_pipeline_state(*m_shader_file.get("draw_lines"));
		for(auto& info : m_draw_infos[draw_type_line])
			context.draw(info.vertex_count, 1, info.start_vertex_location);
		
		context.set_pipeline_state(*m_shader_file.get("draw_line_strip"));
		for(auto& info : m_draw_infos[draw_type_line_strip])
			context.draw(info.vertex_count, 1, info.start_vertex_location);
		
		context.set_pipeline_state(*m_shader_file.get("draw_points"));
		for(auto& info : m_draw_infos[draw_type_point])
			context.draw(info.vertex_count, 1, info.start_vertex_location);
	}
	for(uint i = 0; i < draw_type_count; i++)
		m_draw_infos[i].clear();

	m_front = 0;
	m_back = m_capacity;
	return m_shader_file.is_valid();
}

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

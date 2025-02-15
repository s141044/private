
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//raytracing_manager
///////////////////////////////////////////////////////////////////////////////////////////////////

//コンストラクタ
inline raytracing_manager::raytracing_manager(const uint max_size) : m_bindless_instance_allocator(max_size), m_raytracing_instance_allocator(max_size)
{
	mp_tlas = gp_render_device->create_top_level_acceleration_structure(max_size);
	mp_bindless_instance_descs_buf = gp_render_device->create_structured_buffer(sizeof(bindless_instance_desc), max_size, resource_flag_allow_shader_resource);
	mp_bindless_isntance_descs_srv = gp_render_device->create_shader_resource_view(*mp_bindless_instance_descs_buf, buffer_srv_desc(*mp_bindless_instance_descs_buf));
	mp_bindless_instance_descs_ubuf = gp_render_device->create_upload_buffer(sizeof(bindless_instance_desc) * max_size);
	mp_raytracing_isntance_descs_srv = gp_render_device->create_shader_resource_view(mp_tlas->instance_descs(), buffer_srv_desc(mp_tlas->instance_descs()));
	mp_raytracing_instance_descs_ubuf = gp_render_device->create_upload_buffer(sizeof(raytracing_instance_desc) * max_size);

	gp_render_device->set_name(*mp_tlas, L"tlas");
	gp_render_device->set_name(*mp_bindless_instance_descs_buf, L"bindless_isntance_descs_buf");
	gp_render_device->set_name(*mp_bindless_instance_descs_ubuf, L"bindless_isntance_descs_ubuf");
	gp_render_device->set_name(*mp_raytracing_instance_descs_ubuf, L"raytracing_instance_descs_ubuf");
	gp_render_device->set_name(mp_tlas->instance_descs(), L"raytracing_instance_descs_buf");

	for(uint i = 0; i < max_size; i++)
		update_raytracing_instance(i)->blas_address = 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//インスタンス登録
inline raytracing_manager::register_result raytracing_manager::register_instance(const uint bindless_instance_count, const uint raytracing_instance_count)
{
	register_result result;
	result.bindless_instance_index = uint(m_bindless_instance_allocator.allocate(bindless_instance_count));
	result.raytracing_instance_index = uint(m_raytracing_instance_allocator.allocate(raytracing_instance_count));
	return result;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//インスタンス登録解除
inline void raytracing_manager::unregister_instance(const uint bindless_instance_index, const uint raytracing_instance_index)
{
	const auto n = uint(m_raytracing_instance_allocator.allocate_size(raytracing_instance_index));
	for(uint i = 0; i < n; i++)
		update_raytracing_instance(raytracing_instance_index + i)->blas_address = 0;

	m_bindless_instance_allocator.deallocate(bindless_instance_index);
	m_raytracing_instance_allocator.deallocate(raytracing_instance_index);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//TLAS構築
inline void raytracing_manager::build(render_context &context)
{
	const auto bindless_instance_count = uint(m_bindless_instance_allocator.last_free_block_index());
	const auto raytracing_instance_count = uint(m_raytracing_instance_allocator.last_free_block_index());
	if(raytracing_instance_count)
	{
		context.push_priority(priority_build_top_level_acceleration_structure);
		context.copy_buffer(mp_tlas->instance_descs(), 0, *mp_raytracing_instance_descs_ubuf, 0, sizeof(raytracing_instance_desc) * raytracing_instance_count);
		context.copy_buffer(*mp_bindless_instance_descs_buf, 0, *mp_bindless_instance_descs_ubuf, 0, sizeof(bindless_instance_desc) * bindless_instance_count);
		context.build_top_level_acceleration_structure(*mp_tlas, raytracing_instance_count);
		context.pop_priority();
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

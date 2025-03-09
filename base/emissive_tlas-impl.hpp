
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//emissive_tlas
///////////////////////////////////////////////////////////////////////////////////////////////////

//コンストラクタ
inline emissive_tlas::emissive_tlas(const uint max_instance) : m_instance_allocator(max_instance)
{
	m_shaders = gp_shader_manager->create(L"emissive.sdf.json");
	
	mp_instance_list_ubuf = gp_render_device->create_upload_buffer(sizeof(uint) * max_instance);
	mp_instance_list_buf[0] = gp_render_device->create_byteaddress_buffer(sizeof(uint) * max_instance, resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access | resource_flag_scratch));
	mp_instance_list_buf[1] = gp_render_device->create_byteaddress_buffer(sizeof(uint3) * max_instance, resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access | resource_flag_scratch));
	mp_instance_list_srv[0] = gp_render_device->create_shader_resource_view(*mp_instance_list_buf[0], buffer_srv_desc(*mp_instance_list_buf[0]));
	mp_instance_list_srv[1] = gp_render_device->create_shader_resource_view(*mp_instance_list_buf[1], buffer_srv_desc(*mp_instance_list_buf[1]));
	mp_instance_list_uav[0] = gp_render_device->create_unordered_access_view(*mp_instance_list_buf[0], buffer_uav_desc(*mp_instance_list_buf[0]));
	mp_instance_list_uav[1] = gp_render_device->create_unordered_access_view(*mp_instance_list_buf[1], buffer_uav_desc(*mp_instance_list_buf[1]));
	gp_render_device->set_name(*mp_instance_list_buf[0], L"emissive_instance_list_buf[0]");
	gp_render_device->set_name(*mp_instance_list_buf[1], L"emissive_instance_list_buf[1]");
	
	mp_instance_list_size_buf = gp_render_device->create_byteaddress_buffer(sizeof(uint4), resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access));
	mp_instance_list_size_srv = gp_render_device->create_shader_resource_view(*mp_instance_list_size_buf, buffer_srv_desc(*mp_instance_list_size_buf));
	mp_instance_list_size_uav = gp_render_device->create_unordered_access_view(*mp_instance_list_size_buf, buffer_uav_desc(*mp_instance_list_size_buf));
	gp_render_device->set_name(*mp_instance_list_size_buf, L"emissive_instance_list_size_buf");
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//登録
inline uint emissive_tlas::register_blas(const emissive_blas& blas)
{
	auto index = m_instance_allocator.allocate(1);
	auto* instance_list = mp_instance_list_ubuf->data<uint>();
	instance_list[index] = blas.bindless_handle();
	return uint(index);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//登録解除
inline void emissive_tlas::unregister_blas(uint index)
{
	m_instance_allocator.deallocate(index);
	auto* instance_list = mp_instance_list_ubuf->data<uint>();
	instance_list[index] = 0xffffffff;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//ビルド
inline bool emissive_tlas::build(render_context& context)
{
	//if(m_shaders.has_update())
	//	m_shaders.update();
	//else if(m_shaders.is_invalid())
	//	return false;
	//
	//auto blas_count = uint(m_instance_allocator.last_free_block_index());
	//if(blas_count == 0)
	//	return true;
	//
	//push_priority push_priority(context);
	//context.set_priority(priority_build_top_level_acceleration_structure);
	//context.copy_buffer(*mp_instance_list_buf[0], 0, *mp_instance_list_ubuf, 0, mp_instance_list_ubuf->size());
	//
	//uint clear_value[4] = {};
	//context.clear_unordered_access_view(*mp_instance_list_size_uav, clear_value);
	//
	//context.set_pipeline_state(*m_shaders.get("build_emissive_tlas"));
	//context.set_pipeline_resource("emissive_instance_list_srv", *mp_instance_list_srv[0]);
	//context.set_pipeline_resource("emissive_instance_list_uav", *mp_instance_list_uav[1]);
	//context.set_pipeline_resource("emissive_instance_list_size_uav", *mp_instance_list_size_uav);
	//context.dispatch_with_32bit_constant(ceil_div(blas_count, 256), 1, 1, blas_count);
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//リソースをバインド
inline void emissive_tlas::bind(render_context& context)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

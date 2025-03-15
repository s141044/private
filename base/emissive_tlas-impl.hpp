
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//emissive_tlas
///////////////////////////////////////////////////////////////////////////////////////////////////

//コンストラクタ
inline emissive_tlas::emissive_tlas(const uint max_instance) : m_blas_allocator(max_instance)
{
	m_shaders = gp_shader_manager->create(L"emissive.sdf.json");
	
	mp_blas_handle_ubuf = gp_render_device->create_upload_buffer(sizeof(uint) * max_instance);
	mp_blas_handle_buf = gp_render_device->create_byteaddress_buffer(sizeof(uint) * max_instance, resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access | resource_flag_scratch));
	mp_blas_handle_srv = gp_render_device->create_shader_resource_view(*mp_blas_handle_buf, buffer_srv_desc(*mp_blas_handle_buf));
	mp_blas_handle_uav = gp_render_device->create_unordered_access_view(*mp_blas_handle_buf, buffer_uav_desc(*mp_blas_handle_buf));
	gp_render_device->set_name(*mp_blas_handle_buf, L"emissive_blas_handle_buf");

	mp_tlas_buf = gp_render_device->create_byteaddress_buffer(sizeof(header) + sizeof(bucket) * max_instance, resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access));
	mp_tlas_srv = gp_render_device->create_shader_resource_view(*mp_tlas_buf, buffer_srv_desc(*mp_tlas_buf));
	mp_tlas_uav = gp_render_device->create_unordered_access_view(*mp_tlas_buf, buffer_uav_desc(*mp_tlas_buf));
	gp_render_device->set_name(*mp_tlas_buf, L"emissive_tlas_buf");

	mp_sum_weight_buf = gp_render_device->create_byteaddress_buffer(sizeof(uint), resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access));
	mp_sum_weight_srv = gp_render_device->create_shader_resource_view(*mp_sum_weight_buf, buffer_srv_desc(*mp_sum_weight_buf));
	mp_sum_weight_uav = gp_render_device->create_unordered_access_view(*mp_sum_weight_buf, buffer_uav_desc(*mp_sum_weight_buf));

	mp_max_weight_buf = gp_render_device->create_byteaddress_buffer(sizeof(float), resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access));
	mp_max_weight_srv = gp_render_device->create_shader_resource_view(*mp_max_weight_buf, buffer_srv_desc(*mp_max_weight_buf));
	mp_max_weight_uav = gp_render_device->create_unordered_access_view(*mp_max_weight_buf, buffer_uav_desc(*mp_max_weight_buf));

	mp_scan_scratch_buf = gp_render_device->create_byteaddress_buffer(sizeof(uint4), resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access));
	mp_scan_scratch_srv = gp_render_device->create_shader_resource_view(*mp_scan_scratch_buf, buffer_srv_desc(*mp_scan_scratch_buf));
	mp_scan_scratch_uav = gp_render_device->create_unordered_access_view(*mp_scan_scratch_buf, buffer_uav_desc(*mp_scan_scratch_buf));
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//登録
inline uint emissive_tlas::register_blas(const emissive_blas& blas)
{
	auto index = m_blas_allocator.allocate(1);
	auto* blas_handle = mp_blas_handle_ubuf->data<uint>();
	blas_handle[index] = blas.bindless_handle();
	return uint(index);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//登録解除
inline void emissive_tlas::unregister_blas(uint index)
{
	m_blas_allocator.deallocate(index);
	auto* blas_handle = mp_blas_handle_ubuf->data<uint>();
	blas_handle[index] = 0xffffffff;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//ビルド
inline bool emissive_tlas::build(render_context& context)
{
	if(m_shaders.has_update())
		m_shaders.update();
	else if(m_shaders.is_invalid())
		return false;

	push_priority push_priority(context);
	context.set_priority(priority_build_top_level_acceleration_structure + 2);

	auto blas_count = uint(m_blas_allocator.last_free_block_index());
	if(blas_count == 0)
	{
		if(mp_weight_buf != nullptr)
		{
			mp_weight_buf.reset();
			mp_weight_srv.reset();
			mp_weight_uav.reset();
			mp_light_index_buf.reset();
			mp_light_index_srv.reset();
			mp_light_index_uav.reset();
			mp_heavy_index_buf.reset();
			mp_heavy_index_srv.reset();
			mp_heavy_index_uav.reset();
			mp_light_sum_weight_buf.reset();
			mp_light_sum_weight_srv.reset();
			mp_light_sum_weight_uav.reset();
			mp_heavy_sum_weight_buf.reset();
			mp_heavy_sum_weight_srv.reset();
			mp_heavy_sum_weight_uav.reset();

			auto* p_header = static_cast<header*>(context.update_buffer(*mp_tlas_buf, 0, sizeof(header)));
			p_header->power = 0;
			p_header->inv_power = INFINITY;
			p_header->blas_count = 0;
			p_header->padding = 0;
		}
		return true;
	}

	context.copy_buffer(*mp_blas_handle_buf, 0, *mp_blas_handle_ubuf, 0, sizeof(u32) * blas_count);
	context.clear_unordered_access_view(*mp_sum_weight_uav, uint4(0, 0, 0, 0));
	context.clear_unordered_access_view(*mp_scan_scratch_uav, uint4(0, 0, 0, 0));

	if((mp_weight_buf == nullptr) || (blas_count != mp_weight_buf->num_elements()))
	{
		mp_weight_buf = gp_render_device->create_byteaddress_buffer(sizeof(float) * blas_count, resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access | resource_flag_scratch));
		mp_light_index_buf = gp_render_device->create_byteaddress_buffer(sizeof(uint) * blas_count, resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access | resource_flag_scratch));
		mp_heavy_index_buf = gp_render_device->create_byteaddress_buffer(sizeof(uint) * blas_count, resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access | resource_flag_scratch));
		mp_light_sum_weight_buf = gp_render_device->create_byteaddress_buffer(sizeof(uint) * blas_count, resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access | resource_flag_scratch));
		mp_heavy_sum_weight_buf = gp_render_device->create_byteaddress_buffer(sizeof(uint) * blas_count, resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access | resource_flag_scratch));

		mp_weight_srv = gp_render_device->create_shader_resource_view(*mp_weight_buf, buffer_srv_desc(*mp_weight_buf));
		mp_light_index_srv = gp_render_device->create_shader_resource_view(*mp_light_index_buf, buffer_srv_desc(*mp_light_index_buf));
		mp_heavy_index_srv = gp_render_device->create_shader_resource_view(*mp_heavy_index_buf, buffer_srv_desc(*mp_heavy_index_buf));
		mp_light_sum_weight_srv = gp_render_device->create_shader_resource_view(*mp_light_sum_weight_buf, buffer_srv_desc(*mp_light_sum_weight_buf));
		mp_heavy_sum_weight_srv = gp_render_device->create_shader_resource_view(*mp_heavy_sum_weight_buf, buffer_srv_desc(*mp_heavy_sum_weight_buf));

		mp_weight_uav = gp_render_device->create_unordered_access_view(*mp_weight_buf, buffer_uav_desc(*mp_weight_buf));
		mp_light_index_uav = gp_render_device->create_unordered_access_view(*mp_light_index_buf, buffer_uav_desc(*mp_light_index_buf));
		mp_heavy_index_uav = gp_render_device->create_unordered_access_view(*mp_heavy_index_buf, buffer_uav_desc(*mp_heavy_index_buf));
		mp_light_sum_weight_uav = gp_render_device->create_unordered_access_view(*mp_light_sum_weight_buf, buffer_uav_desc(*mp_light_sum_weight_buf));
		mp_heavy_sum_weight_uav = gp_render_device->create_unordered_access_view(*mp_heavy_sum_weight_buf, buffer_uav_desc(*mp_heavy_sum_weight_buf));
	}

	struct cbuffer
	{
		uint	blas_count;
		uint3	padding;
	};
	cbuffer cbuffer;
	cbuffer.blas_count = blas_count;
	
	auto p_cbuf = gp_render_device->create_temporary_cbuffer(sizeof(cbuffer), &cbuffer);
	context.set_pipeline_resource("emissive_build_cbuf", *p_cbuf);
	
	gp_raytracing_manager->bind(context); //ちょっと嫌...

	context.set_pipeline_resource("weight_uav", *mp_weight_uav);
	context.set_pipeline_resource("max_weight_uav", *mp_max_weight_uav);
	context.set_pipeline_resource("blas_handle_srv", *mp_blas_handle_srv);
	context.set_pipeline_state(*m_shaders.get("initialize_tlas"));
	context.dispatch(ceil_div(blas_count, 256), 1, 1);
	
	context.set_pipeline_resource("weight_srv", *mp_weight_srv);
	context.set_pipeline_resource("max_weight_srv", *mp_max_weight_srv);
	context.set_pipeline_resource("sum_weight_uav", *mp_sum_weight_uav);
	context.set_pipeline_state(*m_shaders.get("initialize2"));
	context.dispatch(ceil_div(blas_count, 256), 1, 1);
	
	context.set_pipeline_resource("weight_srv", *mp_weight_srv);
	context.set_pipeline_resource("sum_weight_srv", *mp_sum_weight_srv);
	context.set_pipeline_resource("max_weight_srv", *mp_max_weight_srv);
	context.set_pipeline_resource("light_index_uav", *mp_light_index_uav);
	context.set_pipeline_resource("heavy_index_uav", *mp_heavy_index_uav);
	context.set_pipeline_resource("scan_scratch_uav", *mp_scan_scratch_uav);
	context.set_pipeline_resource("light_sum_weight_uav", *mp_light_sum_weight_uav);
	context.set_pipeline_resource("heavy_sum_weight_uav", *mp_heavy_sum_weight_uav);
	context.set_pipeline_state(*m_shaders.get("scan"));
	context.dispatch(ceil_div(blas_count, 256), 1, 1);
	
	context.set_pipeline_resource("as_uav", *mp_tlas_uav);
	context.set_pipeline_resource("weight_srv", *mp_weight_srv);
	context.set_pipeline_resource("sum_weight_srv", *mp_sum_weight_srv);
	context.set_pipeline_resource("max_weight_srv", *mp_max_weight_srv);
	context.set_pipeline_resource("light_index_srv", *mp_light_index_srv);
	context.set_pipeline_resource("heavy_index_srv", *mp_heavy_index_srv);
	context.set_pipeline_resource("blas_handle_srv", *mp_blas_handle_srv);
	context.set_pipeline_resource("scan_scratch_srv", *mp_scan_scratch_srv);
	context.set_pipeline_resource("light_sum_weight_srv", *mp_light_sum_weight_srv);
	context.set_pipeline_resource("heavy_sum_weight_srv", *mp_heavy_sum_weight_srv);
	context.set_pipeline_state(*m_shaders.get("build_tlas"));
	context.dispatch(ceil_div(blas_count, 256), 1, 1);
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

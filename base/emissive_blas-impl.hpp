
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//emissive_blas
///////////////////////////////////////////////////////////////////////////////////////////////////

//コンストラクタ
inline emissive_blas::emissive_blas()
{
	m_shaders = gp_shader_manager->create(L"emissive.sdf.json");
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//デストラクタ
inline emissive_blas::~emissive_blas()
{
	if(mp_blas_srv != nullptr)
		gp_render_device->unregister_bindless(*mp_blas_srv);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//ビルド
inline bool emissive_blas::build(render_context& context, const uint raytracing_instance_index, const uint bindless_instance_index, const uint primitive_count, const float power)
{
	if(m_shaders.has_update())
		m_shaders.update();
	else if(m_shaders.is_invalid())
		return false;

	if(mp_blas_buf == nullptr)
	{
		mp_blas_buf = gp_render_device->create_byteaddress_buffer(sizeof(uint2) * primitive_count, resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access));
		mp_blas_srv = gp_render_device->create_shader_resource_view(*mp_blas_buf, buffer_srv_desc(*mp_blas_buf));
		mp_blas_uav = gp_render_device->create_unordered_access_view(*mp_blas_buf, buffer_uav_desc(*mp_blas_buf));
		gp_render_device->register_bindless(*mp_blas_srv);
	}

#if 0

	const uint work_per_thread = 8;
	const uint state_count = ceil_div(primitive_count, work_per_thread);
	auto p_weight_buf = gp_render_device->create_byteaddress_buffer(sizeof(float) * primitive_count, resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access | resource_flag_scratch));
	auto p_light_index_buf = gp_render_device->create_byteaddress_buffer(sizeof(uint) * primitive_count, resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access | resource_flag_scratch));
	auto p_heavy_index_buf = gp_render_device->create_byteaddress_buffer(sizeof(uint) * primitive_count, resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access | resource_flag_scratch));
	auto p_light_sum_weight_buf = gp_render_device->create_byteaddress_buffer(sizeof(uint) * primitive_count, resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access | resource_flag_scratch));
	auto p_heavy_sum_weight_buf = gp_render_device->create_byteaddress_buffer(sizeof(uint) * primitive_count, resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access | resource_flag_scratch));
	auto p_sum_weight_buf = gp_render_device->create_byteaddress_buffer(sizeof(uint), resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access));
	auto p_max_weight_buf = gp_render_device->create_byteaddress_buffer(sizeof(float), resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access));
	auto p_block_light_count_buf = gp_render_device->create_byteaddress_buffer(sizeof(uint) * (ceil_div(primitive_count, 256) + 1), resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access));
	auto p_block_heavy_count_buf = gp_render_device->create_byteaddress_buffer(sizeof(uint) * (ceil_div(primitive_count, 256) + 1), resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access));
	auto p_block_light_sum_weight_buf = gp_render_device->create_byteaddress_buffer(sizeof(uint) * (ceil_div(primitive_count, 256) + 1), resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access | resource_flag_scratch));
	auto p_block_heavy_sum_weight_buf = gp_render_device->create_byteaddress_buffer(sizeof(uint) * (ceil_div(primitive_count, 256) + 1), resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access | resource_flag_scratch));

	auto p_weight_srv = gp_render_device->create_shader_resource_view(*p_weight_buf, buffer_srv_desc(*p_weight_buf));
	auto p_light_index_srv = gp_render_device->create_shader_resource_view(*p_light_index_buf, buffer_srv_desc(*p_light_index_buf));
	auto p_heavy_index_srv = gp_render_device->create_shader_resource_view(*p_heavy_index_buf, buffer_srv_desc(*p_heavy_index_buf));
	auto p_light_sum_weight_srv = gp_render_device->create_shader_resource_view(*p_light_sum_weight_buf, buffer_srv_desc(*p_light_sum_weight_buf));
	auto p_heavy_sum_weight_srv = gp_render_device->create_shader_resource_view(*p_heavy_sum_weight_buf, buffer_srv_desc(*p_heavy_sum_weight_buf));
	auto p_sum_weight_srv = gp_render_device->create_shader_resource_view(*p_sum_weight_buf, buffer_srv_desc(*p_sum_weight_buf));
	auto p_max_weight_srv = gp_render_device->create_shader_resource_view(*p_max_weight_buf, buffer_srv_desc(*p_max_weight_buf));
	auto p_block_light_count_srv = gp_render_device->create_shader_resource_view(*p_block_light_count_buf, buffer_srv_desc(*p_block_light_count_buf));
	auto p_block_heavy_count_srv = gp_render_device->create_shader_resource_view(*p_block_heavy_count_buf, buffer_srv_desc(*p_block_heavy_count_buf));
	auto p_block_light_sum_weight_srv = gp_render_device->create_shader_resource_view(*p_block_light_sum_weight_buf, buffer_srv_desc(*p_block_light_sum_weight_buf));
	auto p_block_heavy_sum_weight_srv = gp_render_device->create_shader_resource_view(*p_block_heavy_sum_weight_buf, buffer_srv_desc(*p_block_heavy_sum_weight_buf));

	auto p_weight_uav = gp_render_device->create_unordered_access_view(*p_weight_buf, buffer_uav_desc(*p_weight_buf));
	auto p_light_index_uav = gp_render_device->create_unordered_access_view(*p_light_index_buf, buffer_uav_desc(*p_light_index_buf));
	auto p_heavy_index_uav = gp_render_device->create_unordered_access_view(*p_heavy_index_buf, buffer_uav_desc(*p_heavy_index_buf));
	auto p_light_sum_weight_uav = gp_render_device->create_unordered_access_view(*p_light_sum_weight_buf, buffer_uav_desc(*p_light_sum_weight_buf));
	auto p_heavy_sum_weight_uav = gp_render_device->create_unordered_access_view(*p_heavy_sum_weight_buf, buffer_uav_desc(*p_heavy_sum_weight_buf));
	auto p_sum_weight_uav = gp_render_device->create_unordered_access_view(*p_sum_weight_buf, buffer_uav_desc(*p_sum_weight_buf));
	auto p_max_weight_uav = gp_render_device->create_unordered_access_view(*p_max_weight_buf, buffer_uav_desc(*p_max_weight_buf));
	auto p_block_light_count_uav = gp_render_device->create_unordered_access_view(*p_block_light_count_buf, buffer_uav_desc(*p_block_light_count_buf));
	auto p_block_heavy_count_uav = gp_render_device->create_unordered_access_view(*p_block_heavy_count_buf, buffer_uav_desc(*p_block_heavy_count_buf));
	auto p_block_light_sum_weight_uav = gp_render_device->create_unordered_access_view(*p_block_light_sum_weight_buf, buffer_uav_desc(*p_block_light_sum_weight_buf));
	auto p_block_heavy_sum_weight_uav = gp_render_device->create_unordered_access_view(*p_block_heavy_sum_weight_buf, buffer_uav_desc(*p_block_heavy_sum_weight_buf));

	auto p_debug_buf = gp_render_device->create_byteaddress_buffer(sizeof(uint4) * primitive_count, resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access | resource_flag_scratch));
	auto p_debug_uav = gp_render_device->create_unordered_access_view(*p_debug_buf, buffer_uav_desc(*p_debug_buf));
	context.set_pipeline_resource("debug_uav", *p_debug_uav);

	struct cbuffer
	{
		uint	primitive_count;
		uint	raytracing_instance_index;
		uint	bindless_instance_index;
		float	power;
	};
	cbuffer cbuffer;
	cbuffer.primitive_count = primitive_count;
	cbuffer.raytracing_instance_index = raytracing_instance_index;
	cbuffer.bindless_instance_index = bindless_instance_index;
	cbuffer.power = power;
	
	auto p_cbuf = gp_render_device->create_temporary_cbuffer(sizeof(cbuffer), &cbuffer);
	context.set_pipeline_resource("emissive_blas_cbuf", *p_cbuf);

	gp_raytracing_manager->bind(context); //ちょっと嫌...

	push_priority push_priority(context);
	context.set_priority(priority_initiaize);
	context.set_pipeline_resource("weight_uav", *p_weight_uav);
	context.set_pipeline_resource("max_weight_uav", *p_max_weight_uav);
	context.set_pipeline_state(*m_shaders.get("initialize_blas"));
	context.dispatch(ceil_div(primitive_count, 256), 1, 1);
	
	context.set_pipeline_resource("weight_srv", *p_weight_srv);
	context.set_pipeline_resource("max_weight_srv", *p_max_weight_srv);
	context.set_pipeline_resource("sum_weight_uav", *p_sum_weight_uav);
	context.set_pipeline_state(*m_shaders.get("initialize_blas2"));
	context.dispatch(ceil_div(primitive_count, 256), 1, 1);

	context.set_pipeline_resource("weight_srv", *p_weight_srv);
	context.set_pipeline_resource("sum_weight_srv", *p_sum_weight_srv);
	context.set_pipeline_resource("max_weight_srv", *p_max_weight_srv);
	context.set_pipeline_resource("light_index_uav", *p_light_index_uav);
	context.set_pipeline_resource("heavy_index_uav", *p_heavy_index_uav);
	context.set_pipeline_resource("block_light_count_uav", *p_block_light_count_uav);
	context.set_pipeline_resource("block_heavy_count_uav", *p_block_heavy_count_uav);
	context.set_pipeline_resource("block_light_sum_weight_uav", *p_block_light_sum_weight_uav);
	context.set_pipeline_resource("block_heavy_sum_weight_uav", *p_block_heavy_sum_weight_uav);
	context.set_pipeline_state(*m_shaders.get("scan1"));
	context.dispatch(ceil_div(primitive_count, 256), 1, 1);
	
	context.set_pipeline_resource("block_light_count_uav", *p_block_light_count_uav);
	context.set_pipeline_resource("block_heavy_count_uav", *p_block_heavy_count_uav);
	context.set_pipeline_resource("block_light_sum_weight_uav", *p_block_light_sum_weight_uav);
	context.set_pipeline_resource("block_heavy_sum_weight_uav", *p_block_heavy_sum_weight_uav);
	context.set_pipeline_state(*m_shaders.get("scan2"));
	context.dispatch(1, 1, 1);

	context.set_pipeline_resource("weight_srv", *p_weight_srv);
	context.set_pipeline_resource("sum_weight_srv", *p_sum_weight_srv);
	context.set_pipeline_resource("max_weight_srv", *p_max_weight_srv);
	context.set_pipeline_resource("light_index_uav", *p_light_index_uav);
	context.set_pipeline_resource("heavy_index_uav", *p_heavy_index_uav);
	context.set_pipeline_resource("block_light_count_srv", *p_block_light_count_srv);
	context.set_pipeline_resource("block_heavy_count_srv", *p_block_heavy_count_srv);
	context.set_pipeline_resource("block_light_sum_weight_srv", *p_block_light_sum_weight_srv);
	context.set_pipeline_resource("block_heavy_sum_weight_srv", *p_block_heavy_sum_weight_srv);
	context.set_pipeline_resource("light_index_uav", *p_light_index_uav);
	context.set_pipeline_resource("heavy_index_uav", *p_heavy_index_uav);
	context.set_pipeline_resource("light_sum_weight_uav", *p_light_sum_weight_uav);
	context.set_pipeline_resource("heavy_sum_weight_uav", *p_heavy_sum_weight_uav);
	context.set_pipeline_state(*m_shaders.get("scan3"));
	context.dispatch(ceil_div(primitive_count, 256), 1, 1);

	context.set_pipeline_resource("blas_uav", *mp_blas_uav);
	context.set_pipeline_resource("weight_srv", *p_weight_srv);
	context.set_pipeline_resource("sum_weight_srv", *p_sum_weight_srv);
	context.set_pipeline_resource("max_weight_srv", *p_max_weight_srv);
	context.set_pipeline_resource("light_index_srv", *p_light_index_srv);
	context.set_pipeline_resource("heavy_index_srv", *p_heavy_index_srv);
	context.set_pipeline_resource("light_sum_weight_srv", *p_light_sum_weight_srv);
	context.set_pipeline_resource("heavy_sum_weight_srv", *p_heavy_sum_weight_srv);
	context.set_pipeline_resource("block_light_count_srv", *p_block_light_count_srv);
	context.set_pipeline_resource("block_heavy_count_srv", *p_block_heavy_count_srv);
	context.set_pipeline_state(*m_shaders.get("build_blas"));
	context.dispatch(ceil_div(primitive_count, 256), 1, 1);


#else

	const uint work_per_thread = 8;
	const uint state_count = ceil_div(primitive_count, work_per_thread);
	auto p_weight_buf = gp_render_device->create_byteaddress_buffer(sizeof(float) * primitive_count, resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access | resource_flag_scratch));
	auto p_light_index_buf = gp_render_device->create_byteaddress_buffer(sizeof(uint) * primitive_count, resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access | resource_flag_scratch));
	auto p_heavy_index_buf = gp_render_device->create_byteaddress_buffer(sizeof(uint) * primitive_count, resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access | resource_flag_scratch));
	auto p_light_sum_weight_buf = gp_render_device->create_byteaddress_buffer(sizeof(uint) * primitive_count, resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access | resource_flag_scratch));
	auto p_heavy_sum_weight_buf = gp_render_device->create_byteaddress_buffer(sizeof(uint) * primitive_count, resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access | resource_flag_scratch));
	auto p_sum_weight_buf = gp_render_device->create_byteaddress_buffer(sizeof(uint), resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access));
	auto p_max_weight_buf = gp_render_device->create_byteaddress_buffer(sizeof(float), resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access));
	auto p_scan_scratch_buf = gp_render_device->create_byteaddress_buffer(sizeof(uint4), resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access));

	auto p_weight_srv = gp_render_device->create_shader_resource_view(*p_weight_buf, buffer_srv_desc(*p_weight_buf));
	auto p_light_index_srv = gp_render_device->create_shader_resource_view(*p_light_index_buf, buffer_srv_desc(*p_light_index_buf));
	auto p_heavy_index_srv = gp_render_device->create_shader_resource_view(*p_heavy_index_buf, buffer_srv_desc(*p_heavy_index_buf));
	auto p_light_sum_weight_srv = gp_render_device->create_shader_resource_view(*p_light_sum_weight_buf, buffer_srv_desc(*p_light_sum_weight_buf));
	auto p_heavy_sum_weight_srv = gp_render_device->create_shader_resource_view(*p_heavy_sum_weight_buf, buffer_srv_desc(*p_heavy_sum_weight_buf));
	auto p_sum_weight_srv = gp_render_device->create_shader_resource_view(*p_sum_weight_buf, buffer_srv_desc(*p_sum_weight_buf));
	auto p_max_weight_srv = gp_render_device->create_shader_resource_view(*p_max_weight_buf, buffer_srv_desc(*p_max_weight_buf));
	auto p_scan_scratch_srv = gp_render_device->create_shader_resource_view(*p_scan_scratch_buf, buffer_srv_desc(*p_scan_scratch_buf));

	auto p_weight_uav = gp_render_device->create_unordered_access_view(*p_weight_buf, buffer_uav_desc(*p_weight_buf));
	auto p_light_index_uav = gp_render_device->create_unordered_access_view(*p_light_index_buf, buffer_uav_desc(*p_light_index_buf));
	auto p_heavy_index_uav = gp_render_device->create_unordered_access_view(*p_heavy_index_buf, buffer_uav_desc(*p_heavy_index_buf));
	auto p_light_sum_weight_uav = gp_render_device->create_unordered_access_view(*p_light_sum_weight_buf, buffer_uav_desc(*p_light_sum_weight_buf));
	auto p_heavy_sum_weight_uav = gp_render_device->create_unordered_access_view(*p_heavy_sum_weight_buf, buffer_uav_desc(*p_heavy_sum_weight_buf));
	auto p_sum_weight_uav = gp_render_device->create_unordered_access_view(*p_sum_weight_buf, buffer_uav_desc(*p_sum_weight_buf));
	auto p_max_weight_uav = gp_render_device->create_unordered_access_view(*p_max_weight_buf, buffer_uav_desc(*p_max_weight_buf));
	auto p_scan_scratch_uav = gp_render_device->create_unordered_access_view(*p_scan_scratch_buf, buffer_uav_desc(*p_scan_scratch_buf));

	auto p_debug_buf = gp_render_device->create_byteaddress_buffer(sizeof(uint4) * primitive_count, resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access | resource_flag_scratch));
	auto p_debug_uav = gp_render_device->create_unordered_access_view(*p_debug_buf, buffer_uav_desc(*p_debug_buf));
	context.set_pipeline_resource("debug_uav", *p_debug_uav);

	struct cbuffer
	{
		uint	primitive_count;
		uint	raytracing_instance_index;
		uint	bindless_instance_index;
		float	power;
	};
	cbuffer cbuffer;
	cbuffer.primitive_count = primitive_count;
	cbuffer.raytracing_instance_index = raytracing_instance_index;
	cbuffer.bindless_instance_index = bindless_instance_index;
	cbuffer.power = power;
	
	auto p_cbuf = gp_render_device->create_temporary_cbuffer(sizeof(cbuffer), &cbuffer);
	context.set_pipeline_resource("emissive_blas_cbuf", *p_cbuf);

	gp_raytracing_manager->bind(context); //ちょっと嫌...

	push_priority push_priority(context);
	context.set_priority(priority_initiaize);
	context.set_pipeline_resource("weight_uav", *p_weight_uav);
	context.set_pipeline_resource("max_weight_uav", *p_max_weight_uav);
	context.set_pipeline_state(*m_shaders.get("initialize_blas"));
	context.dispatch(ceil_div(primitive_count, 256), 1, 1);
	
	context.set_pipeline_resource("weight_srv", *p_weight_srv);
	context.set_pipeline_resource("max_weight_srv", *p_max_weight_srv);
	context.set_pipeline_resource("sum_weight_uav", *p_sum_weight_uav);
	context.set_pipeline_state(*m_shaders.get("initialize_blas2"));
	context.dispatch(ceil_div(primitive_count, 256), 1, 1);

	context.set_pipeline_resource("weight_srv", *p_weight_srv);
	context.set_pipeline_resource("sum_weight_srv", *p_sum_weight_srv);
	context.set_pipeline_resource("max_weight_srv", *p_max_weight_srv);
	context.set_pipeline_resource("light_index_uav", *p_light_index_uav);
	context.set_pipeline_resource("heavy_index_uav", *p_heavy_index_uav);
	context.set_pipeline_resource("scan_scratch_uav", *p_scan_scratch_uav);
	context.set_pipeline_resource("light_sum_weight_uav", *p_light_sum_weight_uav);
	context.set_pipeline_resource("heavy_sum_weight_uav", *p_heavy_sum_weight_uav);
	context.set_pipeline_state(*m_shaders.get("scan"));
	context.dispatch(ceil_div(primitive_count, 256), 1, 1);

	context.set_pipeline_resource("blas_uav", *mp_blas_uav);
	context.set_pipeline_resource("weight_srv", *p_weight_srv);
	context.set_pipeline_resource("sum_weight_srv", *p_sum_weight_srv);
	context.set_pipeline_resource("max_weight_srv", *p_max_weight_srv);
	context.set_pipeline_resource("light_index_srv", *p_light_index_srv);
	context.set_pipeline_resource("heavy_index_srv", *p_heavy_index_srv);
	context.set_pipeline_resource("scan_scratch_srv", *p_scan_scratch_srv);
	context.set_pipeline_resource("light_sum_weight_srv", *p_light_sum_weight_srv);
	context.set_pipeline_resource("heavy_sum_weight_srv", *p_heavy_sum_weight_srv);
	context.set_pipeline_state(*m_shaders.get("build_blas"));
	context.dispatch(ceil_div(primitive_count, 256), 1, 1);
#endif
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//発光パワーを更新
inline void emissive_blas::update(render_context& context, const float power)
{
	push_priority push_priority(context);
	context.set_priority(priority_initiaize);
	*static_cast<float*>(context.update_buffer(*mp_blas_buf, 8, 4)) = power;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

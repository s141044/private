
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//emissive_sampler
///////////////////////////////////////////////////////////////////////////////////////////////////

//コンストラクタ
inline emissive_sampler::emissive_sampler()
{
	m_shaders = gp_shader_manager->create(L"emissive_sampling.sdf.json");
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//サンプリング
inline bool emissive_sampler::sample(render_context& context, const uint sample_count)
{
	if(m_shaders.has_update())
		m_shaders.update();
	else if(m_shaders.is_invalid())
		return false;

	if((mp_emissive_sample_buf == nullptr) || (mp_emissive_sample_buf->num_elements() != sample_count))
	{
		mp_emissive_sample_buf = gp_render_device->create_structured_buffer(sizeof(sample_t), sample_count, resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access | resource_flag_scratch));
		mp_emissive_sample_srv = gp_render_device->create_shader_resource_view(*mp_emissive_sample_buf, buffer_srv_desc(*mp_emissive_sample_buf));
		mp_emissive_sample_uav = gp_render_device->create_unordered_access_view(*mp_emissive_sample_buf, buffer_uav_desc(*mp_emissive_sample_buf));
		gp_render_device->set_name(*mp_emissive_sample_buf, L"emissive_sample");
	}

	context.set_pipeline_state(*m_shaders.get("emissive_presample"));
	context.set_pipeline_resource("emissive_sample_uav", *mp_emissive_sample_uav);
	context.dispatch_with_32bit_constant(ceil_div(sample_count, 256), 1, 1, sample_count);
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//リソースをバインド
inline void emissive_sampler::bind(render_context& context)
{
	context.set_pipeline_resource("emissive_sample_srv", *mp_emissive_sample_srv);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

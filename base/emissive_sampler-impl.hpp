
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

//デバッグ描画
inline void emissive_sampler::debug_draw(render_context& context, const float scale)
{
	if(m_debug_shaders.is_empty())
		m_debug_shaders = gp_shader_manager->create(L"emissive_sampling_debug.sdf.json");
	if(m_debug_shaders.has_update())
		m_debug_shaders.update();
	else if(m_debug_shaders.is_invalid())
		return;

	const uint theta_div = 30;
	const uint phi_div = 120;

	uint index_count = 6 * theta_div * phi_div;
	uint vertex_count = (theta_div + 1) * (phi_div + 1);

	if(mp_debug_gs == nullptr)
	{
		const float dth = (PI() / 2) / theta_div;
		const float dph = (PI() * 2) / phi_div;

		vector<float3> vertices(vertex_count);
		vector<uint16_t> indexes(index_count);
		index_count = 0;
		vertex_count = 0;

		for(uint i = 0; i <= theta_div; i++)
		{
			const float th = dth * i;
			const float st = sin(th);
			const float ct = cos(th);

			for(uint j = 0; j <= phi_div; j++)
			{
				const float ph = dph * j;
				const float sp = sin(ph);
				const float cp = cos(ph);
				vertices[vertex_count++] = float3(st * cp, st * sp, ct);
			}
		}
		for(uint i = 0; i < theta_div; i++)
		{
			for(uint j = 0; j < phi_div; j++)
			{
				const uint i0 = (phi_div + 1) * (i + 0) + (j + 0);
				const uint i1 = (phi_div + 1) * (i + 1) + (j + 0);
				const uint i2 = (phi_div + 1) * (i + 1) + (j + 1);
				const uint i3 = (phi_div + 1) * (i + 0) + (j + 1);

				if(i != 0)
				{
					indexes[index_count++] = i0;
					indexes[index_count++] = i1;
					indexes[index_count++] = i2;
				}
				if(i != theta_div - 1)
				{
					indexes[index_count++] = i0;
					indexes[index_count++] = i2;
					indexes[index_count++] = i3;
				}
			}
		}

		mp_debug_vb = gp_render_device->create_structured_buffer(sizeof(float3), vertex_count, resource_flag_allow_shader_resource, vertices.data());
		mp_debug_ib = gp_render_device->create_structured_buffer(sizeof(uint16_t), index_count, resource_flag_allow_shader_resource, indexes.data());

		buffer *vb_ptrs[] = { mp_debug_vb.get() };
		uint offset = 0, stride = sizeof(float3);
		mp_debug_gs = gp_render_device->create_geometry_state(1, vb_ptrs, &offset, &stride, mp_debug_ib.get());
	}

	gp_debug_draw->draw([&, index_count, scale]()
	{
		context.set_geometry_state(*mp_debug_gs);
		context.set_pipeline_state(*m_debug_shaders.get("debug_draw"));
		context.draw_indexed_with_32bit_constant(index_count, mp_emissive_sample_buf->num_elements(), 0, 0, reinterpret<uint>(scale));
	});
}

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

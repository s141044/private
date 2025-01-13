
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//mesh
///////////////////////////////////////////////////////////////////////////////////////////////////

//コンストラクタ
inline mesh::mesh()
{
	m_shaders = gp_shader_manager->create(L"mesh.sdf.json");
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//更新
inline void mesh::update(render_context &context, const float dt)
{
	update_geometry(context);
	if(gp_raytracing_manager)
		update_raytracing(context);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//描画
inline void mesh::draw(render_context &context, draw_type type)
{
	if(m_shaders.has_update())
		m_shaders.update();
	else if(m_shaders.is_invalid())
		return;

	context.set_pipeline_resource("draw_cb", *gp_render_device->create_temporary_cbuffer(sizeof(float4x4), &ltow_matrix()));
	context.set_geometry_state(geometry_state());

	switch(type){
	case draw_type_mask: context.set_pipeline_state(*m_shaders.get("draw_mask")); break;
	}
	for(auto &cluster : m_clusters)
	{
		context.draw_indexed(cluster.index_count, 1, cluster.start_index_location, cluster.base_vertex_location);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//レイトレ用データの更新
inline void mesh::update_raytracing(render_context &context)
{
	const bool is_static = (m_gs_ptrs[0] == m_gs_ptrs[1]);
	const bool is_first_call = m_blas_ptrs.empty();
	if(is_first_call)
	{
		uint num_blas = 0;
		uint num_descs[material::blas_group_key_max] = {};
		for(auto &cluster : m_clusters)
		{
			if(num_descs[m_material_ptrs[cluster.material_index]->blas_group_key()]++ == 0)
				num_blas++;
		}
		uint sum = 0;
		uint offsets[material::blas_group_key_max + 1];
		for(uint i = 0; i < material::blas_group_key_max; i++)
		{
			const uint tmp = std::exchange(num_descs[i], 0);
			offsets[i] = std::exchange(sum, sum + tmp);
		}
		offsets[material::blas_group_key_max] = sum;
	
		m_blas_ptrs.resize(num_blas);
		num_blas = 0;

		const auto bindless_instance_count = uint(m_clusters.size());
		const auto raytracing_instance_count = uint(m_blas_ptrs.size());
		register_instance(bindless_instance_count, raytracing_instance_count);
		const auto bindless_instance_index = this->bindless_instance_index();
		const auto raytracing_instance_index = this->raytracing_instance_index();

		const uint vb_offsets[8] = { 0, 0, 4, 8, };
		m_bindless_gs_ptrs[0] = std::make_shared<render::bindless_geometry>(*m_gs_ptrs[0], vb_offsets);
		if(m_gs_ptrs[0] != m_gs_ptrs[1])
			m_bindless_gs_ptrs[1] = std::make_shared<render::bindless_geometry>(*m_gs_ptrs[1], vb_offsets);
		else
			m_bindless_gs_ptrs[1] = m_bindless_gs_ptrs[0];

		for(auto &p_material : m_material_ptrs)
			p_material->register_bindless();

		const auto &gs = geometry_state();
		const auto &ib = gs.index_buffer();
		const auto &vb = gs.vertex_buffer(0);
	
		std::vector<raytracing_geometry_desc> descs(sum);
		for(auto &cluster : m_clusters)
		{
			const auto &material = *m_material_ptrs[cluster.material_index];
			const uint key = material.blas_group_key();
			const uint offset = offsets[key] + num_descs[key]++;
	
			auto &desc = descs[offset];
			desc.index_count = cluster.index_count;
			desc.vertex_count = cluster.vertex_count;
			desc.start_index_location = cluster.start_index_location;
			desc.base_vertex_location = cluster.base_vertex_location;

			if(offset + 1 == offsets[key + 1])
			{
				auto &p_blas = m_blas_ptrs[num_blas];
				p_blas = gp_render_device->create_bottom_level_acceleration_structure(gs, &descs[offsets[key]], num_descs[key], not(is_static));
				
				auto *p_raytracing_instance_desc = gp_raytracing_manager->update_raytracing_instance(raytracing_instance_index + num_blas);
				p_raytracing_instance_desc->id = bindless_instance_index + offsets[key];
				p_raytracing_instance_desc->mask = raytracing_instance_mask_default;
				p_raytracing_instance_desc->flags = raytracing_instance_flag_triangle_front_ccw;
				p_raytracing_instance_desc->blas_address = p_blas->gpu_virtual_address();
				memcpy(p_raytracing_instance_desc->transform, &ltow_matrix(), sizeof(float4) * 3);
				num_blas++;
			}

			auto *p_bindless_instance_desc = gp_raytracing_manager->update_bindless_instance(bindless_instance_index + offset);
			p_bindless_instance_desc->start_index_location = cluster.start_index_location;
			p_bindless_instance_desc->base_vertex_location = cluster.base_vertex_location;
			p_bindless_instance_desc->bindless_geometry_handle = bindless_geometry().bindless_handle();
			p_bindless_instance_desc->bindless_material_handle = material.bindless_handle();
		}
	
		context.set_priority(priority_build_bottom_level_acceleration_structure);
		for(auto &p_blas : m_blas_ptrs){ context.build_bottom_level_acceleration_structure(*p_blas); }
	}

	if(is_static)
	{
		//コンパクション
		if(not(m_compaction_completed))
		{
			m_compaction_completed = true;
	
			context.set_priority(priority_compact_bottom_level_acceleration_structure);
			for(uint i = 0; i < uint(m_blas_ptrs.size()); i++)
			{
				auto &p_blas = m_blas_ptrs[i];
				auto state = p_blas->compaction_state();
				if(state == raytracing_compaction_state_not_support)
					continue;
	
				if(state == raytracing_compaction_state_completed)
				{
					auto *p_raytracing_instance_desc = gp_raytracing_manager->update_raytracing_instance(raytracing_instance_index() + i);
					p_raytracing_instance_desc->blas_address = p_blas->gpu_virtual_address();
					continue;
				}
	
				m_compaction_completed = false;

				if(state >= raytracing_compaction_state_ready)
					context.compact_bottom_level_acceleration_structure(*p_blas);
			}
		}
	}
	else
	{
		//リフィット
		if(not(is_first_call))
		{
			context.set_priority(priority_refit_bottom_level_acceleration_structure);
			for(auto &p_blas : m_blas_ptrs)
				context.refit_bottom_level_acceleration_structure(*p_blas, &geometry_state());
	
			for(uint i = 0; i < uint(m_clusters.size()); i++)
			{
				auto *p_bindless_geometry_desc = gp_raytracing_manager->update_bindless_instance(bindless_instance_index() + i);
				p_bindless_geometry_desc->bindless_geometry_handle = bindless_geometry().bindless_handle();
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//法線を圧縮
inline uint mesh::encode_normal(const float3 &normal)
{
	return f32x2_to_u16x2_unorm(f32x3_to_oct(normal) * 0.5f + 0.5f);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//接線を圧縮
inline uint mesh::encode_tangent(const float3 &tangent, const float3 &binormal, const float3 &normal)
{
	const float2 v = f32x3_to_oct(tangent) * 0.5f + 0.5f;
	return f32x2_to_u16x2_snorm((dot(binormal, cross(normal, tangent)) >= 0) ? v : -v);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//UVを圧縮
//inline uint mesh::encode_uv(const float2 &uv)
//{
//#if 1
//	//絶対値が大きくなると精度が悪すぎる
//	half u(uv[0]);
//	half v(uv[1]);
//#else
//	//-40~40くらいである程度の精度(もっと範囲を絞って固定小数点にする？)
//	half u(uv[0] * uv[0] * uv[0]);
//	half v(uv[1] * uv[1] * uv[1]);
//#endif
//	return reinterpret<uint16_t>(u) | (reinterpret<uint16_t>(v) << 16);
//}

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

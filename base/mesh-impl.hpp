
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace anon{

///////////////////////////////////////////////////////////////////////////////////////////////////

static constexpr uint blas_group_key_max_count = 8;

///////////////////////////////////////////////////////////////////////////////////////////////////

//BLASのグループキーを返す
inline uint get_blas_group_key(const material& material)
{
	uint key = 0;
	if(material.has_alpha_map())
		key = 0x1;
	if(material.has_displacement_map() && (material.max_displacement() > 0))
		key |= 0x2;
	if(material.has_subsurface_scattering())
		key |= 0x4;
	return key;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//インスタンスマスクを返す
inline raytracing_instance_mask get_raytracing_instance_mask(const material& material)
{
	raytracing_instance_mask mask = raytracing_instance_mask_default;
	if(material.has_subsurface_scattering())
		mask = raytracing_instance_mask(mask | raytracing_instance_mask_subsurface);
	return mask;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//ジオメトリフラグを返す
inline raytracing_geometry_flags get_raytracing_geometry_flag(const material& material)
{
	raytracing_geometry_flags flags = raytracing_geometry_flag_none;
	if(not(material.has_alpha_map()))
		flags = raytracing_geometry_flags(flags | raytracing_geometry_flag_opaque);
	if(material.has_subsurface_scattering())
		flags = raytracing_geometry_flags(flags | raytracing_geometry_flag_no_duplicate_anyhit_invocation);
	return flags;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace anon

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
inline void mesh::update(render_context& context, const float dt)
{
	update_material(context);
	update_geometry(context);
	update_raytracing(context);
	update_emissive(context);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//描画
inline void mesh::draw_constant(render_context& context, float4 color, uint material_index)
{
	if(m_shaders.has_update())
		m_shaders.update();
	else if(m_shaders.is_invalid())
		return;

	struct cbuffer
	{
		float3x4	ltow;
		float4		color;
	};
	auto& cbuf = *gp_render_device->create_temporary_cbuffer(sizeof(cbuffer));
	auto& cbuf_data = *cbuf.data<cbuffer>();
	cbuf_data.ltow = ltow_matrix();
	cbuf_data.color = color;

	context.set_pipeline_resource("draw_cb", cbuf);
	context.set_geometry_state(geometry_state());
	context.set_pipeline_state(*m_shaders.get("draw_constant"));

	for(auto& cluster : m_clusters)
	{
		if((material_index == -1) || (cluster.material_index == material_index))
			context.draw_indexed(cluster.index_count, 1, cluster.start_index_location, cluster.base_vertex_location);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//マテリアルの更新
inline void mesh::update_material(render_context& context)
{
	if(m_blas_group_keys.empty())
	{
		m_blas_group_keys.resize(m_material_ptrs.size(), -1);
		for(auto& p_mtl : m_material_ptrs)
			p_mtl->register_bindless();
	}

	bool need_initialize = false;
	m_material_changed = false;
	m_has_displacement_map = false;
	for(uint i = 0; i < uint(m_material_ptrs.size()); i++)
	{
		auto& material = *m_material_ptrs[i];
		if(material.has_update())
		{
			material.update(context);
			m_material_changed = true;

			const uint key = anon::get_blas_group_key(material);
			if(m_blas_group_keys[i] != key)
			{
				m_blas_group_keys[i] = key;
				need_initialize = true;
			}
		}
		if(material.has_displacement_map() && (material.max_displacement() > 0))
			m_has_displacement_map = true;
	}

	//TODO: refitで対応
	if(m_material_changed && m_has_displacement_map)
		need_initialize = true;

	if(need_initialize && m_blas_infos.size())
	{
		m_blas_infos.clear();
		gp_raytracing_manager->unregister_instance(bindless_instance_index(), raytracing_instance_index());

		for(auto& info : m_emissive_infos)
		{
			if(info.emissive_instance_index != 0xffffffff)
				gp_raytracing_manager->unregister_emissive(info.emissive_instance_index);
		}
		m_emissive_infos.clear();
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//Emissive用の更新
inline void mesh::update_emissive(render_context& context)
{
	if(m_emissive_infos.empty())
		return;

	for(size_t i = 0; i < m_clusters.size(); i++)
	{
		auto& cluster = m_clusters[i];
		auto& info = m_emissive_infos[i];

		float power = m_material_ptrs[cluster.material_index]->emissive_power();
		if(power > 0)
		{
			if(info.emissive_instance_index == 0xffffffff)
			{
				info.p_emissive_blas.reset(new emissive_blas());
				if(info.p_emissive_blas->build(context, raytracing_instance_index(), info.bindless_instance_index, cluster.index_count / 3, power))
					info.emissive_instance_index = gp_raytracing_manager->register_emissive(*info.p_emissive_blas);
			}
			else if(info.power != power)
				info.p_emissive_blas->update(context, power);
		}
		else if(info.emissive_instance_index != 0xffffffff)
		{
			info.p_emissive_blas.reset();
			gp_raytracing_manager->unregister_emissive(info.emissive_instance_index);
			info.emissive_instance_index = 0xffffffff;
		}
		info.power = power;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//レイトレ用データの更新
inline void mesh::update_raytracing(render_context& context)
{
	if(m_bindless_gs_ptrs[0] == nullptr)
	{
		const uint vb_offsets[8] = { 0, 0, 4, 8, };
		m_bindless_gs_ptrs[0] = std::make_shared<render::bindless_geometry>(*m_gs_ptrs[0], vb_offsets);
		if(m_gs_ptrs[0] != m_gs_ptrs[1])
			m_bindless_gs_ptrs[1] = std::make_shared<render::bindless_geometry>(*m_gs_ptrs[1], vb_offsets);
		else
			m_bindless_gs_ptrs[1] = m_bindless_gs_ptrs[0];
	}

	if(not(update_displacement(context)))
		return;

	const bool is_static = (m_gs_ptrs[0] == m_gs_ptrs[1]);
	const bool is_first_call = m_blas_infos.empty();
	if(is_first_call)
	{
		uint num_blas = 0;
		uint num_descs[anon::blas_group_key_max_count] = {};
		for(auto& cluster : m_clusters)
		{
			if(num_descs[anon::get_blas_group_key(*m_material_ptrs[cluster.material_index])]++ == 0)
				num_blas++;
		}
		uint sum = 0;
		uint offsets[anon::blas_group_key_max_count + 1];
		for(uint i = 0; i < anon::blas_group_key_max_count; i++)
		{
			const uint tmp = std::exchange(num_descs[i], 0);
			offsets[i] = std::exchange(sum, sum + tmp);
		}
		offsets[anon::blas_group_key_max_count] = sum;
	
		m_blas_infos.resize(num_blas);
		num_blas = 0;

		const auto bindless_instance_count = uint(m_clusters.size());
		const auto raytracing_instance_count = uint(m_blas_infos.size());
		register_instance(bindless_instance_count, raytracing_instance_count);
		const auto bindless_instance_index = this->bindless_instance_index();
		const auto raytracing_instance_index = this->raytracing_instance_index();
		const auto& gs = geometry_state();

		std::vector<raytracing_geometry_desc> descs(sum);
		for(auto& cluster : m_clusters)
		{
			const auto& material = *m_material_ptrs[cluster.material_index];
			const uint key = anon::get_blas_group_key(material);
			const uint offset = offsets[key] + num_descs[key]++;

			auto& desc = descs[offset];
			desc.flags = anon::get_raytracing_geometry_flag(material);
			if(material.has_displacement_map() && (material.max_displacement() > 0))
			{
				desc.type = raytracing_geometry_type_procedural;
				desc.aabbs.p_buf = mp_aabb_buf.get();
				desc.aabbs.count = cluster.index_count / 3;
				desc.aabbs.start_location = cluster.start_index_location / 3;
			}
			else
			{
				desc.type = raytracing_geometry_type_triangles;
				desc.triangles.index_count = cluster.index_count;
				desc.triangles.vertex_count = cluster.vertex_count;
				desc.triangles.start_index_location = cluster.start_index_location;
				desc.triangles.base_vertex_location = cluster.base_vertex_location;
			}

			if(offset + 1 == offsets[key + 1])
			{
				auto& info = m_blas_infos[num_blas];
				info.p_blas = gp_render_device->create_bottom_level_acceleration_structure(gs, &descs[offsets[key]], num_descs[key], not(is_static));

				auto* p_raytracing_instance_desc = gp_raytracing_manager->update_raytracing_instance(raytracing_instance_index + num_blas);
				p_raytracing_instance_desc->id = bindless_instance_index + offsets[key];
				p_raytracing_instance_desc->mask = anon::get_raytracing_instance_mask(material);
				p_raytracing_instance_desc->flags = raytracing_instance_flag_triangle_front_ccw;
				p_raytracing_instance_desc->blas_address = info.p_blas->gpu_virtual_address();

				const auto ltow = ltow_matrix();
				memcpy(p_raytracing_instance_desc->transform, &ltow, sizeof(float4) * 3);
				num_blas++;
			}

			auto* p_bindless_instance_desc = gp_raytracing_manager->update_bindless_instance(bindless_instance_index + offset);
			p_bindless_instance_desc->start_index_location = cluster.start_index_location;
			p_bindless_instance_desc->base_vertex_location = cluster.base_vertex_location;
			p_bindless_instance_desc->bindless_geometry_handle = bindless_geometry().bindless_handle();
			p_bindless_instance_desc->bindless_material_handle = material.bindless_handle();

			m_emissive_infos.emplace_back();
			m_emissive_infos.back().emissive_instance_index = 0xffffffff;
			m_emissive_infos.back().bindless_instance_index = bindless_instance_index + offset;
		}
	
		context.set_priority(priority_build_bottom_level_acceleration_structure);
		for(auto& info : m_blas_infos){ context.build_bottom_level_acceleration_structure(*info.p_blas); }

		m_update_frame = gp_render_device->frame_count();
	}
	else if(m_update_frame < transform::update_frame())
	{
		const auto ltow = ltow_matrix();
		for(uint i = 0; i < uint(m_blas_infos.size()); i++)
		{
			auto* p_raytracing_instance_desc = gp_raytracing_manager->update_raytracing_instance(raytracing_instance_index() + i);
			memcpy(p_raytracing_instance_desc->transform, &ltow, sizeof(float4) * 3);
		}
	}

	if(is_static)
	{
		//コンパクション
		if(not(m_compaction_completed))
		{
			m_compaction_completed = true;
	
			context.set_priority(priority_compact_bottom_level_acceleration_structure);
			for(uint i = 0; i < uint(m_blas_infos.size()); i++)
			{
				auto& p_blas = m_blas_infos[i].p_blas;
				auto state = p_blas->compaction_state();
				if(state == raytracing_compaction_state_not_support)
					continue;
	
				if(state == raytracing_compaction_state_completed)
				{
					auto* p_raytracing_instance_desc = gp_raytracing_manager->update_raytracing_instance(raytracing_instance_index() + i);
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
			for(auto& info : m_blas_infos)
				context.refit_bottom_level_acceleration_structure(*info.p_blas, &geometry_state());

			for(uint i = 0; i < uint(m_clusters.size()); i++)
			{
				auto* p_bindless_instance_desc = gp_raytracing_manager->update_bindless_instance(bindless_instance_index() + i);
				p_bindless_instance_desc->bindless_geometry_handle = bindless_geometry().bindless_handle();
			}
		}
	}
}

inline bool mesh::update_displacement(render_context& context)
{
	if(not(m_has_displacement_map))
	{
		mp_aabb_buf.reset();
		mp_aabb_uav.reset();
		return true;
	}
	
	if(m_shaders.has_update())
		m_shaders.update();
	else if(m_shaders.is_invalid())
		return false;

	if(mp_aabb_buf == nullptr)
	{
		uint count = 0;
		for(auto& cluster : m_clusters)
			count += cluster.index_count;

		mp_aabb_buf = gp_render_device->create_structured_buffer(sizeof(float3) * 2, count / 3, resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access));
		mp_aabb_uav = gp_render_device->create_unordered_access_view(*mp_aabb_buf, buffer_uav_desc(*mp_aabb_buf));
	}

	const bool is_static = (m_gs_ptrs[0] == m_gs_ptrs[1]);
	const bool is_first_call = m_blas_infos.empty();
	if(is_first_call || not(is_static) || m_material_changed)
	{
		context.set_priority(priority_after_compute_skinning);
		context.set_pipeline_resource("aabb_uav", *mp_aabb_uav);

		bool uav_barrier = true;
		for(auto& cluster : m_clusters)
		{
			auto& material = *m_material_ptrs[cluster.material_index];
			if(material.has_displacement_map() && (material.max_displacement() > 0))
			{
				struct cbuffer
				{
					uint	triangle_count;
					uint	base_vertex_location;
					uint	start_index_location;
					uint	bindless_geometry_handle;
					float	max_displacement;
					float3	padding;
				};
				auto p_cbuffer = gp_render_device->create_temporary_cbuffer(sizeof(cbuffer));
				auto& cbuf_data = *p_cbuffer->data<cbuffer>();
				cbuf_data.triangle_count = cluster.index_count / 3;
				cbuf_data.base_vertex_location = cluster.base_vertex_location;
				cbuf_data.start_index_location = cluster.start_index_location;
				cbuf_data.bindless_geometry_handle = bindless_geometry().bindless_handle();
				cbuf_data.max_displacement = m_material_ptrs[cluster.material_index]->max_displacement();

				context.set_pipeline_resource("calc_prism_aabbs_cb", *p_cbuffer);
				context.set_pipeline_state(*m_shaders.get("calc_prism_aabbs"));
				context.dispatch(ceil_div(cluster.index_count / 3, 256), 1, 1, uav_barrier);
				uav_barrier = false;
			}
		}
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//法線を圧縮
inline uint mesh::encode_normal(const float3& normal)
{
	return f32x2_to_u16x2_unorm(f32x3_to_oct(normal) * 0.5f + 0.5f);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//接線を圧縮
inline uint mesh::encode_tangent(const float3& tangent, const float3& binormal, const float3& normal)
{
	const float2 v = f32x3_to_oct(tangent) * 0.5f + 0.5f;
	return f32x2_to_u16x2_snorm((dot(binormal, cross(normal, tangent)) >= 0) ? v : -v);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//UVを圧縮
//inline uint mesh::encode_uv(const float2& uv)
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

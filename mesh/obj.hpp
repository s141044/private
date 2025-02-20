
#pragma once

#ifndef NN_RENDER_MESH_OBJ_HPP
#define NN_RENDER_MESH_OBJ_HPP

#include"../base.hpp"
#include<ns/objfile.hpp>
#include<ns/objfile/ts.hpp>

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//前方宣言
///////////////////////////////////////////////////////////////////////////////////////////////////

using objfile = ns::objfile;
using mtlfile = ns::mtlfile;

///////////////////////////////////////////////////////////////////////////////////////////////////
//obj_mesh
///////////////////////////////////////////////////////////////////////////////////////////////////

class obj_mesh : public mesh
{
public:

	//コンストラクタ
	obj_mesh(const objfile &file, const bool recalc_normals = false, const float smoothing_angle = 60)
	{
		using namespace ns;
		unordered_map<string, short> mtl_ids;
		for(const auto &mtl : file.mtllib().materials())
		{
			mtl_ids.emplace(mtl.name(), short(mtl_ids.size()));
		}

		uint num_indexes = 0;
		uint num_vertices = 0;
		vector<uint> subset_offsets(file.mtllib().materials().size() + 1, 0);
		vector<const objfile::subset*> subset_ptrs;
		for(const auto &grp : file.groups())
		{
			for(const auto &sub : grp.subsets())
			{
				num_indexes += sub.num_vertices() - 2 * sub.num_faces();
				num_vertices += sub.num_vertices();
				subset_offsets[mtl_ids[sub.mtl_name()]]++;
			}
		}
		num_indexes *= 3;
		for(size_t i = 1; i < subset_offsets.size(); i++)
		{
			subset_offsets[i] += subset_offsets[i - 1];
		}

		subset_ptrs.resize(subset_offsets.back());
		for(const auto &grp : file.groups())
		{
			for(const auto &sub : grp.subsets())
			{
				subset_ptrs[--subset_offsets[mtl_ids[sub.mtl_name()]]] = &sub;
			}
		}

		struct ntuv_t{ uint n, t; float2 uv; };
		vector<uint16_t> indexes(num_indexes);
		vector<uint32_t> vertices(num_vertices * (sizeof(float3) + sizeof(ntuv_t)));
		auto *positions = reinterpret_cast<float3*>(vertices.data());
		auto *ntuv = reinterpret_cast<ntuv_t*>(positions + num_vertices);

		uint index_count = 0;
		uint vertex_count = 0;
		uint start_index_location = 0;
		uint base_vertex_location = 0;

		tangent_space_calculator ts_calculator;
		for(uint mtl_id = 0; mtl_id < uint(subset_offsets.size() - 1); mtl_id++)
		{
			for(uint sub_id = subset_offsets[mtl_id]; sub_id < subset_offsets[mtl_id + 1]; sub_id++)
			{
				using namespace enum_op;
				const auto &sub = *subset_ptrs[sub_id];
				const bool has_vp = ((sub.face_type() & objfile::face_types::has_vp) != 0);
				const bool has_vt = ((sub.face_type() & objfile::face_types::has_vt) != 0);
				const bool has_vn = ((sub.face_type() & objfile::face_types::has_vn) != 0) && not(recalc_normals);
				ts_calculator.initialize(file, sub, smoothing_angle);

				for(int i = 0, nf = sub.num_faces(); i < nf; i++)
				{
					if(vertex_count + sub.num_vertices(i) - 1 >= USHRT_MAX) //USRT_MAXはストリップカット用なので使えない
					{
						auto &cluster = m_clusters.emplace_back();
						cluster.index_count = index_count;
						cluster.vertex_count = vertex_count;
						cluster.base_vertex_location = base_vertex_location;
						cluster.start_index_location = start_index_location;
						cluster.material_index = mtl_id;

						start_index_location += index_count;
						base_vertex_location += vertex_count;
						index_count = 0;
						vertex_count = 0;
					}

					for(int j = 0, nv = sub.num_vertices(i); j < nv; j++)
					{
						vec4 vp;
						if(has_vp)
							vp = vec4(file.positions().at(sub.position_index(i, j)), 1);

						vec4 vn;
						if(has_vn)
							vn = vec4(file.normals().at(sub.normal_index(i, j)), 0);

						vec2 vt;
						if(has_vt)
							vt = file.texcoords().at(sub.texcoord_index(i, j));
							
						const auto ts = ts_calculator.calculate(sub, i, j, has_vn ? &vn : nullptr);
						if(not(has_vn))
							vn = ts.axis_z();

						positions[base_vertex_location + vertex_count + j] = float3(vp);
						ntuv[base_vertex_location + vertex_count + j].n = encode_normal(float3(vn));
						ntuv[base_vertex_location + vertex_count + j].t = encode_tangent(float3(ts.axis_x()), float3(ts.axis_y()), float3(vn));
						ntuv[base_vertex_location + vertex_count + j].uv = float2(vt);
					}

					for(int j = 2, nv = sub.num_vertices(i); j < nv; j++)
					{
						const auto &p0 = positions[base_vertex_location + vertex_count + 0 + 0];
						const auto &p1 = positions[base_vertex_location + vertex_count + j - 1];
						const auto &p2 = positions[base_vertex_location + vertex_count + j - 0];
						if(square_norm(cross(p1 - p0, p2 - p0)) > 0)
						{
							indexes[start_index_location + index_count++] = uint16_t(vertex_count + 0 + 0);
							indexes[start_index_location + index_count++] = uint16_t(vertex_count + j - 1);
							indexes[start_index_location + index_count++] = uint16_t(vertex_count + j - 0);
						}
					}
					vertex_count += sub.num_vertices(i);
				}

				if(index_count > 0)
				{
					auto &cluster = m_clusters.emplace_back();
					cluster.index_count = index_count;
					cluster.vertex_count = vertex_count;
					cluster.base_vertex_location = base_vertex_location;
					cluster.start_index_location = start_index_location;
					cluster.material_index = mtl_id;

					start_index_location += index_count;
					base_vertex_location += vertex_count;
					index_count = 0;
					vertex_count = 0;
				}
			}
		}

		mp_ib = gp_render_device->create_structured_buffer(sizeof(uint16_t), num_indexes, resource_flag_allow_shader_resource, indexes.data());
		mp_vb = gp_render_device->create_structured_buffer(uint(sizeof(float3) + sizeof(ntuv_t)), num_vertices, resource_flag_allow_shader_resource, vertices.data());
		gp_render_device->set_name(*mp_ib, L"obj_mesh ib");
		gp_render_device->set_name(*mp_vb, L"obj_mesh vb");

		buffer *vb_ptrs[] = { mp_vb.get(), mp_vb.get(), mp_vb.get(), mp_vb.get(), };
		uint offsets[] = { 0, sizeof(float3) * num_vertices, sizeof(float3) * num_vertices, sizeof(float3) * num_vertices, };
		uint strides[] = { sizeof(float3), sizeof(ntuv_t), sizeof(ntuv_t), sizeof(ntuv_t), };
		mp_orig_gs = gp_render_device->create_geometry_state(4, vb_ptrs, offsets, strides, mp_ib.get());
		m_gs_ptrs[0] = m_gs_ptrs[1] = mp_orig_gs;
		
		//マテリアルはユーザが指定
		m_material_ptrs.resize(file.mtllib().materials().size(), nullptr);
	}

	//マテリアルを設定
	void set_material(const uint i, material_ptr p_mtl)
	{
		m_material_ptrs[i] = std::move(p_mtl);
	}

private:

	buffer_ptr	mp_vb;
	buffer_ptr	mp_ib;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

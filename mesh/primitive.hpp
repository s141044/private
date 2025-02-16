
#pragma once

#ifndef NN_RENDER_MESH_PRIMITIVE_HPP
#define NN_RENDER_MESH_PRIMITIVE_HPP

#include"../base.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//sphere_mesh
///////////////////////////////////////////////////////////////////////////////////////////////////

class sphere_mesh : public mesh
{
public:

	//コンストラクタ
	sphere_mesh(const uint theta_div = 45, const uint phi_div = 90)
	{
		const float dth = 1 * PI() / theta_div;
		const float dph = 2 * PI() / phi_div;

		struct ntuv_t{ uint n, t; float2 uv; };
		uint num_indexes = 6 * theta_div * phi_div;
		uint num_vertices = (theta_div + 1) * (phi_div + 1);
		vector<uint16_t> indexes(num_indexes);
		vector<uint32_t> vertices(num_vertices * (sizeof(float3) + sizeof(ntuv_t)));
		auto *positions = reinterpret_cast<float3*>(vertices.data());
		auto *ntuv = reinterpret_cast<ntuv_t*>(positions + num_vertices);
		num_indexes = 0;
		num_vertices = 0;

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

				const float3 p(st * cp, ct, st * sp);
				const float2 uv(j / float(phi_div), i / float(theta_div));
				const float3 binormal(ct * cp, -st, ct * sp);
				const float3 tangent = cross(binormal, p);

				positions[num_vertices] = p;
				ntuv[num_vertices].n = encode_normal(p);
				ntuv[num_vertices].t = encode_tangent(tangent, binormal, p);
				ntuv[num_vertices].uv = uv;
				num_vertices++;
			}
		}
		for(uint i = 0; i < theta_div; i++)
		{
			for(uint j = 0; j < phi_div; j++)
			{
				const uint i0 = (phi_div + 1) * (i + 0) + (j + 0);
				const uint i1 = (phi_div + 1) * (i + 0) + (j + 1);
				const uint i2 = (phi_div + 1) * (i + 1) + (j + 1);
				const uint i3 = (phi_div + 1) * (i + 1) + (j + 0);

				if(i != 0)
				{
					indexes[num_indexes++] = i0;
					indexes[num_indexes++] = i1;
					indexes[num_indexes++] = i2;
				}
				if(i != theta_div - 1)
				{
					indexes[num_indexes++] = i0;
					indexes[num_indexes++] = i2;
					indexes[num_indexes++] = i3;
				}
			}
		}

		mp_ib = gp_render_device->create_structured_buffer(sizeof(uint16_t), num_indexes, resource_flag_allow_shader_resource, indexes.data());
		mp_vb = gp_render_device->create_structured_buffer(uint(sizeof(float3) + sizeof(ntuv_t)), num_vertices, resource_flag_allow_shader_resource, vertices.data());
		gp_render_device->set_name(*mp_ib, L"sphere_mesh ib");
		gp_render_device->set_name(*mp_vb, L"sphere_mesh vb");

		buffer *vb_ptrs[] = { mp_vb.get(), mp_vb.get(), mp_vb.get(), mp_vb.get(), };
		uint offsets[] = { 0, sizeof(float3) * num_vertices, sizeof(float3) * num_vertices, sizeof(float3) * num_vertices, };
		uint strides[] = { sizeof(float3), sizeof(ntuv_t), sizeof(ntuv_t), sizeof(ntuv_t), };
		mp_orig_gs = gp_render_device->create_geometry_state(4, vb_ptrs, offsets, strides, mp_ib.get());
		m_gs_ptrs[0] = m_gs_ptrs[1] = mp_orig_gs;

		auto &cluster = m_clusters.emplace_back();
		cluster.index_count = num_indexes;
		cluster.vertex_count = num_vertices;
		cluster.material_index = 0;
		cluster.base_vertex_location = 0;
		cluster.start_index_location = 0;

		//マテリアルはユーザが指定
		m_material_ptrs.resize(1, nullptr);
	}

	//マテリアルを設定
	void set_material(material_ptr p_mtl)
	{
		m_material_ptrs[0] = std::move(p_mtl);
	}

private:

	buffer_ptr			mp_vb;
	buffer_ptr			mp_ib;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

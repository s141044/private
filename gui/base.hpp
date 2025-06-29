
#pragma once

#ifndef NN_RENDER_RENDER_GUI_BASE_HPP
#define NN_RENDER_RENDER_GUI_BASE_HPP

#include"../base.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace gui{

///////////////////////////////////////////////////////////////////////////////////////////////////
//transform
///////////////////////////////////////////////////////////////////////////////////////////////////

class transform : public property_editor
{
public:

	transform(render::transform& transform, string name = "transform") : property_editor(std::move(name)), m_transform(transform)
	{
	}

	void edit() override
	{
		float3 translation = m_transform.translation();
		if(input_float3("translation", translation)){ m_transform.set_translation(translation); }
		
		float3 rotation = m_transform.rotation();
		if(input_float3("rotation", rotation)){ m_transform.set_rotation(rotation); }
		
		float3 scaling = m_transform.scaling();
		if(input_float3("scaling", scaling)){ m_transform.set_scaling(scaling); }
	}
	
	void serialize(json_file::values_t& json)
	{
		write_float3(json, "translation", m_transform.translation());
		write_float3(json, "rotation", m_transform.rotation());
		write_float3(json, "scaling", m_transform.scaling());
	}

	void deserialize(const json_file::values_t& json)
	{
		float3 translation;
		if(read_float3(json, "translation", translation)){ m_transform.set_translation(translation); }

		float3 rotation;
		if(read_float3(json, "rotation", rotation)){ m_transform.set_rotation(rotation); }

		float3 scaling;
		if(read_float3(json, "scaling", scaling)){ m_transform.set_scaling(scaling); }
	}

private:

	render::transform& m_transform;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//mesh
///////////////////////////////////////////////////////////////////////////////////////////////////

class mesh : public property_editor
{
public:

	mesh(render::mesh& mesh, string name = "mesh") : property_editor(std::move(name)), m_transform(mesh), m_mesh(mesh)
	{
		m_material_ptrs.resize(mesh.material_ptrs().size());
		m_material_is_open.resize(mesh.material_ptrs().size());
	}

	void edit() override
	{
		m_transform.edit();

		for(uint i = 0; i < uint(m_material_ptrs.size()); i++)
		{
			auto& p_editor = m_material_ptrs[i];
			tree_node tree_node(p_editor->name().c_str());
			if(tree_node.is_open()){ p_editor->edit(); }

			//選択中のものの場合
			if((m_current_mesh_ptr == &m_mesh) && (m_current_material_index == i))
			{
				//選択状態を継続
				if(tree_node.is_open())
					m_frame_count = gp_render_device->frame_count();
				//選択状態を解除
				else
				{
					m_current_mesh_ptr = nullptr;
					m_current_material_index = uint(-1);
				}
			}
			//新規に選択状態にするか
			else if(m_material_is_open[i] != tree_node.is_open())
			{
				m_material_is_open[i] = tree_node.is_open();
				if(tree_node.is_open())
				{
					m_current_mesh_ptr = &m_mesh;
					m_current_material_index = i;
					m_frame_count = gp_render_device->frame_count();
				}
			}
		}
	}
	
	void serialize(json_file::values_t& json)
	{
		m_transform.serialize(json);
		for(auto& p_editor : m_material_ptrs)
		{
			auto* p_json_node = write_tree_node(json, p_editor->name().c_str());
			if(p_json_node){ p_editor->serialize(*p_json_node); }
		}
	}

	void deserialize(const json_file::values_t& json)
	{
		m_transform.deserialize(json);
		for(auto& p_editor : m_material_ptrs)
		{
			const auto* p_json_node = read_tree_node(json, p_editor->name().c_str());
			if(p_json_node){ p_editor->deserialize(*p_json_node); }
		}
	}

	void set_material(const uint i, shared_ptr<property_editor> p_editor)
	{
		m_material_ptrs[i] = std::move(p_editor);
	}

private:

	render::mesh&						m_mesh;
	transform							m_transform;
	vector<shared_ptr<property_editor>>	m_material_ptrs;
	vector<bool>						m_material_is_open;

public:

	static render::mesh* current_mesh_ptr()
	{
		return (m_frame_count == gp_render_device->frame_count()) ? m_current_mesh_ptr : nullptr;
	}
	static uint current_material_index()
	{
		return m_current_material_index;
	}

private:

	static inline render::mesh*			m_current_mesh_ptr = nullptr;
	static inline uint					m_current_material_index = -1;
	static inline uint					m_frame_count = 0;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//camera
///////////////////////////////////////////////////////////////////////////////////////////////////

class camera : public property_editor
{
public:

	camera(render::camera& camera, string name = "camera") : property_editor(std::move(name)), m_camera(camera)
	{
	}

	void edit() override
	{
		float3 position = m_camera.position();
		if(input_float3("position", position)){ m_camera.set_position(position); }

		float3 center = m_camera.center();
		if(input_float3("center", center)){ m_camera.look_at(position, center); }

		float distance = m_camera.distance();
		if(input_float("distance", distance)){ m_camera.look_at(position, normalize(center - position), std::max(distance, 1e-3f)); }

		float fovy = m_camera.fovy();
		if(slider_float("fovy", fovy, 1, 180)){ m_camera.set_fovy(fovy); }

		float near_clip = m_camera.near_clip();
		if(input_float("near_clip", near_clip)){ m_camera.set_near_clip(near_clip); }

		float far_clip = m_camera.far_clip();
		if(input_float("far_clip", far_clip)){ m_camera.set_far_clip(far_clip); }
	}
	
	void serialize(json_file::values_t& json)
	{
		write_float3(json, "position", m_camera.position());
		write_float3(json, "center", m_camera.center());
		write_float3(json, "fovy", m_camera.fovy());
		write_float3(json, "near_clip", m_camera.near_clip());
		write_float3(json, "far_clip", m_camera.far_clip());
	}

	void deserialize(const json_file::values_t& json)
	{
		float3 center = m_camera.center();
		float3 position = m_camera.position();
		read_float3(json, "center", center);
		read_float3(json, "position", position);
		m_camera.look_at(position, center);

		float fovy;
		if(read_float(json, "fovy", fovy)){ m_camera.set_fovy(fovy); }

		float near_clip;
		if(read_float(json, "near_clip", near_clip)){ m_camera.set_near_clip(near_clip); }

		float far_clip;
		if(read_float(json, "far_clip", far_clip)){ m_camera.set_far_clip(far_clip); }
	}

private:

	render::camera& m_camera;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace gui

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

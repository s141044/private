
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
	}

	void edit() override
	{
		m_transform.edit();
		for(auto& p_editor : m_material_ptrs)
		{
			tree_node tree_node(p_editor->name().c_str());
			if(tree_node.is_open()){ p_editor->edit(); }
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
		if(input_float("distance", distance)){ m_camera.look_at(position, normalize(center - position), distance); }

		float fovy = m_camera.fovy();
		if(slider_float("fovy", fovy, 1, 180)){ m_camera.set_fovy(fovy); }

		float near_clip = m_camera.near_clip();
		if(input_float("near_clip", near_clip)){ m_camera.set_near_clip(near_clip); }

		float far_clip = m_camera.far_clip();
		if(input_float("far_clip", far_clip)){ m_camera.set_far_clip(far_clip); }
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

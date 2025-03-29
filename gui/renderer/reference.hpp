
#pragma once

#ifndef NN_RENDER_GUI_RENDERER_REFERENCE_HPP
#define NN_RENDER_GUI_RENDERER_REFERENCE_HPP

#include"../../renderer/reference.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace gui{

///////////////////////////////////////////////////////////////////////////////////////////////////
//reference_renderer
///////////////////////////////////////////////////////////////////////////////////////////////////

class reference_renderer : public property_editor
{
public:

	reference_renderer(render::reference_renderer& ref, string name = "reference_renderer") : property_editor(std::move(name)), m_ref(ref)
	{
	}

	void edit() override
	{
		auto max_bounce = int(m_ref.max_bounce());
		if(input_int("max_bounce", max_bounce)){ m_ref.set_max_bounce(std::max(0, max_bounce)); }

		bool use_accumulation = m_ref.use_accumulation();
		if(checkbox("use_accumulation", use_accumulation)){ m_ref.set_use_accumulation(use_accumulation); }

		bool use_realistic_camera = m_ref.use_realistic_camera();
		if(checkbox("use_realistic_camera", use_realistic_camera)){ m_ref.set_use_realistic_camera(use_realistic_camera); }
	}
	
	void serialize(json_file::values_t& json) override
	{
		write_int(json, "max_bounce", m_ref.max_bounce());
		write_bool(json, "use_realistic_camera", m_ref.use_realistic_camera());
	}

	void deserialize(const json_file::values_t& json) override
	{
		int max_bounce;
		if(read_int(json, "max_bounce", max_bounce)){ m_ref.set_max_bounce(max_bounce); }

		bool use_realistic_camera;
		if(read_bool(json, "use_realistic_camera", use_realistic_camera)){ m_ref.set_use_realistic_camera(use_realistic_camera); }
	}

private:

	render::reference_renderer& m_ref;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace gui

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

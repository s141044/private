
#pragma once

#ifndef NN_RENDER_GUI_RENDERER_REALISTIC_CAMERA_HPP
#define NN_RENDER_GUI_RENDERER_REALISTIC_CAMERA_HPP

#include"../../renderer/realistic_camera.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace gui{

///////////////////////////////////////////////////////////////////////////////////////////////////
//realistic_camera
///////////////////////////////////////////////////////////////////////////////////////////////////

class realistic_camera : public property_editor
{
public:

	realistic_camera(render::realistic_camera& ref, string name = "realistic_camera") : property_editor(std::move(name)), m_ref(ref)
	{
	}

	void edit() override
	{
		char filename[512];
		memcpy(filename, m_ref.filename().c_str(), m_ref.filename().size() + 1);
		if(input_text("filename", filename, _countof(filename)))
			m_ref.set_filename(filename);

		auto blade_count = int(m_ref.blade_count());
		if(input_int("blade_count", blade_count)){ m_ref.set_blade_count(std::max(3, blade_count)); }

		float closure_ratio = m_ref.closure_ratio();
		if(slider_float("closure_ratio", closure_ratio, 1e-3f, 1)){ m_ref.set_closure_ratio(closure_ratio); }

		float sensitivity = m_ref.sensitivity();
		if(input_float("sensitivity", sensitivity)){ m_ref.set_sensitivity(std::max<float>(0, sensitivity)); }

		bool auto_exposure = m_ref.auto_exposure();
		if(checkbox("auto_exposure", auto_exposure)){ m_ref.set_auto_exposure(auto_exposure); }
	}
	
	void serialize(json_file::values_t& json) override
	{
		write_text(json, "filename", m_ref.filename());
		write_int(json, "blade_count", m_ref.blade_count());
		write_float(json, "closure_ratio", m_ref.closure_ratio());
		write_float(json, "sensitivity", m_ref.sensitivity());
		write_bool(json, "auto_exposure", m_ref.auto_exposure());
	}

	void deserialize(const json_file::values_t& json) override
	{
		string filename;
		if(read_text(json, "filename", filename)){ m_ref.set_filename(filename); }

		int blade_count;
		if(read_int(json, "blade_count", blade_count)){ m_ref.set_blade_count(blade_count); }

		float closure_ratio;
		if(read_float(json, "closure_ratio", closure_ratio)){ m_ref.set_closure_ratio(closure_ratio); }

		float sensitivity;
		if(read_float(json, "sensitivity", sensitivity)){ m_ref.set_sensitivity(sensitivity); }

		bool auto_exposure;
		if(read_bool(json, "auto_exposure", auto_exposure)){ m_ref.set_auto_exposure(auto_exposure); }
	}

private:

	render::realistic_camera& m_ref;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace gui

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

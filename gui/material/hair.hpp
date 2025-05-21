
#pragma once

#ifndef NN_RENDER_GUI_MATERIAL_HAIR_HPP
#define NN_RENDER_GUI_MATERIAL_HAIR_HPP

#include"../../material/hair.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace gui{

///////////////////////////////////////////////////////////////////////////////////////////////////
//hair_material
/*/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////*/

class hair_material : public property_editor
{
public:

	hair_material(render::hair_material& mtl, string name = "hair_material") : property_editor(std::move(name)), m_mtl(mtl)
	{
	}

	void edit() override
	{
		float3 I_R = m_mtl.I_R();
		if(color_edit("I_R", I_R)){ m_mtl.set_I_R(I_R); }

		float a_R = to_degree(m_mtl.a_R());
		if(slider_float("a_R", a_R, 0, 90)){ m_mtl.set_a_R(to_radian(a_R)); }

		float b_R = to_degree(m_mtl.b_R());
		if(slider_float("b_R", b_R, 0, 90)){ m_mtl.set_b_R(to_radian(b_R)); }

		separator();

		float3 I_TT = m_mtl.I_TT();
		if(color_edit("I_TT", I_TT)){ m_mtl.set_I_TT(I_TT); }

		float a_TT = to_degree(m_mtl.a_TT());
		if(slider_float("a_TT", a_TT, 0, 90)){ m_mtl.set_a_TT(to_radian(a_TT)); }

		float b_TT = to_degree(m_mtl.b_TT());
		if(slider_float("b_TT", b_TT, 0, 90)){ m_mtl.set_b_TT(to_radian(b_TT)); }

		float r_TT = to_degree(m_mtl.r_TT());
		if(slider_float("r_TT", r_TT, 0, 90)){ m_mtl.set_r_TT(to_radian(r_TT)); }

		separator();

		float3 I_TRT = m_mtl.I_TRT();
		if(color_edit("I_TRT", I_TRT)){ m_mtl.set_I_TRT(I_TRT); }

		float a_TRT = to_degree(m_mtl.a_TRT());
		if(slider_float("a_TRT", a_TRT, 0, 90)){ m_mtl.set_a_TRT(to_radian(a_TRT)); }

		float b_TRT = to_degree(m_mtl.b_TRT());
		if(slider_float("b_TRT", b_TRT, 0, 90)){ m_mtl.set_b_TRT(to_radian(b_TRT)); }

		separator();

		float3 I_g = m_mtl.I_g();
		if(color_edit("I_g", I_g)){ m_mtl.set_I_g(I_g); }

		float r_g = to_degree(m_mtl.r_g());
		if(slider_float("r_g", r_g, 0, 90)){ m_mtl.set_r_g(to_radian(r_g)); }

		float phi_g = to_degree(m_mtl.phi_g());
		if(slider_float("phi_g", phi_g, 0, 90)){ m_mtl.set_phi_g(to_radian(phi_g)); }
	}
	
	void serialize(json_file::values_t& json)
	{
		write_float3(json, "I_R", m_mtl.I_R());
		write_float(json, "a_R", m_mtl.a_R());
		write_float(json, "b_R", m_mtl.b_R());

		write_float3(json, "I_TT", m_mtl.I_TT());
		write_float(json, "a_TT", m_mtl.a_TT());
		write_float(json, "b_TT", m_mtl.b_TT());
		write_float(json, "r_TT", m_mtl.r_TT());

		write_float3(json, "I_TRT", m_mtl.I_TRT());
		write_float(json, "a_TRT", m_mtl.a_TRT());
		write_float(json, "b_TRT", m_mtl.b_TRT());

		write_float3(json, "I_g", m_mtl.I_g());
		write_float(json, "r_g", m_mtl.r_g());
		write_float(json, "phi_g", m_mtl.phi_g());
	}

	void deserialize(const json_file::values_t& json)
	{
		float3 I_R;
		if(read_float3(json, "I_R", I_R)){ m_mtl.set_I_R(I_R); }

		float a_R;
		if(read_float(json, "a_R", a_R)){ m_mtl.set_a_R(a_R); }

		float b_R;
		if(read_float(json, "b_R", b_R)){ m_mtl.set_b_R(b_R); }

		float3 I_TT;
		if(read_float3(json, "I_TT", I_TT)){ m_mtl.set_I_TT(I_TT); }

		float a_TT;
		if(read_float(json, "a_TT", a_TT)){ m_mtl.set_a_TT(a_TT); }

		float b_TT;
		if(read_float(json, "b_TT", b_TT)){ m_mtl.set_b_TT(b_TT); }

		float r_TT;
		if(read_float(json, "r_TT", r_TT)){ m_mtl.set_r_TT(r_TT); }

		float3 I_TRT;
		if(read_float3(json, "I_TRT", I_TRT)){ m_mtl.set_I_TRT(I_TRT); }

		float a_TRT;
		if(read_float(json, "a_TRT", a_TRT)){ m_mtl.set_a_TRT(a_TRT); }

		float b_TRT;
		if(read_float(json, "b_TRT", b_TRT)){ m_mtl.set_b_TRT(b_TRT); }

		float3 I_g;
		if(read_float3(json, "I_g", I_g)){ m_mtl.set_I_g(I_g); }

		float r_g;
		if(read_float(json, "r_g", r_g)){ m_mtl.set_r_g(r_g); }

		float phi_g;
		if(read_float(json, "phi_g", phi_g)){ m_mtl.set_phi_g(phi_g); }
	}

private:

	render::hair_material& m_mtl;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace gui

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

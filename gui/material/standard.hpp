
#pragma once

#ifndef NN_RENDER_GUI_MATERIAL_STANDARD_HPP
#define NN_RENDER_GUI_MATERIAL_STANDARD_HPP

#include"../../material/standard.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace gui{

///////////////////////////////////////////////////////////////////////////////////////////////////
//standard_material
/*/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////*/

class standard_material : public property_editor
{
public:

	standard_material(render::standard_material& mtl, string name = "standard_material") : property_editor(std::move(name)), m_mtl(mtl)
	{
	}

	void edit() override
	{
		float3 diffuse_color = m_mtl.diffuse_color();
		if(color_edit("diffuse_color", diffuse_color)){ m_mtl.set_diffuse_color(diffuse_color); }

		float diffuse_roughness = m_mtl.diffuse_roughness();
		if(slider_float("diffuse_roughness", diffuse_roughness, 0, 1)){ m_mtl.set_diffuse_roughness(diffuse_roughness); }

		float specular_scale = m_mtl.specular_scale();
		if(slider_float("specular_scale", specular_scale, 0, 1)){ m_mtl.set_specular_scale(specular_scale); }

		float3 specular_color0 = m_mtl.specular_color0();
		if(color_edit("specular_color0", specular_color0)){ m_mtl.set_specular_color0(specular_color0); }

		float specular_roughness = m_mtl.specular_roughness();
		if(slider_float("specular_roughness", specular_roughness, 0, 1)){ m_mtl.set_specular_roughness(specular_roughness); }

		float coat_scale = m_mtl.coat_scale();
		if(slider_float("coat_scale", coat_scale, 0, 1)){ m_mtl.set_coat_scale(coat_scale); }

		float3 coat_color0 = m_mtl.coat_color0();
		if(color_edit("coat_color0", coat_color0)){ m_mtl.set_coat_color0(coat_color0); }

		float coat_roughness = m_mtl.coat_roughness();
		if(slider_float("coat_roughness", coat_roughness, 0, 1)){ m_mtl.set_coat_roughness(coat_roughness); }

		float3 sheen_color = m_mtl.sheen_color();
		if(color_edit("sheen_color", sheen_color)){ m_mtl.set_sheen_color(sheen_color); }

		float sheen_roughness = m_mtl.sheen_roughness();
		if(slider_float("sheen_roughness", sheen_roughness, 0, 1)){ m_mtl.set_sheen_roughness(sheen_roughness); }

		float emission_scale = m_mtl.emission_scale();
		if(input_float("emission_scale", emission_scale)){ m_mtl.set_emission_scale(emission_scale); }

		float3 emission_color = m_mtl.emission_color();
		if(color_edit("emission_color", emission_color)){ m_mtl.set_emission_color(emission_color); }

		float subsurface_radius_scale = m_mtl.subsurface_radius_scale();
		if(input_float("subsurface_radius_scale", subsurface_radius_scale)){ m_mtl.set_subsurface_radius_scale(subsurface_radius_scale); }

		float3 subsurface_radius = m_mtl.subsurface_radius();
		if(color_edit("subsurface_radius", subsurface_radius)){ m_mtl.set_subsurface_radius(subsurface_radius); }
	}

private:

	render::standard_material& m_mtl;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace gui

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

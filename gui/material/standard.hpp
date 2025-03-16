
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

		separator();

		float specular_scale = m_mtl.specular_scale();
		if(slider_float("specular_scale", specular_scale, 0, 1)){ m_mtl.set_specular_scale(specular_scale); }

		float3 specular_color0 = m_mtl.specular_color0();
		if(color_edit("specular_color0", specular_color0)){ m_mtl.set_specular_color0(specular_color0); }

		float specular_roughness = m_mtl.specular_roughness();
		if(slider_float("specular_roughness", specular_roughness, 0, 1)){ m_mtl.set_specular_roughness(specular_roughness); }

		separator();

		float coat_scale = m_mtl.coat_scale();
		if(slider_float("coat_scale", coat_scale, 0, 1)){ m_mtl.set_coat_scale(coat_scale); }

		float3 coat_color0 = m_mtl.coat_color0();
		if(color_edit("coat_color0", coat_color0)){ m_mtl.set_coat_color0(coat_color0); }

		float coat_roughness = m_mtl.coat_roughness();
		if(slider_float("coat_roughness", coat_roughness, 0, 1)){ m_mtl.set_coat_roughness(coat_roughness); }

		separator();

		float3 sheen_color = m_mtl.sheen_color();
		if(color_edit("sheen_color", sheen_color)){ m_mtl.set_sheen_color(sheen_color); }

		float sheen_roughness = m_mtl.sheen_roughness();
		if(slider_float("sheen_roughness", sheen_roughness, 0, 1)){ m_mtl.set_sheen_roughness(sheen_roughness); }

		separator();

		float emissive_scale = m_mtl.emissive_scale();
		if(input_float("emissive_scale", emissive_scale)){ m_mtl.set_emissive_scale(emissive_scale); }

		float3 emissive_color = m_mtl.emissive_color();
		if(color_edit("emissive_color", emissive_color)){ m_mtl.set_emissive_color(emissive_color); }

		separator();

		float subsurface_radius_scale = m_mtl.subsurface_radius_scale();
		if(input_float("subsurface_radius_scale", subsurface_radius_scale)){ m_mtl.set_subsurface_radius_scale(subsurface_radius_scale); }

		float3 subsurface_radius = m_mtl.subsurface_radius();
		if(color_edit("subsurface_radius", subsurface_radius)){ m_mtl.set_subsurface_radius(subsurface_radius); }
	}
	
	void serialize(json_file::values_t& json)
	{
		write_float3(json, "diffuse_color", m_mtl.diffuse_color());
		write_float(json, "diffuse_roughness", m_mtl.diffuse_roughness());
		
		write_float(json, "specular_scale", m_mtl.specular_scale());
		write_float3(json, "specular_color0", m_mtl.specular_color0());
		write_float(json, "specular_roughness", m_mtl.specular_roughness());

		write_float(json, "coat_scale", m_mtl.coat_scale());
		write_float3(json, "coat_color0", m_mtl.coat_color0());
		write_float(json, "coat_roughness", m_mtl.coat_roughness());

		write_float3(json, "sheen_color", m_mtl.sheen_color());
		write_float(json, "sheen_roughness", m_mtl.sheen_roughness());
		
		write_float(json, "emissive_scale", m_mtl.emissive_scale());
		write_float3(json, "emissive_color", m_mtl.emissive_color());
		
		write_float(json, "subsurface_radius_scale", m_mtl.subsurface_radius_scale());
		write_float3(json, "subsurface_radius", m_mtl.subsurface_radius());
	}

	void deserialize(const json_file::values_t& json)
	{
		float3 diffuse_color;
		if(read_float3(json, "diffuse_color", diffuse_color)){ m_mtl.set_diffuse_color(diffuse_color); }
		
		float diffuse_roughness;
		if(read_float(json, "diffuse_roughness", diffuse_roughness)){ m_mtl.set_diffuse_roughness(diffuse_roughness); }
		
		float specular_scale;
		if(read_float(json, "specular_scale", specular_scale)){ m_mtl.set_specular_scale(specular_scale); }

		float3 specular_color0;
		if(read_float3(json, "specular_color0", specular_color0)){ m_mtl.set_specular_color0(specular_color0); }
		
		float specular_roughness;
		if(read_float(json, "specular_roughness", specular_roughness)){ m_mtl.set_specular_roughness(specular_roughness); }
		
		float coat_scale;
		if(read_float(json, "coat_scale", coat_scale)){ m_mtl.set_coat_scale(coat_scale); }

		float3 coat_color0;
		if(read_float3(json, "coat_color0", coat_color0)){ m_mtl.set_coat_color0(coat_color0); }
		
		float coat_roughness;
		if(read_float(json, "coat_roughness", coat_roughness)){ m_mtl.set_coat_roughness(coat_roughness); }

		float3 sheen_color;
		if(read_float3(json, "sheen_color", sheen_color)){ m_mtl.set_sheen_color(sheen_color); }
		
		float sheen_roughness;
		if(read_float(json, "sheen_roughness", sheen_roughness)){ m_mtl.set_sheen_roughness(sheen_roughness); }
		
		float emissive_scale;
		if(read_float(json, "emissive_scale", emissive_scale)){ m_mtl.set_emissive_scale(emissive_scale); }

		float3 emissive_color;
		if(read_float3(json, "emissive_color", emissive_color)){ m_mtl.set_emissive_color(emissive_color); }
		
		float subsurface_radius_scale;
		if(read_float(json, "subsurface_radius_scale", subsurface_radius_scale)){ m_mtl.set_subsurface_radius_scale(subsurface_radius_scale); }

		float3 subsurface_radius;
		if(read_float3(json, "subsurface_radius", subsurface_radius)){ m_mtl.set_subsurface_radius(subsurface_radius); }
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

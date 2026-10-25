
#pragma once

#ifndef NN_RENDER_GUI_MATERIAL_EYEBALL_HPP
#define NN_RENDER_GUI_MATERIAL_EYEBALL_HPP

#include"../../material/eyeball.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace gui{

///////////////////////////////////////////////////////////////////////////////////////////////////
//eyeball_material
/*/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////*/

class eyeball_material : public property_editor
{
public:

	eyeball_material(render::eyeball_material& mtl, string name = "eyeball_material") : property_editor(std::move(name)), m_mtl(mtl)
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

		float subsurface = m_mtl.subsurface();
		if(slider_float("subsurface", subsurface, 0, 1)){ m_mtl.set_subsurface(subsurface); }

		float subsurface_radius_scale = m_mtl.subsurface_radius_scale() * 1000;
		if(input_float("subsurface_radius_scale", subsurface_radius_scale)){ m_mtl.set_subsurface_radius_scale(std::max<float>(subsurface_radius_scale / 1000, 0)); }

		float3 subsurface_radius = m_mtl.subsurface_radius();
		if(color_edit("subsurface_radius", subsurface_radius)){ m_mtl.set_subsurface_radius(subsurface_radius); }

		separator();

		float height_scale = m_mtl.height_scale() * 1000;
		if(input_float("height_scale", height_scale)){ m_mtl.set_height_scale(std::max<float>(height_scale / 1000, 0)); }

		separator();

		float2 uv_offset = m_mtl.uv_offset();
		if(slider_float("uv_offset_x", uv_offset.x, -1, 1)){ m_mtl.set_uv_offset(uv_offset); }
		if(slider_float("uv_offset_y", uv_offset.y, -1, 1)){ m_mtl.set_uv_offset(uv_offset); }
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

		write_float(json, "subsurface", m_mtl.subsurface());
		write_float(json, "subsurface_radius_scale", m_mtl.subsurface_radius_scale());
		write_float3(json, "subsurface_radius", m_mtl.subsurface_radius());

		write_float(json, "height_scale", m_mtl.height_scale());

		write_float2(json, "uv_offset", m_mtl.uv_offset());
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

		float subsurface;
		if(read_float(json, "subsurface", subsurface)){ m_mtl.set_subsurface(subsurface); }
		
		float subsurface_radius_scale;
		if(read_float(json, "subsurface_radius_scale", subsurface_radius_scale)){ m_mtl.set_subsurface_radius_scale(subsurface_radius_scale); }

		float3 subsurface_radius;
		if(read_float3(json, "subsurface_radius", subsurface_radius)){ m_mtl.set_subsurface_radius(subsurface_radius); }

		float height_scale;
		if(read_float(json, "height_scale", height_scale)){ m_mtl.set_height_scale(height_scale); }

		float2 uv_offset;
		if(read_float2(json, "uv_offset", uv_offset)){ m_mtl.set_uv_offset(uv_offset); }
	}

private:

	render::eyeball_material& m_mtl;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace gui

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

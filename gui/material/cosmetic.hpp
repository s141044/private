
#pragma once

#ifndef NN_RENDER_GUI_MATERIAL_COSMETIC_HPP
#define NN_RENDER_GUI_MATERIAL_COSMETIC_HPP

#include"../../material/cosmetic.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace gui{

///////////////////////////////////////////////////////////////////////////////////////////////////
//cosmetic_material
/*/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////*/

class cosmetic_material : public property_editor
{
public:

	cosmetic_material(render::cosmetic_material& mtl, string name = "cosmetic_material") : property_editor(std::move(name)), m_mtl(mtl)
	{
	}

	void edit() override
	{
		float3 base_diffuse_color = m_mtl.base_diffuse_color();
		if(color_edit("base_diffuse_color", base_diffuse_color)){ m_mtl.set_base_diffuse_color(base_diffuse_color); }

		separator();

		float base_specular_scale = m_mtl.base_specular_scale();
		if(slider_float("base_specular_scale", base_specular_scale, 0, 1)){ m_mtl.set_base_specular_scale(base_specular_scale); }

		float base_specular_reflectance = m_mtl.base_specular_reflectance();
		if(slider_float("base_specular_reflectance", base_specular_reflectance, 0, 1)){ m_mtl.set_base_specular_reflectance(base_specular_reflectance); }

		float base_specular_roughness = m_mtl.base_specular_roughness();
		if(slider_float("base_specular_roughness", base_specular_roughness, 0, 1)){ m_mtl.set_base_specular_roughness(base_specular_roughness); }

		separator();

		float coat_specular_scale = m_mtl.coat_specular_scale();
		if(slider_float("coat_specular_scale", coat_specular_scale, 0, 1)){ m_mtl.set_coat_specular_scale(coat_specular_scale); }

		float coat_specular_reflectance = m_mtl.coat_specular_reflectance();
		if(slider_float("coat_specular_reflectance", coat_specular_reflectance, 0, 1)){ m_mtl.set_coat_specular_reflectance(coat_specular_reflectance); }

		float coat_specular_roughness = m_mtl.coat_specular_roughness();
		if(slider_float("coat_specular_roughness", coat_specular_roughness, 0, 1)){ m_mtl.set_coat_specular_roughness(coat_specular_roughness); }

		separator();

		float subsurface = m_mtl.subsurface();
		if(slider_float("subsurface", subsurface, 0, 1)){ m_mtl.set_subsurface(subsurface); }

		float subsurface_radius_scale = m_mtl.subsurface_radius_scale() * 1000;
		if(input_float("subsurface_radius_scale", subsurface_radius_scale)){ m_mtl.set_subsurface_radius_scale(std::max<float>(subsurface_radius_scale / 1000, 0)); }

		float3 subsurface_radius = m_mtl.subsurface_radius();
		if(color_edit("subsurface_radius", subsurface_radius)){ m_mtl.set_subsurface_radius(subsurface_radius); }

		separator();

		float thickness = m_mtl.thickness();
		if(input_float("thickness", thickness)){ m_mtl.set_thickness(thickness); }

		float thickness_scale = m_mtl.thickness_scale();
		if(input_float("thickness_scale", thickness_scale)){ m_mtl.set_thickness_scale(thickness_scale); }

		float coat_normal_map_strength = m_mtl.coat_normal_map_strength();
		if(slider_float("coat_normal_map_strength", coat_normal_map_strength, 0, 1)){ m_mtl.set_coat_normal_map_strength(coat_normal_map_strength); }

		float diffuser_ratio = m_mtl.diffuser_ratio();
		if(slider_float("diffuser_ratio", diffuser_ratio, 0, 1)){ m_mtl.set_diffuser_ratio(diffuser_ratio); }

		separator();
		
		float3 diffuser_albedo = m_mtl.diffuser_albedo();
		if(color_edit("diffuser_albedo", diffuser_albedo)){ m_mtl.set_diffuser_albedo(diffuser_albedo); }
		
		float diffuser_anisotropy1 = m_mtl.diffuser_anisotropy1();
		if(slider_float("diffuser_anisotropy1", diffuser_anisotropy1, -1, 1)){ m_mtl.set_diffuser_anisotropy1(diffuser_anisotropy1); }

		float diffuser_anisotropy2 = m_mtl.diffuser_anisotropy2();
		if(slider_float("diffuser_anisotropy2", diffuser_anisotropy2, -1, 1)){ m_mtl.set_diffuser_anisotropy2(diffuser_anisotropy2); }

		float diffuser_lobe_weight = m_mtl.diffuser_lobe_weight();
		if(slider_float("diffuser_lobe_weight", diffuser_lobe_weight, 0, 1)){ m_mtl.set_diffuser_lobe_weight(diffuser_lobe_weight); }

		separator();
		
		float3 platelet_albedo = m_mtl.platelet_albedo();
		if(color_edit("platelet_albedo", platelet_albedo)){ m_mtl.set_platelet_albedo(platelet_albedo); }

		float platelet_roughness = m_mtl.platelet_roughness();
		if(slider_float("platelet_roughness", platelet_roughness, 0, 1)){ m_mtl.set_platelet_roughness(platelet_roughness); }
	}
	
	void serialize(json_file::values_t& json)
	{
		write_float3(json, "base_diffuse_color", m_mtl.base_diffuse_color());
		
		write_float(json, "base_specular_scale", m_mtl.base_specular_scale());
		write_float(json, "base_specular_reflectance", m_mtl.base_specular_reflectance());
		write_float(json, "base_specular_roughness", m_mtl.base_specular_roughness());
		
		write_float(json, "coat_specular_scale", m_mtl.coat_specular_scale());
		write_float(json, "coat_specular_reflectance", m_mtl.coat_specular_reflectance());
		write_float(json, "coat_specular_roughness", m_mtl.coat_specular_roughness());
		
		write_float(json, "subsurface", m_mtl.subsurface());
		write_float(json, "subsurface_radius_scale", m_mtl.subsurface_radius_scale());
		write_float3(json, "subsurface_radius", m_mtl.subsurface_radius());

		write_float(json, "thickness", m_mtl.thickness());
		write_float(json, "thickness_scale", m_mtl.thickness_scale());
		write_float(json, "coat_normal_map_strength", m_mtl.coat_normal_map_strength());
		write_float(json, "diffuser_ratio", m_mtl.diffuser_ratio());

		write_float3(json, "diffuser_albedo", m_mtl.diffuser_albedo());
		write_float(json, "diffuser_anisotropy1", m_mtl.diffuser_anisotropy1());
		write_float(json, "diffuser_anisotropy2", m_mtl.diffuser_anisotropy2());
		write_float(json, "diffuser_lobe_weight", m_mtl.diffuser_lobe_weight());
		
		write_float3(json, "platelet_albedo", m_mtl.platelet_albedo());
		write_float(json, "platelet_roughness", m_mtl.platelet_roughness());
	}

	void deserialize(const json_file::values_t& json)
	{
		float3 base_diffuse_color;
		if(read_float3(json, "base_diffuse_color", base_diffuse_color)){ m_mtl.set_base_diffuse_color(base_diffuse_color); }
		
		float base_specular_scale;
		if(read_float(json, "base_specular_scale", base_specular_scale)){ m_mtl.set_base_specular_scale(base_specular_scale); }

		float base_specular_reflectance;
		if(read_float(json, "base_specular_reflectance", base_specular_reflectance)){ m_mtl.set_base_specular_reflectance(base_specular_reflectance); }
		
		float base_specular_roughness;
		if(read_float(json, "base_specular_roughness", base_specular_roughness)){ m_mtl.set_base_specular_roughness(base_specular_roughness); }
		
		float coat_specular_scale;
		if(read_float(json, "coat_specular_scale", coat_specular_scale)){ m_mtl.set_coat_specular_scale(coat_specular_scale); }

		float coat_specular_reflectance;
		if(read_float(json, "coat_specular_reflectance", coat_specular_reflectance)){ m_mtl.set_coat_specular_reflectance(coat_specular_reflectance); }
		
		float coat_specular_roughness;
		if(read_float(json, "coat_specular_roughness", coat_specular_roughness)){ m_mtl.set_coat_specular_roughness(coat_specular_roughness); }

		float subsurface;
		if(read_float(json, "subsurface", subsurface)){ m_mtl.set_subsurface(subsurface); }
		
		float subsurface_radius_scale;
		if(read_float(json, "subsurface_radius_scale", subsurface_radius_scale)){ m_mtl.set_subsurface_radius_scale(subsurface_radius_scale); }

		float3 subsurface_radius;
		if(read_float3(json, "subsurface_radius", subsurface_radius)){ m_mtl.set_subsurface_radius(subsurface_radius); }
		
		float thickness;
		if(read_float(json, "thickness", thickness)){ m_mtl.set_thickness(thickness); }
		
		float thickness_scale;
		if(read_float(json, "thickness_scale", thickness_scale)){ m_mtl.set_thickness_scale(thickness_scale); }
		
		float coat_normal_map_strength;
		if(read_float(json, "coat_normal_map_strength", coat_normal_map_strength)){ m_mtl.set_coat_normal_map_strength(coat_normal_map_strength); }
		
		float diffuser_ratio;
		if(read_float(json, "diffuser_ratio", diffuser_ratio)){ m_mtl.set_diffuser_ratio(diffuser_ratio); }
		
		float3 diffuser_albedo;
		if(read_float3(json, "diffuser_albedo", diffuser_albedo)){ m_mtl.set_diffuser_albedo(diffuser_albedo); }
		
		float diffuser_anisotropy1;
		if(read_float(json, "diffuser_anisotropy1", diffuser_anisotropy1)){ m_mtl.set_diffuser_anisotropy1(diffuser_anisotropy1); }
		
		float diffuser_anisotropy2;
		if(read_float(json, "diffuser_anisotropy2", diffuser_anisotropy2)){ m_mtl.set_diffuser_anisotropy2(diffuser_anisotropy2); }
		
		float diffuser_lobe_weight;
		if(read_float(json, "diffuser_lobe_weight", diffuser_lobe_weight)){ m_mtl.set_diffuser_lobe_weight(diffuser_lobe_weight); }

		float3 platelet_albedo;
		if(read_float3(json, "platelet_albedo", platelet_albedo)){ m_mtl.set_platelet_albedo(platelet_albedo); }
		
		float platelet_roughness;
		if(read_float(json, "platelet_roughness", platelet_roughness)){ m_mtl.set_platelet_roughness(platelet_roughness); }
	}

private:

	render::cosmetic_material& m_mtl;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace gui

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif


#pragma once

#ifndef NN_RENDER_GUI_LIGHT_DIRECTION_HPP
#define NN_RENDER_GUI_LIGHT_DIRECTION_HPP

#include"../../light/direction.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace gui{

///////////////////////////////////////////////////////////////////////////////////////////////////
//directional_light
///////////////////////////////////////////////////////////////////////////////////////////////////

class directional_light : public property_editor
{
public:

	directional_light(render::directional_light& light, string name = "directional_light") : property_editor(std::move(name)), m_light(light)
	{
	}

	void edit() override
	{
		bool dir_changed = false;
		if(input_float("phi", m_phi)){ dir_changed = true; }
		if(input_float("theta", m_theta)){ dir_changed = true; }
		if(dir_changed){ m_light.set_direction(direction()); }

		bool power_changed = false;
		if(color_edit("color", m_color)){ power_changed = true; }
		if(input_float("power", m_power)){ power_changed = true; }
		if(power_changed){ m_light.set_power(m_color * m_power); }

		auto source_angle = m_light.source_angle();
		if(input_float("source_angle", source_angle)){ m_light.set_source_angle(std::max<float>(0, source_angle)); }
	}
	
	void serialize(json_file::values_t& json) override
	{
		write_float(json, "phi", m_phi);
		write_float(json, "theta", m_theta);
		write_float(json, "power", m_power);
		write_float3(json, "color", m_color);
		write_float(json, "source_angle", m_light.source_angle());
	}

	void deserialize(const json_file::values_t& json) override
	{
		read_float(json, "phi", m_phi);
		read_float(json, "theta", m_theta);
		m_light.set_direction(direction());

		read_float(json, "power", m_power);
		read_float3(json, "color", m_color);
		m_light.set_power(m_color * m_power);
		
		float source_angle;
		if(read_float(json, "source_angle", source_angle)){ m_light.set_source_angle(source_angle); }
	}

private:

	float3 direction() const
	{
		const float ph = to_radian(m_phi);
		const float th = to_radian(m_theta);
		const float ct = cos(th);
		const float st = sin(th);
		const float cp = cos(ph);
		const float sp = sin(ph);
		return float3(st * cp, ct, st * sp);
	}

private:

	float						m_phi = 0;
	float						m_theta = 0;
	float						m_power = 1;
	float3						m_color = float3(1);
	render::directional_light&	m_light;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace gui

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

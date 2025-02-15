
#pragma once

#ifndef NN_RENDER_DEV_GRID_HPP
#define NN_RENDER_DEV_GRID_HPP

#include"../core.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace dev{

///////////////////////////////////////////////////////////////////////////////////////////////////
//grid
///////////////////////////////////////////////////////////////////////////////////////////////////

class grid
{
public:

	//コンストラクタ
	grid()
	{
		m_shader_file = gp_shader_manager->create(L"dev/grid.sdf.json");
	}

	//描画
	bool draw(render_context& context, target_state& target_state)
	{
		if(m_shader_file.has_update())
			m_shader_file.update();
		else if(m_shader_file.is_invalid())
			return false;

		if(m_update)
		{
			struct cbuffer
			{
				float3	color;
				float	pad0;
				float3	grid_min;
				float	pad1;
				float3	grid_max;
				float	pad2;
				uint3	cell_count;
				float	cell_size;
			};

			mp_cbuffer = gp_render_device->create_constant_buffer(sizeof(cbuffer));
			auto* p_cbuffer = mp_cbuffer->data<cbuffer>();
			p_cbuffer->color = m_color;
			p_cbuffer->cell_size = m_cell_size;
			p_cbuffer->cell_count = m_cell_count;

			float3 grid_size = m_cell_size * float3(m_cell_count);
			p_cbuffer->grid_min = m_grid_center - grid_size / 2;
			p_cbuffer->grid_max = m_grid_center + grid_size / 2;
		}

		context.set_target_state(target_state);
		context.set_pipeline_state(*m_shader_file.get("draw_grid"));
		context.set_pipeline_resource("grid_cbuffer", *mp_cbuffer);
		context.draw_with_32bit_constant(2 * (m_cell_count.y + 1), m_cell_count.z + 1, 0, 0); //x
		context.draw_with_32bit_constant(2 * (m_cell_count.z + 1), m_cell_count.x + 1, 0, 1); //y
		context.draw_with_32bit_constant(2 * (m_cell_count.x + 1), m_cell_count.y + 1, 0, 2); //z
		return true;
	}

	//パラメータ
	const float3& color() const
	{
		return m_color;
	}
	void set_color(const float3& color)
	{
		if(m_color != color)
		{
			m_color = color;
			m_update = true;
		}
	}
	const float3& center() const
	{
		return m_grid_center;
	}
	void set_center(const float3& center)
	{
		if(m_grid_center != center)
		{
			m_grid_center = center;
			m_update = true;
		}
	}
	void set_cell_size(const float size)
	{
		if(m_cell_size != size)
		{
			m_cell_size = size;
			m_update = true;
		}
	}
	float cell_size(const float size) const
	{
		return m_cell_size;
	}
	const uint3& cell_count() const
	{
		return m_cell_count;
	}
	void set_cell_count(const uint3& count)
	{
		if(m_cell_count != count)
		{
			m_cell_count = count;
			m_update = true;
		}
	}

private:

	shader_file_holder	m_shader_file;
	constant_buffer_ptr	mp_cbuffer;
	float3				m_color			= float3(0.5f);
	float3				m_grid_center	= float3(0.0f);
	float				m_cell_size		= 1.0f;
	uint3				m_cell_count	= uint3(100, 0, 100);
	bool				m_update		= true;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace dev

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

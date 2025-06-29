
#pragma once

#ifndef NN_RENDER_UTILITY_HIGHLIGHT_HPP
#define NN_RENDER_UTILITY_HIGHLIGHT_HPP

#include"../base.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//highlight_renderer
///////////////////////////////////////////////////////////////////////////////////////////////////

class highlight_renderer
{
public:

	//•`‰æ
	void draw(render_context &context, target_state& target, render_entity* entity_ptr, uint material_index)
	{
		if(entity_ptr == nullptr)
		{
			m_current_entity_ptr = nullptr;
			m_current_material_index = -1;
			return;
		}

		if(m_current_entity_ptr != entity_ptr || m_current_material_index != material_index)
		{
			m_current_entity_ptr = entity_ptr;
			m_current_material_index = material_index;
			m_start = std::chrono::high_resolution_clock::now();
		}

		auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - m_start).count();
		if(elapsed_ms > 500)
			return;

		float4 color(1, 1, 0, 0);
		color *= 1 - pow(elapsed_ms / 500.0f, 2.0f);

		context.set_target_state(target);
		entity_ptr->draw_constant(context, color, material_index);
	}

private:

	render_entity*									m_current_entity_ptr = nullptr;
	uint											m_current_material_index = -1;
	std::chrono::high_resolution_clock::time_point	m_start;

};

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

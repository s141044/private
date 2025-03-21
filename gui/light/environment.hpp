
#pragma once

#ifndef NN_RENDER_GUI_LIGHT_ENVIRONMENT_HPP
#define NN_RENDER_GUI_LIGHT_ENVIRONMENT_HPP

#include"../../light/environment.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace gui{

///////////////////////////////////////////////////////////////////////////////////////////////////
//environment_light
///////////////////////////////////////////////////////////////////////////////////////////////////

class environment_light : public property_editor
{
public:

	environment_light(render::environment_light& env, string name = "environment_light") : property_editor(std::move(name)), m_env(env)
	{
	}

	void edit() override
	{
		char filename[512];
		if(auto& p_tex = m_env.panorama_tex_ptr(); p_tex != nullptr)
			memcpy(filename, p_tex->filename().c_str(), p_tex->filename().size() + 1);
		else
			filename[0] = '\0';
		
		if(input_text("panorama_hdr", filename, _countof(filename)))
			try{ m_env.set_panorama_tex_ptr(gp_texture_manager->create(string(filename), texture_type_hdr)); } catch(const std::exception& e){ std::cout << e.what() << std::endl; }

		auto cube_res = int(m_env.cube_res());
		if(input_int("cube_res", cube_res)){ m_env.set_cube_res(std::max(1, cube_res)); }
	}
	
	void serialize(json_file::values_t& json) override
	{
		write_text(json, "panorama_hdr", (m_env.panorama_tex_ptr() != nullptr) ? m_env.panorama_tex_ptr()->filename() : string());
		write_int(json, "cube_res", m_env.cube_res());
	}

	void deserialize(const json_file::values_t& json) override
	{
		string filename;
		if(read_text(json, "panorama_hdr", filename) && filename.size())
			try{ m_env.set_panorama_tex_ptr(gp_texture_manager->create(filename, texture_type_hdr)); } catch(const std::exception& e){ std::cout << e.what() << std::endl; }
		
		int cube_res;
		if(read_int(json, "cube_res", cube_res)){ m_env.set_cube_res(cube_res); }
	}

private:

	render::environment_light& m_env;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace gui

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif


#pragma once

#ifndef NN_RENDER_MATERIAL_HAIR_HPP
#define NN_RENDER_MATERIAL_HAIR_HPP

#include"../base.hpp"
#include"..//utility/convert.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//hair_material
/*/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////*/

class hair_material : public material
{
public:
	
	struct bindless_material : bindless_material_base
	{
		bindless_material(texture_resource_ptr& p_alpha_map) : bindless_material_base(material_type_hair, p_alpha_map ? p_alpha_map->srv().bindless_handle() : -1, -1, 0)
		{
		}
	};

	//コンストラクタ
	hair_material() : material(material_type_hair)
	{
		m_has_update = true;
	}

	//バインドレスの管理
	void register_bindless()
	{
		mp_buf = gp_render_device->create_byteaddress_buffer(sizeof(bindless_material), resource_flag_allow_shader_resource);
		mp_srv = gp_render_device->create_shader_resource_view(*mp_buf, buffer_srv_desc(*mp_buf));
		gp_render_device->register_bindless(*mp_srv);
	}

	//パラメータを更新
	void update(render_context& context) override
	{
		bindless_material params(mp_alpha_map);

		push_priority push_priority(context);
		context.set_priority(priority_initiaize);
		void* dst = context.update_buffer(*mp_buf, 0, sizeof(params));
		memcpy(dst, &params, sizeof(params));
		m_has_update = false;
	}

	//アルファマップがあるか
	bool has_alpha_map() const override { return (mp_alpha_map != nullptr); }

	//パラメータ

private:

	template<class T, class U> void set_impl(T& dst, U& src)
	{
		if(dst != src)
		{
			dst = std::move(src);
			m_has_update = true;
		}
	}

private:

	texture_resource_ptr	mp_alpha_map;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

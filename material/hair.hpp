
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
エネルギー保存条件: I_R + I_TT + I_TRT * (1 + I_g) <= 1
/////////////////////////////////////////////////////////////////////////////////////////////////*/

class hair_material : public material
{
public:
	
	struct bindless_material : bindless_material_base
	{
		bindless_material(texture_resource_ptr& p_alpha_map) : bindless_material_base(material_type_hair, p_alpha_map ? p_alpha_map->srv().bindless_handle() : -1, -1, 0)
		{
		}
		uint	Ia_R;
		uint	Ia_TT;
		uint	Ia_TRT;
		uint	Iphi_g;
		uint	b;
		uint	r;
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
		params.Ia_R = f32x4_to_u8x4_unorm(float4(m_I_R, m_a_R * inv_2PI()));
		params.Ia_TT = f32x4_to_u8x4_unorm(float4(m_I_TT, m_a_TT * inv_2PI()));
		params.Ia_TRT = f32x4_to_u8x4_unorm(float4(m_I_TRT, m_a_TRT * inv_2PI()));
		params.Iphi_g = f32x4_to_u8x4_unorm(float4(m_I_g, m_phi_g * inv_2PI()));
		params.b = f32x4_to_u8x4_unorm(float4(m_b_R, m_b_TT, m_b_TRT, 0) * inv_2PI());
		params.r = f32x4_to_u8x4_unorm(float4(0, m_r_TT, 0, m_r_g) * inv_2PI());

		push_priority push_priority(context);
		context.set_priority(priority_initiaize);
		void* dst = context.update_buffer(*mp_buf, 0, sizeof(params));
		memcpy(dst, &params, sizeof(params));
		m_has_update = false;
	}

	//アルファマップがあるか
	bool has_alpha_map() const override { return (mp_alpha_map != nullptr); }

	//パラメータ
	void set_alpha_map(texture_resource_ptr t){ set_impl(mp_alpha_map, t); }
	void set_I_R(const float3& v){ set_impl(m_I_R, v); }
	void set_a_R(const float v){ set_impl(m_a_R, v); }
	void set_b_R(const float v){ set_impl(m_b_R, v); }
	void set_I_TT(const float3& v){ set_impl(m_I_TT, v); }
	void set_a_TT(const float v){ set_impl(m_a_TT, v); }
	void set_b_TT(const float v){ set_impl(m_b_TT, v); }
	void set_r_TT(const float v){ set_impl(m_r_TT, v); }
	void set_I_TRT(const float3& v){ set_impl(m_I_TRT, v); }
	void set_a_TRT(const float v){ set_impl(m_a_TRT, v); }
	void set_b_TRT(const float v){ set_impl(m_b_TRT, v); }
	void set_I_g(const float3& v){ set_impl(m_I_g, v); }
	void set_r_g(const float v){ set_impl(m_r_g, v); }
	void set_phi_g(const float v){ set_impl(m_phi_g, v); }
	float3	I_R() const { return m_I_R; }
	float	a_R() const { return m_a_R; }
	float	b_R() const { return m_b_R; }
	float3	I_TT() const { return m_I_TT; }
	float	a_TT() const { return m_a_TT; }
	float	b_TT() const { return m_b_TT; }
	float	r_TT() const { return m_r_TT; }
	float3	I_TRT() const { return m_I_TRT; }
	float	a_TRT() const { return m_a_TRT; }
	float	b_TRT() const { return m_b_TRT; }
	float3	I_g() const { return m_I_g; }
	float	r_g() const { return m_r_g; }
	float	phi_g() const { return m_phi_g; }

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
	float3					m_I_R = 0.25f;
	float					m_a_R = to_radian(0.0f);
	float					m_b_R = to_radian(6.0f);
	float3					m_I_TT = 0.25f;
	float					m_a_TT = to_radian(0.0f);
	float					m_b_TT = to_radian(6.0f);
	float					m_r_TT = to_radian(6.0f);
	float3					m_I_TRT = 0.25f;
	float					m_a_TRT = to_radian(0.0f);
	float					m_b_TRT = to_radian(30.0f);
	float3					m_I_g = 1.0f;
	float					m_r_g = to_radian(2.5f);
	float					m_phi_g = to_radian(20.0f);
};

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

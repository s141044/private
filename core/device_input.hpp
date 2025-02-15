
#pragma once

#ifndef NN_RENDER_CORE_DEVICE_INPUT_HPP
#define NN_RENDER_CORE_DEVICE_INPUT_HPP

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//device_input
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace iface{ class device_input
{
public:

	enum button
	{
		key_escape = 0,
		key_1,
		key_2,
		key_3,
		key_4,
		key_5,
		key_6,
		key_7,
		key_8,
		key_9,
		key_0,
		key_minus,
		key_equals,
		key_back,
		key_tab,
		key_q,
		key_w,
		key_e,
		key_r,
		key_t,
		key_y,
		key_u,
		key_i,
		key_o,
		key_p,
		key_lbracket,
		key_rbracket,
		key_return,
		key_lcontrol,
		key_a,
		key_s,
		key_d,
		key_f,
		key_g,
		key_h,
		key_j,
		key_k,
		key_l,
		key_semicolon,
		key_apostrophe,
		key_grave,
		key_lshift,
		key_backslash,
		key_z,
		key_x,
		key_c,
		key_v,
		key_b,
		key_n,
		key_m,
		key_comma,
		key_period,
		key_slash,
		key_rshift,
		key_multiply,
		key_lalt,
		key_space,
		key_capital,
		key_f1,
		key_f2,
		key_f3,
		key_f4,
		key_f5,
		key_f6,
		key_f7,
		key_f8,
		key_f9,
		key_f10,
		key_numlock,
		key_scroll,
		key_numpad7,
		key_numpad8,
		key_numpad9,
		key_subtract,
		key_numpad4,
		key_numpad5,
		key_numpad6,
		key_add,
		key_numpad1,
		key_numpad2,
		key_numpad3,
		key_numpad0,
		key_decimal,
		key_f11,
		key_f12,
		key_numpadenter,
		key_rcontrol,
		key_divide,
		key_sysrq,
		key_ralt,
		key_pause,
		key_home,
		key_up,
		key_pgup,
		key_left,
		key_right,
		key_end,
		key_down,
		key_pgdn,
		key_insert,
		key_delete,
		key_lwin,
		key_rwin,
		key_apps,
		num_keys,

		mouse0, //left
		mouse1, //right
		mouse2, //middle
		mouse3,
		mouse4,
		mouse5,
		mouse6,
		mouse7,
		num_inputs,
	};

	//デストラクタ
	virtual ~device_input(){}

	//初期化
	virtual void initialize() = 0;

	//終了処理
	virtual void finalize() = 0;

	//更新
	virtual void update(const float dt) = 0;

public:

	//ボタンの状態を返す
	bool pressed(const button button) const { return m_button_state[m_current_index][button]; }
	bool released(const button button) const { return not(m_button_state[m_current_index][button]); }
	bool first_pressed(const button button) const { return (m_button_state[m_current_index][button] && not(m_button_state[1 - m_current_index][button])); }
	bool first_released(const button button) const { return (not(m_button_state[m_current_index][button]) && m_button_state[1 - m_current_index][button]); }
	float press_duration(const button button) const { return m_press_duration[button]; }

	//マウス位置を返す
	int mouse_pos_x() const { return m_mouse_pos[0]; }
	int mouse_pos_y() const { return m_mouse_pos[1]; }

	//マウス移動量を返す
	int mouse_delta_x() const { return m_mouse_delta[0]; }
	int mouse_delta_y() const { return m_mouse_delta[1]; }
	int mouse_delta_z() const { return m_mouse_delta[2]; }

protected:

	int		m_mouse_pos[2] = {};
	int		m_mouse_delta[3] = {};
	bool	m_button_state[2][num_inputs] = {};
	float	m_press_duration[num_inputs] = {};
	size_t	m_current_index = 0;
};}

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

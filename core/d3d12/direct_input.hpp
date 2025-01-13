
#pragma once

#ifndef NN_RENDER_D3D12_DIRECT_INPUT_HPP
#define NN_RENDER_D3D12_DIRECT_INPUT_HPP

#define DIRECTINPUT_VERSION 0x0800
#include<dinput.h>
#include<wrl.h>

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace d3d12{

///////////////////////////////////////////////////////////////////////////////////////////////////
//前方宣言
///////////////////////////////////////////////////////////////////////////////////////////////////

using Microsoft::WRL::ComPtr;

///////////////////////////////////////////////////////////////////////////////////////////////////
//device_input
///////////////////////////////////////////////////////////////////////////////////////////////////

class device_input
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

	//初期化/終了
	void initialize(const HWND hwnd)
	{
		if(FAILED(DirectInput8Create(GetModuleHandle(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8, &mp_direct_input, nullptr)))
			throw std::runtime_error("DirectInput8 initialization failed.");
		if(FAILED(mp_direct_input->CreateDevice(GUID_SysKeyboard, &mp_keyboard, nullptr)))
			throw std::runtime_error("Keyboard CreateDevice failed.");
		if(FAILED(mp_keyboard->SetDataFormat(&c_dfDIKeyboard)))
			throw std::runtime_error("Keyboard SetDataFormat failed.");
		if(FAILED(mp_keyboard->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)))
			throw std::runtime_error("Keyboard SetCooperativeLevel failed.");

		DIPROPDWORD dipdw;
		dipdw.diph.dwSize = sizeof(DIPROPDWORD);
		dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
		dipdw.diph.dwObj = 0;
		dipdw.diph.dwHow = DIPH_DEVICE;
		dipdw.dwData = 10;
		if(FAILED(mp_keyboard->SetProperty(DIPROP_BUFFERSIZE, &dipdw.diph)))
			throw std::runtime_error("Keyboard set buffer size failed.");
		if(FAILED(mp_direct_input->CreateDevice(GUID_SysMouse, &mp_mouse, nullptr)))
			throw std::runtime_error("Mouse CreateDevice failed.");
		if(FAILED(mp_mouse->SetDataFormat(&c_dfDIMouse2)))
			throw std::runtime_error("Mouse SetDataFormat failed.");
		if(FAILED(mp_mouse->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)))
			throw std::runtime_error("Mouse SetCooperativeLevel failed.");

		m_idx = 0;
		memset(&m_press_duration, 0, sizeof(m_press_duration));
		memset(m_button_state[m_idx], 0, sizeof(m_button_state[m_idx]));

		{
			m_button_mapping[key_escape] = 1;
			m_button_mapping[key_1] = 2;
			m_button_mapping[key_2] = 3;
			m_button_mapping[key_3] = 4;
			m_button_mapping[key_4] = 5;
			m_button_mapping[key_5] = 6;
			m_button_mapping[key_6] = 7;
			m_button_mapping[key_7] = 8;
			m_button_mapping[key_8] = 9;
			m_button_mapping[key_9] = 10;
			m_button_mapping[key_0] = 11;
			m_button_mapping[key_minus] = 12;
			m_button_mapping[key_equals] = 13;
			m_button_mapping[key_back] = 14;
			m_button_mapping[key_tab] = 15;
			m_button_mapping[key_q] = 16;
			m_button_mapping[key_w] = 17;
			m_button_mapping[key_e] = 18;
			m_button_mapping[key_r] = 19;
			m_button_mapping[key_t] = 20;
			m_button_mapping[key_y] = 21;
			m_button_mapping[key_u] = 22;
			m_button_mapping[key_i] = 23;
			m_button_mapping[key_o] = 24;
			m_button_mapping[key_p] = 25;
			m_button_mapping[key_lbracket] = 26;
			m_button_mapping[key_rbracket] = 27;
			m_button_mapping[key_return] = 28;
			m_button_mapping[key_lcontrol] = 29;
			m_button_mapping[key_a] = 30;
			m_button_mapping[key_s] = 31;
			m_button_mapping[key_d] = 32;
			m_button_mapping[key_f] = 33;
			m_button_mapping[key_g] = 34;
			m_button_mapping[key_h] = 35;
			m_button_mapping[key_j] = 36;
			m_button_mapping[key_k] = 37;
			m_button_mapping[key_l] = 38;
			m_button_mapping[key_semicolon] = 39;
			m_button_mapping[key_apostrophe] = 40;
			m_button_mapping[key_grave] = 41;
			m_button_mapping[key_lshift] = 42;
			m_button_mapping[key_backslash] = 43;
			m_button_mapping[key_z] = 44;
			m_button_mapping[key_x] = 45;
			m_button_mapping[key_c] = 46;
			m_button_mapping[key_v] = 47;
			m_button_mapping[key_b] = 48;
			m_button_mapping[key_n] = 49;
			m_button_mapping[key_m] = 50;
			m_button_mapping[key_comma] = 51;
			m_button_mapping[key_period] = 52;
			m_button_mapping[key_slash] = 53;
			m_button_mapping[key_rshift] = 54;
			m_button_mapping[key_multiply] = 55;
			m_button_mapping[key_lalt] = 56;
			m_button_mapping[key_space] = 57;
			m_button_mapping[key_capital] = 58;
			m_button_mapping[key_f1] = 59;
			m_button_mapping[key_f2] = 60;
			m_button_mapping[key_f3] = 61;
			m_button_mapping[key_f4] = 62;
			m_button_mapping[key_f5] = 63;
			m_button_mapping[key_f6] = 64;
			m_button_mapping[key_f7] = 65;
			m_button_mapping[key_f8] = 66;
			m_button_mapping[key_f9] = 67;
			m_button_mapping[key_f10] = 68;
			m_button_mapping[key_numlock] = 69;
			m_button_mapping[key_scroll] = 70;
			m_button_mapping[key_numpad7] = 71;
			m_button_mapping[key_numpad8] = 72;
			m_button_mapping[key_numpad9] = 73;
			m_button_mapping[key_subtract] = 74;
			m_button_mapping[key_numpad4] = 75;
			m_button_mapping[key_numpad5] = 76;
			m_button_mapping[key_numpad6] = 77;
			m_button_mapping[key_add] = 78;
			m_button_mapping[key_numpad1] = 79;
			m_button_mapping[key_numpad2] = 80;
			m_button_mapping[key_numpad3] = 81;
			m_button_mapping[key_numpad0] = 82;
			m_button_mapping[key_decimal] = 83;
			m_button_mapping[key_f11] = 87;
			m_button_mapping[key_f12] = 88;
			m_button_mapping[key_numpadenter] = 156;
			m_button_mapping[key_rcontrol] = 157;
			m_button_mapping[key_divide] = 181;
			m_button_mapping[key_sysrq] = 183;
			m_button_mapping[key_ralt] = 184;
			m_button_mapping[key_pause] = 197;
			m_button_mapping[key_home] = 199;
			m_button_mapping[key_up] = 200;
			m_button_mapping[key_pgup] = 201;
			m_button_mapping[key_left] = 203;
			m_button_mapping[key_right] = 205;
			m_button_mapping[key_end] = 207;
			m_button_mapping[key_down] = 208;
			m_button_mapping[key_pgdn] = 209;
			m_button_mapping[key_insert] = 210;
			m_button_mapping[key_delete] = 211;
			m_button_mapping[key_lwin] = 219;
			m_button_mapping[key_rwin] = 220;
			m_button_mapping[key_apps] = 221;
		}
	}
	void finalize()
	{
		mp_mouse.Reset();
		mp_keyboard.Reset();
		mp_direct_input.Reset();
	}

	//更新
	void update(const HWND hwnd, const float delta_time)
	{
		const HWND foreground = GetForegroundWindow();
		const bool visible = IsWindowVisible(foreground) != 0;
		
		m_idx = 1 - m_idx;

		if((foreground != hwnd) || not(visible))
		{
			memset(&m_press_duration, 0, sizeof(m_press_duration));
			memset(m_button_state[m_idx], 0, sizeof(m_button_state[m_idx]));
		}
		else
		{
			mp_mouse->Acquire();
			mp_mouse->GetDeviceState(sizeof(DIMOUSESTATE2), &m_mouse_state);
			mp_keyboard->Acquire();
			mp_keyboard->GetDeviceState(sizeof(m_key_state), m_key_state);

			for(size_t i = 0; i < num_keys; i++)
			{
				m_button_state[m_idx][i] = ((m_key_state[m_button_mapping[i]] & 0x80) != 0);
			}
			for(size_t i = 0; i < 8; i++)
			{
				m_button_state[m_idx][mouse0 + i] = (m_mouse_state.rgbButtons[i] > 0);
			}
			for(size_t i = 0; i < num_inputs; i++)
			{
				m_press_duration[i] = (m_button_state[m_idx][i] && m_button_state[1 - m_idx][0]) ? delta_time : 0;
			}
		}

		POINT p;
		GetCursorPos(&p);
		ScreenToClient(hwnd, &p);
		m_cursor_pos_x = p.x;
		m_cursor_pos_y = p.y;
	}

	//ボタンの状態を返す
	bool pressed(const button button) const { return m_button_state[m_idx][button]; }
	bool released(const button button) const { return not(m_button_state[m_idx][button]); }
	bool first_pressed(const button button) const { return (m_button_state[m_idx][button] && not(m_button_state[1 - m_idx][button])); }
	bool first_released(const button button) const { return (not(m_button_state[m_idx][button]) && m_button_state[1 - m_idx][button]); }
	float press_duration(const button button) const { return m_press_duration[button]; }

	//マウス移動量を返す
	int mouse_delta_x() const { return m_mouse_state.lX; }
	int mouse_delta_y() const { return m_mouse_state.lY; }
	int mouse_delta_z() const { return m_mouse_state.lZ; }

	//カーソル位置を返す
	int cursor_pos_x() const { return m_cursor_pos_x; }
	int cursor_pos_y() const { return m_cursor_pos_y; }

private:

	ComPtr<IDirectInput8A>			mp_direct_input;
	ComPtr<IDirectInputDevice8A>	mp_mouse;
	ComPtr<IDirectInputDevice8A>	mp_keyboard;
	DIMOUSESTATE2					m_mouse_state;
	unsigned char					m_key_state[256];
	bool							m_button_state[2][num_inputs];
	float							m_press_duration[num_inputs];
	size_t							m_button_mapping[num_inputs];
	size_t							m_idx;
	int								m_cursor_pos_x;
	int								m_cursor_pos_y;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace d3d12

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

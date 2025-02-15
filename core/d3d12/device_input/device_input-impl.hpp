
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace d3d12{

///////////////////////////////////////////////////////////////////////////////////////////////////
//device_input
///////////////////////////////////////////////////////////////////////////////////////////////////

//コンストラクタ
inline device_input::device_input(const HWND hwnd) : m_hwnd(hwnd)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//初期化
inline void device_input::initialize() 
{
	if(FAILED(DirectInput8Create(GetModuleHandle(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8, &mp_direct_input, nullptr)))
		throw std::runtime_error("DirectInput8 initialization failed.");
	if(FAILED(mp_direct_input->CreateDevice(GUID_SysKeyboard, &mp_keyboard, nullptr)))
		throw std::runtime_error("Keyboard CreateDevice failed.");
	if(FAILED(mp_keyboard->SetDataFormat(&c_dfDIKeyboard)))
		throw std::runtime_error("Keyboard SetDataFormat failed.");
	if(FAILED(mp_keyboard->SetCooperativeLevel(m_hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)))
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
	if(FAILED(mp_mouse->SetCooperativeLevel(m_hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)))
		throw std::runtime_error("Mouse SetCooperativeLevel failed.");

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

///////////////////////////////////////////////////////////////////////////////////////////////////

//終了
inline void device_input::finalize()
{
	mp_mouse.Reset();
	mp_keyboard.Reset();
	mp_direct_input.Reset();
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//更新
inline void device_input::update(const float dt)
{
	m_current_index = 1 - m_current_index;

	const HWND foreground = GetForegroundWindow();
	const bool visible = IsWindowVisible(foreground) != 0;
	
	if((foreground != m_hwnd) || not(visible))
	{
		memset(&m_press_duration, 0, sizeof(m_press_duration));
		memset(m_button_state[m_current_index], 0, sizeof(m_button_state[m_current_index]));
	}
	else
	{
		DIMOUSESTATE2 mouse_state;
		mp_mouse->Acquire();
		mp_mouse->GetDeviceState(sizeof(DIMOUSESTATE2), &mouse_state);

		m_mouse_delta[0] = mouse_state.lX;
		m_mouse_delta[1] = mouse_state.lY;
		m_mouse_delta[2] = mouse_state.lZ;

		mp_keyboard->Acquire();
		mp_keyboard->GetDeviceState(sizeof(m_key_state), m_key_state);

		for(size_t i = 0; i < num_keys; i++)
		{
			m_button_state[m_current_index][i] = ((m_key_state[m_button_mapping[i]] & 0x80) != 0);
		}
		for(size_t i = 0; i < 8; i++)
		{
			m_button_state[m_current_index][mouse0 + i] = (mouse_state.rgbButtons[i] > 0);
		}
		for(size_t i = 0; i < num_inputs; i++)
		{
			if(m_button_state[m_current_index][i] && m_button_state[1 - m_current_index][0])
				m_press_duration[i] += dt;
			else
				m_press_duration[i] = 0;
		}
	}

	POINT p;
	GetCursorPos(&p);
	ScreenToClient(m_hwnd, &p);
	m_mouse_pos[0] = p.x;
	m_mouse_pos[1] = p.y;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace d3d12

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

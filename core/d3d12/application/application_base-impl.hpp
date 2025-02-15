
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace d3d12{

///////////////////////////////////////////////////////////////////////////////////////////////////
//application_base
///////////////////////////////////////////////////////////////////////////////////////////////////

//コンストラクタ
inline application_base::application_base(const wchar_t* window_name, const HINSTANCE hinstance, const int ncmdshow) : m_window_name(window_name), m_hinstance(hinstance), m_ncmdshow(ncmdshow)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//初期化
inline void application_base::base_initialize(const uint2 present_size)
{
	Microsoft::WRL::Wrappers::RoInitializeWrapper InitializeWinRT(RO_INIT_MULTITHREADED);
	check_hresult(InitializeWinRT);

	//デバッグレイヤー有効化
#if _DEBUG || defined(ENABLE_DEBUG_LAYER)
	ComPtr<ID3D12Debug6> p_debug;
	if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&p_debug))))
	{
		p_debug->EnableDebugLayer();
		p_debug->SetEnableGPUBasedValidation(true);
		p_debug->SetForceLegacyBarrierValidation(true);
	}
#endif

	//ウィンドウ作成
	{
		WNDCLASSEX wcex;
		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = wnd_proc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = m_hinstance;
		wcex.hIcon = LoadIcon(m_hinstance, IDI_APPLICATION);
		wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wcex.lpszMenuName = nullptr;
		wcex.lpszClassName = m_window_name;
		wcex.hIconSm = LoadIcon(m_hinstance, IDI_APPLICATION);
		check_result(RegisterClassEx(&wcex) != 0);

		auto window_rect = RECT{ 0, 0, int(present_size.x), int(present_size.y) };
		AdjustWindowRect(&window_rect, m_window_style, FALSE);

		const auto width = window_rect.right - window_rect.left;
		const auto height = window_rect.bottom - window_rect.top;
		m_hwnd = CreateWindow(m_window_name, m_window_name, m_window_style, CW_USEDEFAULT, CW_USEDEFAULT, width, height, nullptr, nullptr, m_hinstance, nullptr);
		check_result(m_hwnd != 0);

		SetWindowLongPtr(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
	}

	gp_render_device.reset(new render_device(m_hwnd));
	gp_device_input.reset(new device_input(m_hwnd));
	gp_gui_manager.reset(new gui_manager(m_hwnd));
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//終了処理
inline void application_base::base_finalize()
{
	gp_gui_manager.reset();
	gp_device_input.reset();
	gp_render_device.reset();
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//メインループ
inline int application_base::main_loop()
{
	//ウィンドウの表示
	ShowWindow(m_hwnd, m_ncmdshow);
	
	//メッセージループ
	int ret = 0;
	while(true)
	{
		MSG msg = {};
		bool done = false;
		while(PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if(done = (msg.message == WM_QUIT)){ break; }
		}
		if(done){ break; }			

		//リサイズ
		if(m_window_size[0] != m_window_size[1])
		{
			m_window_size[0] = m_window_size[1];
			base_resize(m_window_size[0]);
		}

		//更新
		if((ret = base_update()) != 0)
		{
			DestroyWindow(m_hwnd);
			break;
		}
	}
	toggle_fullscreen(false);
	return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//ウィンドウプロシージャ
inline LRESULT application_base::wnd_proc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp)
{
	auto *p_app = reinterpret_cast<application_base*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
	if(p_app != nullptr)
	{
		if(static_cast<gui_manager*>(gp_gui_manager.get())->wnd_proc(message, wp, lp))
			return 0;

		switch(message)
		{
		case WM_SIZE:
		{
			p_app->m_window_size[1].x = LOWORD(lp);
			p_app->m_window_size[1].y = HIWORD(lp);
			return 0;
		}
		case WM_SYSKEYDOWN:
		{
			//Alt+Enter
			if((wp == VK_RETURN) && (lp & (1 << 29)))
			{
				auto& m_fullscreen = p_app->m_fullscreen;
				auto& m_window_rect = p_app->m_window_rect;

				//フルスクリーンから戻す
				if(m_fullscreen)
				{
					p_app->toggle_fullscreen(false);
					SetWindowLong(hwnd, GWL_STYLE, m_window_style);
					SetWindowPos(hwnd, nullptr, m_window_rect.left, m_window_rect.top, m_window_rect.right - m_window_rect.left, m_window_rect.bottom - m_window_rect.top, SWP_FRAMECHANGED | SWP_NOACTIVATE);
					ShowWindow(hwnd, SW_SHOWNORMAL);
				}
				//フルスクリーンに変更
				else
				{
					GetWindowRect(hwnd, &m_window_rect);
					SetWindowLong(hwnd, GWL_STYLE, m_window_style & ~(WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU | WS_THICKFRAME));
					SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED | SWP_NOACTIVATE);
					ShowWindow(hwnd, SW_MAXIMIZE);
					p_app->toggle_fullscreen(true);
				}
				m_fullscreen = not(m_fullscreen);
				return 0;
			}
			break;
		}
		case WM_DESTROY:
		{
			PostQuitMessage(0);
			return 0;
		}
		case WM_CLOSE:
		{
			DestroyWindow(hwnd);
			return 0;
		}}
	}
	return DefWindowProc(hwnd, message, wp, lp);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace d3d12

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//application_base
///////////////////////////////////////////////////////////////////////////////////////////////////

//�R���X�g���N�^
inline application_base::application_base(const uint2 present_size, const texture_format hdr_format) : m_present_size(present_size), m_hdr_format(hdr_format)
{
	m_delta_time = -1;
	m_frame_count = 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//���s
inline int application_base::run()
{
	//�f�o�C�X�̍쐬
	auto *p_device = new d3d12::render_device();
	gp_render_device.reset(p_device);

	//�X���b�v�`�F�C���쐬
	mp_swapchain = gp_render_device->create_swapchain(m_hwnd, m_present_size);
	m_back_buffer_index = mp_swapchain->back_buffer_index();

	//�X���b�v�`�F�C����RTV���쐬
	for(uint i = 0; i < mp_swapchain->buffer_count(); i++)
	{
		auto &back_buffer = mp_swapchain->back_buffer(i);
		m_back_buffer_rtv_ptrs[i] = gp_render_device->create_render_target_view(back_buffer, texture_rtv_desc(back_buffer));
		gp_render_device->set_name(back_buffer, (L"back_buffer " + std::to_wstring(i)).c_str());
	}
	
	//�f�o�C�X���͂̏�����
	m_device_input.initialize(m_hwnd);

	//GUI�}�l�[�W���̏�����
	gp_gui_manager.reset(new gui_manager());
	gp_gui_manager->initialize(*p_device, m_hwnd, mp_swapchain->buffer_count(), m_hdr_format);

	//�V�F�[�_�}�l�[�W���̏�����
	const uint max_instance_count = 65535;
	gp_shader_manager.reset(new shader_manager(*gp_render_device));

	//�A�v���P�[�V�����ŗL�̏���������
	initialize();
	
	//�E�B���h�E�̕\��
	ShowWindow(m_hwnd, nCmdShow);

	//���s
	auto ret = static_cast<Impl*>(this)->run();


	//���b�Z�[�W���[�v
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
	
		//���Ԃ̍X�V
		const time_point current_time = clock::now();
		const float delta_time = (m_frame_count == 0) ? 0 : float(std::chrono::duration_cast<std::chrono::nanoseconds>(current_time - m_current_time).count() / double(1000 * 1000));
		m_current_time = current_time;
		m_frame_count++;
	
		//�f�o�C�X���͂̍X�V
		m_device_input.update(m_hwnd, delta_time);
		m_gui_input.new_frame();

		//
		render_context context(*gp_render_device);
		gp_texture_manager->update(context);

		//�A�v���P�[�V�����ŗL�̍X�V
		if(update(context, delta_time) == false)
		{
			DestroyWindow(m_hwnd);
			break;
		}
	
		//��ʍX�V
		gp_render_device->present(*mp_swapchain);
		m_back_buffer_index = mp_swapchain->back_buffer_index();

		//�x�����
		g_delay_deleter.delay_delete();
	}
	
	//�A�v���P�[�V�����ŗL�̏I������
	finalize();
	
	//�f�o�C�X���͂̏I������
	m_device_input.finalize();
	m_gui_input.finalize();

	//�O���[�o���ϐ��̍폜
	g_delay_deleter.force_delete();
	gp_shader_manager.reset();
	gp_render_device.reset();
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

inline void application_base::change_present_size(const uint2 size)
{
	if(m_present_size == size)
		return;

	//����
	gp_render_device->wait_idle();

	//�X���b�v�`�F�C���̃��T�C�Y
	mp_swapchain->resize(size);
	m_back_buffer_index = mp_swapchain->back_buffer_index();
	for(uint i = 0; i < mp_swapchain->buffer_count(); i++)
	{
		auto &back_buffer = mp_swapchain->back_buffer(i);
		m_back_buffer_rtv_ptrs[i] = gp_render_device->create_render_target_view(back_buffer, texture_rtv_desc(back_buffer));
		gp_render_device->set_name(back_buffer, (L"back_buffer " + std::to_wstring(i)).c_str());
	}

	//�A�v���P�[�V�����ŗL�̃��T�C�Y
	resize(size);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//inline void application_base::toggle_fullscreen()
//{
//	//�t���X�N���[������߂�
//	if(m_fullscreen)
//	{
//		SetWindowLong(m_hwnd, GWL_STYLE, m_window_style);
//		SetWindowPos(m_hwnd, HWND_NOTOPMOST, m_window_rect.left, m_window_rect.top, m_window_rect.right - m_window_rect.left, m_window_rect.bottom - m_window_rect.top, SWP_FRAMECHANGED | SWP_NOACTIVATE);
//		ShowWindow(m_hwnd, SW_NORMAL);
//	}
//	//�t���X�N���[���ɕύX
//	else
//	{
//		GetWindowRect(m_hwnd, &m_window_rect);
//		SetWindowLong(m_hwnd, GWL_STYLE, m_window_style & ~(WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU | WS_THICKFRAME));
//
//		ComPtr<IDXGIOutput> pOutput;
//		m_swapchain.get_impl()->GetContainingOutput(&pOutput);
//
//		DXGI_OUTPUT_DESC Desc;
//		pOutput->GetDesc(&Desc);
//
//		RECT fullscreen_window_rect = Desc.DesktopCoordinates;
//		SetWindowPos(m_hwnd, HWND_TOPMOST, fullscreen_window_rect.left, fullscreen_window_rect.top, fullscreen_window_rect.right, fullscreen_window_rect.bottom, SWP_FRAMECHANGED | SWP_NOACTIVATE);
//		ShowWindow(m_hwnd, SW_MAXIMIZE);
//	}
//	m_fullscreen = not(m_fullscreen);
//}

///////////////////////////////////////////////////////////////////////////////////////////////////
//�֐���`
///////////////////////////////////////////////////////////////////////////////////////////////////

//�E�B���h�E�v���V�[�W��
inline LRESULT CALLBACK wnd_proc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp)
{
	auto *p_app = reinterpret_cast<application_base*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
	if(p_app->m_gui_input.wnd_proc(hwnd, message, wp, lp))
		return 0;

	switch(message)
	{
	case WM_SIZE:
	{
		RECT rect;
		GetClientRect(hwnd, &rect);
		p_app->change_present_size(uint2(rect.right - rect.left, rect.bottom - rect.top));
		return 0;
	}
	case WM_SYSKEYDOWN:
	{
		//Alt+Enter
		//if((wp == VK_RETURN) && (lp & (1 << 29)))
		//{
		//	p_app->toggle_fullscreen();
		//	return 0;
		//}
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
	return DefWindowProc(hwnd, message, wp, lp);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

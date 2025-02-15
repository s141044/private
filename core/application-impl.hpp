
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{ 

///////////////////////////////////////////////////////////////////////////////////////////////////

//コンストラクタ
template<class... Args> inline application::application(Args&&... args) : application_base(std::forward<Args>(args)...)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//実行
inline int application::run(const uint2 size)
{
	m_present_size = size;

	//基盤の初期化
	base_initialize(size);
	gp_device_input->initialize();
	gp_gui_manager->initialize(*gp_render_device, ldr_texture_format(), mp_swapchain->buffer_count());
	gp_shader_manager.reset(new shader_manager(*gp_render_device));
	
	//スワップチェインの作成
	mp_swapchain = gp_render_device->create_swapchain(size);
	m_back_buffer_index = mp_swapchain->back_buffer_index();

	//スワップチェインのRTVを作成
	for(uint i = 0; i < mp_swapchain->buffer_count(); i++)
	{
		auto &back_buffer = mp_swapchain->back_buffer(i);
		m_back_buffer_rtv_ptrs[i] = gp_render_device->create_render_target_view(back_buffer, texture_rtv_desc(back_buffer));
		gp_render_device->set_name(back_buffer, (L"back_buffer " + std::to_wstring(i)).c_str());
	}

	//アプリケーション固有の初期化
	initialize();

	//メインループ
	const auto ret = main_loop();

	//アプリケーション固有の終了処理
	finalize();
	
	//スワップチェインの破棄
	for(uint i = 0; i < mp_swapchain->buffer_count(); i++){ m_back_buffer_rtv_ptrs[i].reset(); }
	mp_swapchain.reset();

	//基盤の終了処理
	gp_shader_manager.reset();
	gp_gui_manager->finalize();
	gp_device_input->finalize();
	base_finalize();
	return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//更新
inline int application::base_update()
{
	const auto prev_time = m_current_time;
	m_current_time = clock::now();

	float delta_time = 0;
	if(prev_time.time_since_epoch().count() > 0)
		delta_time = float(std::chrono::duration_cast<std::chrono::nanoseconds>(m_current_time - prev_time).count() / double(1000 * 1000));

	//デバイス入力の更新
	gp_device_input->update(delta_time);

	//GUIの更新
	gp_gui_manager->new_frame();
	gp_gui_manager->property_edit();

	//アプリケーション固有の更新
	render_context context(*gp_render_device);
	const auto ret = update(context, delta_time);
	
	//画面更新
	gp_render_device->present(*mp_swapchain);
	m_back_buffer_index = mp_swapchain->back_buffer_index();

	//遅延開放
	g_delay_deleter.delay_delete();
	return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//リサイズ
inline void application::base_resize(const uint2 size)
{
	m_present_size = size;
	mp_swapchain->resize(size);
	m_back_buffer_index = mp_swapchain->back_buffer_index();
	for(uint i = 0; i < mp_swapchain->buffer_count(); i++)
	{
		auto &back_buffer = mp_swapchain->back_buffer(i);
		m_back_buffer_rtv_ptrs[i] = gp_render_device->create_render_target_view(back_buffer, texture_rtv_desc(back_buffer));
		gp_render_device->set_name(back_buffer, (L"back_buffer " + std::to_wstring(i)).c_str());
	}
	resize(size);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//フルスクリーンの切り替え
inline void application::toggle_fullscreen(bool fullscreen)
{
	mp_swapchain->toggle_fullscreen(fullscreen);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

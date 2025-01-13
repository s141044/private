
#pragma once

#ifndef NS_RENDER_APPLICATION_HPP
#define NS_RENDER_APPLICATION_HPP

#include<queue>
#include<array>
#include<vector>
#include<string>
#include<chrono>
#include<cassert>
#include<sstream>
#include<fstream>
#include<iostream>
#include<concepts>
#include<unordered_map>
#include<unordered_set>

#define DIRECTINPUT_VERSION 0x0800
#include<dinput.h>

#include"utility.hpp"
#include"core_impl.hpp"
#include"render_type.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//application_base
///////////////////////////////////////////////////////////////////////////////////////////////////

class application_base
{
public:

	//コンストラクタ
	application_base(const uint2 present_size = { 1920, 1080 });

	//初期化/終了処理
	virtual void initialize() = 0;
	virtual void finalize() = 0;

	//更新
	virtual bool update(render_context &context, const float dt) = 0;

	//リサイズ
	virtual void resize(const uint2 present_size) = 0;

	//実行
	int run(const wchar_t *className, HINSTANCE hInst, int nCmdShow);

protected:

	//デバイス入力を返す
	device_input &input(){ return m_device_input; }

	//バックバッファを返す
	uint back_buffer_index() const { return m_back_buffer_index; }
	texture &back_buffer() const { return back_buffer(m_back_buffer_index); }
	texture &back_buffer(const uint i) const { return mp_swapchain->back_buffer(i); } 
	render_target_view &back_buffer_rtv() const { return back_buffer_rtv(m_back_buffer_index); }
	render_target_view &back_buffer_rtv(const uint i) const { return *m_back_buffer_rtv_ptrs[i]; }

	//バックバッファサイズを返す
	uint2 back_buffer_size() const { return m_present_size; }

private:
	
	template<class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

	friend LRESULT CALLBACK wnd_proc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp);
	void change_present_size(uint2 size);
	//void toggle_fullscreen();

private:

	uint2									m_present_size;
	RECT									m_window_rect;
	bool									m_fullscreen;
	HWND									m_hwnd;

	swapchain_ptr							mp_swapchain;
	device_input							m_device_input;
	uint32_t								m_back_buffer_index = 0;
	render_target_view_ptr					m_back_buffer_rtv_ptrs[swapchain::buffer_count()];

	size_t									m_frame_count;
	time_point								m_current_time;
	float									m_delta_time;

	static constexpr uint					m_window_style = WS_OVERLAPPEDWINDOW;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//関数宣言
///////////////////////////////////////////////////////////////////////////////////////////////////

//ウィンドウプロシージャ
static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp);

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#include"application/application_base-impl.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

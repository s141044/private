
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

#include"core.hpp"
#include"utility.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////

template<class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

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
	virtual bool update(const float dt) = 0;

	//描画
	virtual void render() = 0;
	virtual void render_ui() = 0;

	//リサイズ
	virtual void resize(const uint2 present_size) = 0;

	//実行
	int run(const wchar_t *className, HINSTANCE hInst, int nCmdShow);

protected:

	//
	//render_device &device(){ return m_device; }

private:

	friend LRESULT CALLBACK wnd_proc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp);
	void change_render_size(uint2 size);
	void toggle_fullscreen();

private:

	uint2									m_present_size;
	RECT									m_window_rect;
	bool									m_fullscreen;
	HWND									m_hwnd;
	
	d3d12::render_device					m_device;

	swapchain_ptr							mp_swapchain;

	
	size_t			m_frame_count;
	time_point		m_current_time;
	float			m_delta_time;

	static constexpr uint			m_window_style = WS_OVERLAPPEDWINDOW;
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

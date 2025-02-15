
#pragma once

#ifndef NN_RENDER_CORE_HPP
#define NN_RENDER_CORE_HPP

#include"core/d3d12/gui.hpp"
#include"core/d3d12/application.hpp"
#include"core/d3d12/device_input.hpp"
#include"core/d3d12/render_device.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{ 

///////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(_DEBUG) || !defined(NDEBUG)
using gui_manager		= iface::gui_manager;
using device_input		= iface::device_input;
using render_device		= iface::render_device;
#else
using gui_manager		= d3d12::gui_manager;
using device_input		= d3d12::device_input;
using render_device		= d3d12::render_device;
#endif

using application_base	= d3d12::application_base;
static_assert(std::is_base_of<iface::application_base, application_base>::value);

///////////////////////////////////////////////////////////////////////////////////////////////////
//application
///////////////////////////////////////////////////////////////////////////////////////////////////

class application : public application_base
{
public:

	//コンストラクタ
	template<class... Args> application(Args&&... args);

	//実行
	int run(const uint2 size);

private:

	//初期化
	virtual void initialize() = 0;

	//終了処理
	virtual void finalize() = 0;

	//更新
	virtual int update(render_context& context, const float delta_time) = 0;

	//リサイズ
	virtual void resize(const uint2 size) = 0;

private:

	//更新
	int base_update() override final;

	//リサイズ
	void base_resize(const uint2 size) override final;

	//フルスクリーンの切り替え
	void toggle_fullscreen(bool fullscreen) override final;

protected:

	//画面解像度を返す
	uint2 present_size() const { return m_present_size; }

	//バックバッファを返す
	render_target_view& back_buffer_rtv(const uint index) const { return *m_back_buffer_rtv_ptrs[index]; }
	render_target_view& back_buffer_rtv() const { return *m_back_buffer_rtv_ptrs[m_back_buffer_index]; }

	//バックバッファのインデックスを返す
	uint back_buffer_index() const { return m_back_buffer_index; }

	//ldrフォーマットを返す
	texture_format ldr_texture_format() const { return texture_format_r10g10b10a2_unorm; }

private:

	uint2					m_present_size;
	swapchain_ptr			mp_swapchain;
	uint					m_back_buffer_index;
	render_target_view_ptr	m_back_buffer_rtv_ptrs[swapchain::buffer_count()];
	time_point				m_current_time;
	float					m_delta_time;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#include"core/application-impl.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

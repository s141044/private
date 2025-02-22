
#pragma once

#ifndef NN_RENDER_CORE_GUI_HPP
#define NN_RENDER_CORE_GUI_HPP

#include"utility.hpp"
#include"imgui/imgui.h"
#include"render_device.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//前方宣言
///////////////////////////////////////////////////////////////////////////////////////////////////

class property_editor;

///////////////////////////////////////////////////////////////////////////////////////////////////
//gui_manager
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace iface{ class gui_manager
{
public:

	//デストラクタ
	virtual ~gui_manager(){}

	//初期化処理
	virtual void initialize(iface::render_device &device, const texture_format rtv_format, const uint buffer_count) = 0;

	//終了処理
	virtual void finalize() = 0;

	//新規フレーム開始
	virtual void new_frame() = 0;

	//GUI描画
	virtual void render(iface::render_device& device, render_context& context) = 0;

public:

	//登録/解除
	bool register_editor(const size_t key, property_editor* p_editor);
	bool unregister_editor(const size_t key);

	//編集実行
	void property_edit();

private:

	unordered_map<size_t, unique_ptr<property_editor>> m_editor_ptrs;
};}

///////////////////////////////////////////////////////////////////////////////////////////////////
//property_editor
///////////////////////////////////////////////////////////////////////////////////////////////////

class property_editor
{
public:

	//コンストラクタ
	property_editor(string name) : m_name(std::move(name)){}

	//デストラクタ
	virtual ~property_editor(){}

	//編集
	virtual void edit() = 0;

	//名前を返す
	const string& name() const { return m_name; }

protected:

	class tree_node
	{
	public:

		tree_node(const char* label){ m_is_open = ImGui::TreeNode(label); }
		~tree_node(){ if(m_is_open){ ImGui::TreePop(); }}
		bool is_open() const { return m_is_open; }

	private:

		bool m_is_open;
	};

protected:

	//ヘッダ
	bool header(const char* label);

	//セパレータ
	void separator(const char* label);

	//ボタン
	bool button(const char* label);

	//チェックボックス
	bool checkbox(const char* label, bool& check);

	//コンボボックス
	bool combobox(const char* label, int& selected, const char** item_names, const int item_count);

	//入力ボックス
	bool input_int(const char* label, int& value, int step = 1, int step_fast = 100);
	bool input_int2(const char* label, int2& value);
	bool input_int3(const char* label, int3& value);
	bool input_int4(const char* label, int4& value);
	bool input_float(const char* label, float& value, float step = 1, float step_fast = 100);
	bool input_float2(const char* label, float2& value);
	bool input_float3(const char* label, float3& value);
	bool input_float4(const char* label, float4& value);

	//スライダー
	bool slider_int(const char* label, int& value, int min, int max);
	bool slider_float(const char* label, float& value, float min, float max);

	//色
	bool color_edit(const char* label, float3& color);
	bool color_edit(const char* label, float4& color);

private:

	string m_name;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#include"gui/gui_manager-impl.hpp"
#include"gui/property_editor-impl.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

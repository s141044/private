
#pragma once

#ifndef NN_RENDER_CORE_GUI_HPP
#define NN_RENDER_CORE_GUI_HPP

#include"utility.hpp"
#include"imgui/imgui.h"
#include"render_device.hpp"
#include"../../utility/json.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//�O���錾
///////////////////////////////////////////////////////////////////////////////////////////////////

class property_editor;

///////////////////////////////////////////////////////////////////////////////////////////////////
//gui_manager
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace iface{ class gui_manager
{
public:

	//�f�X�g���N�^
	virtual ~gui_manager(){}

	//����������
	virtual void initialize(iface::render_device &device, const texture_format rtv_format, const uint buffer_count) = 0;

	//�I������
	virtual void finalize() = 0;

	//�V�K�t���[���J�n
	virtual void new_frame() = 0;

	//GUI�`��
	virtual void render(iface::render_device& device, render_context& context) = 0;

public:

	struct hasher{ size_t operator()(const unique_ptr<property_editor>& p) const; };
	using unordered_set = unordered_set<unique_ptr<property_editor>, hasher>;
	using handle = unordered_set::iterator;

	//�o�^/����
	handle register_editor(property_editor* p_editor);
	void unregister_editor(handle handle);

	//�ҏW���s
	void property_edit();

	//�V���A���C�Y/�f�V���A���C�Y
	void serialize(string filename);
	void deserialize(string filename);

private:

	unordered_set m_editor_ptrs;
};}

///////////////////////////////////////////////////////////////////////////////////////////////////
//property_editor
///////////////////////////////////////////////////////////////////////////////////////////////////

class property_editor
{
public:

	//�R���X�g���N�^
	property_editor(string name) : m_name(std::move(name)){}

	//�f�X�g���N�^
	virtual ~property_editor(){}

	//�ҏW
	virtual void edit() = 0;

	//�V���A���C�Y/�f�V���A���C�Y
	virtual void serialize(json_file::values_t& json) = 0;
	virtual void deserialize(const json_file::values_t& json) = 0;

	//���O��Ԃ�
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

	//�w�b�_
	bool header(const char* label);

	//�Z�p���[�^
	void separator();
	void separator(const char* label);

	//�{�^��
	bool button(const char* label);

	//�`�F�b�N�{�b�N�X
	bool checkbox(const char* label, bool& check);

	//�R���{�{�b�N�X
	bool combobox(const char* label, int& selected, const char** item_names, const int item_count);

	//���̓{�b�N�X
	bool input_int(const char* label, int& value, int step = 1, int step_fast = 100);
	bool input_int2(const char* label, int2& value);
	bool input_int3(const char* label, int3& value);
	bool input_int4(const char* label, int4& value);
	bool input_float(const char* label, float& value, float step = 1, float step_fast = 100);
	bool input_float2(const char* label, float2& value);
	bool input_float3(const char* label, float3& value);
	bool input_float4(const char* label, float4& value);
	bool input_text(const char* label, char* buf, size_t bus_size);

	//�X���C�_�[
	bool slider_int(const char* label, int& value, int min, int max);
	bool slider_float(const char* label, float& value, float min, float max);

	//�F
	bool color_edit(const char* label, float3& color);
	bool color_edit(const char* label, float4& color);

protected:

	//json��������
	static void write_int(json_file::values_t& json, const char* label, const int& value);
	static void write_int2(json_file::values_t& json, const char* label, const int2& value);
	static void write_int3(json_file::values_t& json, const char* label, const int3& value);
	static void write_int4(json_file::values_t& json, const char* label, const int4& value);
	static void write_bool(json_file::values_t& json, const char* label, const bool& value);
	static void write_float(json_file::values_t& json, const char* label, const float& value);
	static void write_float2(json_file::values_t& json, const char* label, const float2& value);
	static void write_float3(json_file::values_t& json, const char* label, const float3& value);
	static void write_float4(json_file::values_t& json, const char* label, const float4& value);
	static void write_text(json_file::values_t& json, const char* label, const string& value);
	static json_file::values_t* write_tree_node(json_file::values_t& json, const char* label);

	//json�ǂݍ���
	static bool read_int(const json_file::values_t& json, const char* label, int& value);
	static bool read_int2(const json_file::values_t& json, const char* label, int2& value);
	static bool read_int3(const json_file::values_t& json, const char* label, int3& value);
	static bool read_int4(const json_file::values_t& json, const char* label, int4& value);
	static bool read_bool(const json_file::values_t& json, const char* label, bool& value);
	static bool read_float(const json_file::values_t& json, const char* label, float& value);
	static bool read_float2(const json_file::values_t& json, const char* label, float2& value);
	static bool read_float3(const json_file::values_t& json, const char* label, float3& value);
	static bool read_float4(const json_file::values_t& json, const char* label, float4& value);
	static bool read_text(const json_file::values_t& json, const char* label, string& value);
	static const json_file::values_t* read_tree_node(const json_file::values_t& json, const char* label);

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

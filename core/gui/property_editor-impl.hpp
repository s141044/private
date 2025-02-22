
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//property_editor
///////////////////////////////////////////////////////////////////////////////////////////////////

//ヘッダ
inline bool property_editor::header(const char* label)
{
	return ImGui::CollapsingHeader(label);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//セパレータ
inline void property_editor::separator()
{
	return ImGui::Separator();
}

inline void property_editor::separator(const char* label)
{
	return ImGui::SeparatorText(label);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//ボタン
inline bool property_editor::button(const char* label)
{
	return ImGui::Button(label);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//チェックボックス
inline bool property_editor::checkbox(const char* label, bool& check)
{
	return ImGui::Checkbox(label, &check);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//コンボボックス
inline bool property_editor::combobox(const char* label, int& selected, const char** item_names, const int item_count)
{
	return ImGui::Combo(label, &selected, item_names, item_count);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//入力ボックス
inline bool property_editor::input_int(const char* label, int& value, int step, int step_fast)
{
	return ImGui::InputInt(label, &value, step, step_fast);
}

inline bool property_editor::input_int2(const char* label, int2& value)
{
	return ImGui::InputInt2(label, &value[0]);
}

inline bool property_editor::input_int3(const char* label, int3& value)
{
	return ImGui::InputInt3(label, &value[0]);
}

inline bool property_editor::input_int4(const char* label, int4& value)
{
	return ImGui::InputInt4(label, &value[0]);
}

inline bool property_editor::input_float(const char* label, float& value, float step, float step_fast)
{
	return ImGui::InputFloat(label, &value, step, step_fast);
}

inline bool property_editor::input_float2(const char* label, float2& value)
{
	return ImGui::InputFloat2(label, &value[0]);
}

inline bool property_editor::input_float3(const char* label, float3& value)
{
	return ImGui::InputFloat3(label, &value[0]);
}

inline bool property_editor::input_float4(const char* label, float4& value)
{
	return ImGui::InputFloat4(label, &value[0]);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//スライダー
inline bool property_editor::slider_int(const char* label, int& value, int min, int max)
{
	return ImGui::SliderInt(label, &value, min, max);
}

inline bool property_editor::slider_float(const char* label, float& value, float min, float max)
{
	return ImGui::SliderFloat(label, &value, min, max, "%.6f");
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//色
inline bool property_editor::color_edit(const char* label, float3& color)
{
	return ImGui::ColorEdit3(label, &color[0]);
}

inline bool property_editor::color_edit(const char* label, float4& color)
{
	return ImGui::ColorEdit4(label, &color[0]);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//json書き込み
namespace anon{ template<class T> inline void property_editor_write_impl(json_file::values_t& json, const char* label, const T* values, const int num_values)
{
	auto result = json.try_emplace(label);
	if(!result.second)
		throw std::runtime_error("property_editor::write_xxx(..., " + string(label) + ", ...)");
	else
	{
		auto& json_values = result.first->second;
		for(int i = 0; i < num_values; i++)
		{
			if(std::is_same<T, bool>::value)
				json_values.emplace_back(bool(values[i]));
			else
				json_values.emplace_back(double(values[i]));
		}
	}
}}

inline void property_editor::write_int(json_file::values_t& json, const char* label, const int& value)
{
	anon::property_editor_write_impl(json, label, &value, 1);
}

inline void property_editor::write_int2(json_file::values_t& json, const char* label, const int2& value)
{
	anon::property_editor_write_impl(json, label, &value[0], 2);
}

inline void property_editor::write_int3(json_file::values_t& json, const char* label, const int3& value)
{
	anon::property_editor_write_impl(json, label, &value[0], 3);
}

inline void property_editor::write_int4(json_file::values_t& json, const char* label, const int4& value)
{
	anon::property_editor_write_impl(json, label, &value[0], 4);
}

inline void property_editor::write_float(json_file::values_t& json, const char* label, const float& value)
{
	anon::property_editor_write_impl(json, label, &value, 1);
}

inline void property_editor::write_float2(json_file::values_t& json, const char* label, const float2& value)
{
	anon::property_editor_write_impl(json, label, &value[0], 2);
}

inline void property_editor::write_float3(json_file::values_t& json, const char* label, const float3& value)
{
	anon::property_editor_write_impl(json, label, &value[0], 3);
}

inline void property_editor::write_float4(json_file::values_t& json, const char* label, const float4& value)
{
	anon::property_editor_write_impl(json, label, &value[0], 4);
}

inline void property_editor::write_bool(json_file::values_t& json, const char* label, const bool& value)
{
	anon::property_editor_write_impl(json, label, &value, 1);
}

inline json_file::values_t* property_editor::write_tree_node(json_file::values_t& json, const char* label)
{
	auto result = json.try_emplace(label);
	if(!result.second)
		return nullptr;

	auto* values = new json_file::values_t;
	result.first->second.emplace_back(values);
	return values;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//json読み込み
namespace anon{ template<class T> inline bool property_editor_read_impl(const json_file::values_t& json, const char* label, T* values, const int num_values)
{
	auto it = json.find(label);
	if(it == json.end())
		return false;

	const auto& json_values = it->second;
	for(int i = 0; i < num_values; i++)
	{
		if(std::is_same<T, bool>::value)
			values[i] = bool(json_values[i]);
		else
			values[i] = T(double(json_values[i]));
	}
	return true;
}}

inline bool property_editor::read_int(const json_file::values_t& json, const char* label, int& value)
{
	return anon::property_editor_read_impl(json, label, &value, 1);
}

inline bool property_editor::read_int2(const json_file::values_t& json, const char* label, int2& value)
{
	return anon::property_editor_read_impl(json, label, &value[0], 2);
}

inline bool property_editor::read_int3(const json_file::values_t& json, const char* label, int3& value)
{
	return anon::property_editor_read_impl(json, label, &value[0], 3);
}

inline bool property_editor::read_int4(const json_file::values_t& json, const char* label, int4& value)
{
	return anon::property_editor_read_impl(json, label, &value[0], 4);
}

inline bool property_editor::read_float(const json_file::values_t& json, const char* label, float& value)
{
	return anon::property_editor_read_impl(json, label, &value, 1);
}

inline bool property_editor::read_float2(const json_file::values_t& json, const char* label, float2& value)
{
	return anon::property_editor_read_impl(json, label, &value[0], 2);
}

inline bool property_editor::read_float3(const json_file::values_t& json, const char* label, float3& value)
{
	return anon::property_editor_read_impl(json, label, &value[0], 3);
}

inline bool property_editor::read_float4(const json_file::values_t& json, const char* label, float4& value)
{
	return anon::property_editor_read_impl(json, label, &value[0], 4);
}

inline bool property_editor::read_bool(const json_file::values_t& json, const char* label, bool& value)
{
	return anon::property_editor_read_impl(json, label, &value, 1);
}

inline const json_file::values_t* property_editor::read_tree_node(const json_file::values_t& json, const char* label)
{
	auto it = json.find(label);
	if(it == json.end())
		return nullptr;

	const json_file::values_t& values = it->second[0];
	return &values;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

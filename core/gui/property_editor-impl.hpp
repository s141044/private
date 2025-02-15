
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

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

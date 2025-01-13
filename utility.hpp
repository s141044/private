
#pragma once

#ifndef NN_RENDER_UTILITY_HPP
#define NN_RENDER_UTILITY_HPP

#include"../vector.hpp"
#include"../utility.hpp"

#include"utility/half.hpp"
#include"utility/math.hpp"
#include"utility/convert.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//関数定義
///////////////////////////////////////////////////////////////////////////////////////////////////

inline void check_hresult(const HRESULT hresult)
{
#ifdef _DEBUG
	if(FAILED(hresult)){ __debugbreak(); }
#endif
}

inline void check_result(const bool success)
{
#if _DEBUG
	if(not(success)){ __debugbreak(); }
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//単位変換
template<class T> inline constexpr T kilo(const T val)
{
	return val * 1000;
}

template<class T> inline constexpr T mega(const T val)
{
	return val * 1000 * 1000;
}

template<class T> inline constexpr T kibi(const T val)
{
	return val * 1000;
}

template<class T> inline constexpr T mebi(const T val)
{
	return val * 1000 * 1000;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//ハッシュ値を計算
template<class T> inline size_t calc_hash(const T *p, const size_t n)
{
	return std::hash<std::string_view>{}(std::string_view(reinterpret_cast<const char*>(p), sizeof(T) * n));
}

template<class T, size_t N> inline size_t calc_hash(const T (&a)[N])
{
	return calc_hash(a, sizeof(T) * N);;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//UTF8からUTF16へ
inline std::wstring utf8_to_utf16(const std::string &utf8)
{
	std::wstring utf16;
	utf16.resize(MultiByteToWideChar(CP_UTF8, 0, utf8.data(), int(utf8.size()), nullptr, 0));
	MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), int(utf8.size()), utf16.data(), int(utf16.size()));
	return utf16;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//UTF16からUTF8へ
inline std::string utf16_to_utf8(const std::wstring &utf16)
{
	std::string utf8;
	utf8.resize(WideCharToMultiByte(CP_UTF8, 0, utf16.data(), int(utf16.size()), nullptr, 0, nullptr, nullptr));
	WideCharToMultiByte(CP_UTF8, 0, utf16.c_str(), int(utf16.size()), utf8.data(), int(utf8.size()), nullptr, nullptr);
	return utf8;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//パスを正規化
inline std::string format(const std::string &path)
{
	return std::filesystem::path(path).make_preferred().lexically_normal().string();
}

inline std::wstring format(const std::wstring &path)
{
	return std::filesystem::path(path).make_preferred().lexically_normal().wstring();
}

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

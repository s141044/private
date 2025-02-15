
#pragma once

#ifndef NN_RENDER_CORE_UTILITY_HPP
#define NN_RENDER_CORE_UTILITY_HPP

#include<queue>
#include<deque>
#include<stack>
#include<array>
#include<mutex>
#include<vector>
#include<string>
#include<chrono>
#include<atomic>
#include<cassert>
#include<sstream>
#include<fstream>
#include<iostream>
#include<concepts>
#include<functional>
#include<filesystem>
#include<shared_mutex>
#include<unordered_map>
#include<unordered_set>

#define USE_CODECVT
#if defined(USE_CODECVT) //将来削除されるがwindows.hのインクルードもしたくない
#include<codecvt>
#else
#include<windows.h>
#endif

#include"../../vector.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//前方宣言
///////////////////////////////////////////////////////////////////////////////////////////////////

template<class Key, class Val, class Hasher = std::hash<Key>, class Comp = std::equal_to<Key>, class Alloc = std::allocator<std::pair<const Key, Val>>> 
using unordered_map = std::unordered_map<Key, Val, Hasher, Comp, Alloc>;

template<class Key, class Hasher = std::hash<Key>, class Comp = std::equal_to<Key>, class Alloc = std::allocator<Key>> 
using unordered_set = std::unordered_set<Key, Hasher, Comp, Alloc>;

template<class T, class Alloc = std::allocator<T>>
using vector = std::vector<T, Alloc>;

template<class T, class Alloc = std::allocator<T>>
using deque = std::deque<T, Alloc>;

template<class T, class Container = std::deque<T>>
using stack = std::stack<T, Container>;

template<class T, class Container = std::deque<T>>
using queue = std::queue<T, Container>;

template<class T, class Deleter = std::default_delete<T>>
using unique_ptr = std::unique_ptr<T, Deleter>;

template<class T> using weak_ptr = std::weak_ptr<T>;
template<class T> using shared_ptr = std::shared_ptr<T>;

using string = std::string;
using wstring = std::wstring;

template<class T, size_t N> 
using array = std::array<T, N>;

template<class T, class U> 
using pair = std::pair<T, U>;

template<class... Ts> 
using tuple = std::tuple<Ts...>;

using mutex = std::mutex;
using shared_mutex = std::shared_mutex;

template<class F>
using function = std::function<F>;

///////////////////////////////////////////////////////////////////////////////////////////////////

using clock = std::chrono::high_resolution_clock;
using time_point = clock::time_point;

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace iface{ class render_device; }

class render_context;
class swapchain;
class texture;
class buffer;
class upload_buffer;
class constant_buffer;
class readback_buffer;
class render_target_view;
class depth_stencil_view;
class shader_resource_view;
class unordered_access_view;
class sampler;
class shader;
class shader_compiler;
class shader_file;
class target_state;
class pipeline_state;
class geometry_state;
class top_level_acceleration_structure;
class bottom_level_acceleration_structure;

///////////////////////////////////////////////////////////////////////////////////////////////////

template<class T> class intrusive_ptr;

using swapchain_ptr = intrusive_ptr<swapchain>;
using texture_ptr = intrusive_ptr<texture>;
using buffer_ptr = intrusive_ptr<buffer>;
using upload_buffer_ptr = intrusive_ptr<upload_buffer>;
using constant_buffer_ptr = intrusive_ptr<constant_buffer>;
using readback_buffer_ptr = intrusive_ptr<readback_buffer>;
using render_target_view_ptr = intrusive_ptr<render_target_view>;
using depth_stencil_view_ptr = intrusive_ptr<depth_stencil_view>;
using shader_resource_view_ptr = intrusive_ptr<shader_resource_view>;
using unordered_access_view_ptr = intrusive_ptr<unordered_access_view>;
using sampler_ptr = intrusive_ptr<sampler>;
using target_state_ptr = intrusive_ptr<target_state>;
using geometry_state_ptr = intrusive_ptr<geometry_state>;
using pipeline_state_ptr = intrusive_ptr<pipeline_state>;
using shader_ptr = intrusive_ptr<shader>;
using top_level_acceleration_structure_ptr = intrusive_ptr<top_level_acceleration_structure>;
using bottom_level_acceleration_structure_ptr = intrusive_ptr<bottom_level_acceleration_structure>;

using shader_file_ptr = shared_ptr<shader_file>;
using shader_compiler_ptr = shared_ptr<shader_compiler>;

///////////////////////////////////////////////////////////////////////////////////////////////////
//immovable
///////////////////////////////////////////////////////////////////////////////////////////////////

template<class T> struct immovable
{
	immovable() = default;
	immovable(immovable&&) = delete;
	immovable(const immovable&) = delete;
	immovable operator=(immovable&&) = delete;
	immovable operator=(const immovable&) = delete;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//関数定義
///////////////////////////////////////////////////////////////////////////////////////////////////

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
#if defined(USE_CODECVT)
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.from_bytes(utf8);
#else
	std::wstring utf16;
	utf16.resize(MultiByteToWideChar(CP_UTF8, 0, utf8.data(), int(utf8.size()), nullptr, 0));
	MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), int(utf8.size()), utf16.data(), int(utf16.size()));
	return utf16;
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//UTF16からUTF8へ
inline std::string utf16_to_utf8(const std::wstring &utf16)
{
#if defined(USE_CODECVT)
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.to_bytes(utf16);
#else
	std::string utf8;
	utf8.resize(WideCharToMultiByte(CP_UTF8, 0, utf16.data(), int(utf16.size()), nullptr, 0, nullptr, nullptr));
	WideCharToMultiByte(CP_UTF8, 0, utf16.c_str(), int(utf16.size()), utf8.data(), int(utf8.size()), nullptr, nullptr);
#endif
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

//切り上げ除算
template<class T, class U> inline std::common_type_t<T, U> ceil_div(const T x, const U y)
{
	return (x + y - 1) / y;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//x以上のyの倍数に切り上げ
template<class T, class U> inline std::common_type_t<T, U> roundup(const T x, const U y)
{
	return ceil_div(x, y) * y;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

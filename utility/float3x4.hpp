
#pragma once

#ifndef NN_RENDER_UTILITY_FLOAT3x4_HPP
#define NN_RENDER_UTILITY_FLOAT3x4_HPP

#include"../../vector.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//float3x4
///////////////////////////////////////////////////////////////////////////////////////////////////

class float3x4
{
public:

	//コンストラクタ
	float3x4() = default;
	float3x4(const float4x4 &m) : m_r{m[0], m[1], m[2]}{}
	float3x4(const float4 &r0, const float4 &r1, const float4 &r2) : m_r{r0, r1, r2}{}
	float3x4(
		const float _00, const float _01, const float _02, const float _03, 
		const float _10, const float _11, const float _12, const float _13, 
		const float _20, const float _21, const float _22, const float _23) : m_r{float4(_00, _01, _02, _03), float4(_10, _11, _12, _13), float4(_20, _21, _22, _23)}{}

	//要素を返す
	float &operator()(const size_t i, const size_t j){ return m_r[i][j]; }
	const float &operator()(const size_t i, const size_t j) const { return m_r[i][j]; }

	//行ベクトルを返す
	float4 &operator[](const size_t i){ return m_r[i]; }
	const float4 &operator[](const size_t i) const { return m_r[i]; }

	//キャスト演算子
	explicit operator float4x4() const { return float4x4(m_r[0], m_r[1], m_r[2], float4(0, 0, 0, 1)); }

private:

	float4 m_r[3];
};

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

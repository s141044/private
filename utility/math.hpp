
#pragma once

#ifndef NN_RENDER_UTILITY_MATH_HPP
#define NN_RENDER_UTILITY_MATH_HPP

#include<cmath>

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//�֐���`
///////////////////////////////////////////////////////////////////////////////////////////////////

//���Ȃ�1���Ȃ�-1��Ԃ�
inline float sign(const float f)
{
	return (f >= 0) ? 1.0f : -1.0f;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//0����1�ɃN�����v
inline float saturate(const float f)
{
	return std::clamp(f, 0.0f, 1.0f);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

#define MAKE_VECTOR_OP(func) \
inline float2 func(const float2 &f){ return float2(func(f.x), func(f.y)); } \
inline float3 func(const float3 &f){ return float3(func(f.x), func(f.y), func(f.z)); } \
inline float4 func(const float4 &f){ return float4(func(f.x), func(f.y), func(f.z), func(f.w)); }

#define MAKE_STD_VECTOR_OP(func) \
using std::func; \
inline float2 func(const float2 &f){ return float2(func(f.x), func(f.y)); } \
inline float3 func(const float3 &f){ return float3(func(f.x), func(f.y), func(f.z)); } \
inline float4 func(const float4 &f){ return float4(func(f.x), func(f.y), func(f.z), func(f.w)); }

MAKE_VECTOR_OP(sign)
MAKE_VECTOR_OP(saturate)
MAKE_STD_VECTOR_OP(abs)
MAKE_STD_VECTOR_OP(sin)
MAKE_STD_VECTOR_OP(cos)
MAKE_STD_VECTOR_OP(tan)
MAKE_STD_VECTOR_OP(exp)
MAKE_STD_VECTOR_OP(log)

#undef MAKE_VECTOR_OP
#undef MAKE_STD_VECTOR_OP

///////////////////////////////////////////////////////////////////////////////////////////////////

//�P�x��Ԃ�
inline float luminance(float3 rgb)
{
	return dot(rgb, float3(0.2126f, 0.7152f, 0.0722f));
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//���K�������x�N�g�����v�Z
inline void calc_orthonormal_basis(float3 normal, float3 &tangent, float3 &binormal)
{
	float s = sign(normal.z);
	float a = -1 / (s + normal.z);
	float b = normal.x * normal.y * a;
	tangent = float3(1 + s * normal.x * normal.x * a, s * b, -s * normal.x);
	binormal = float3(b, s + normal.y * normal.y * a, -normal.y);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//�x���@�ƌʓx�@�̕ϊ�
inline float to_degree(const float radian)
{
	return radian * (180 / PI());
}

inline float to_radian(const float degree)
{
	return degree * (PI() / 180);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

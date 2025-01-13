
#pragma once

#ifndef NN_RENDER_UTILITY_CONVERT_HPP
#define NN_RENDER_UTILITY_CONVERT_HPP

#include"math.hpp"
#include"half.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//ŠÖ”’è‹`
///////////////////////////////////////////////////////////////////////////////////////////////////

//•ûŒü‚ğ[-1,1]^2‚Ì‹óŠÔ‚É•ÏŠ·
inline float2 f32x3_to_oct(const float3 &v)
{
	float2 p = v.xy / (abs(v.x) + abs(v.y) + abs(v.z));
	return (v.z <= 0) ? (float2(1) - abs(p.yx)) * sign(p) : p;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//[-1,1]^2‚Ì‹óŠÔ‚ğ•ûŒü‚É•ÏŠ·
inline float3 oct_to_f32x3(const float2 &e)
{
	float3 v = float3(e.xy, 1 - abs(e.x) - abs(e.y));
	if(v.z < 0){ v.xy = (float2(1) - abs(v.yx)) * sign(v.xy); }
	return normalize(v);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

inline uint f32x4_to_u8x4_unorm(const float4 &v)
{
	return 
		(uint(saturate(v.x) * 255 + 0.5f) << (8 * 0)) | 
		(uint(saturate(v.y) * 255 + 0.5f) << (8 * 1)) | 
		(uint(saturate(v.z) * 255 + 0.5f) << (8 * 2)) | 
		(uint(saturate(v.w) * 255 + 0.5f) << (8 * 3));
}

inline float4 u8x4_unorm_to_f32x4(const uint v)
{
	return float4(
		float((v >> (8 * 0)) & 0xff), 
		float((v >> (8 * 1)) & 0xff), 
		float((v >> (8 * 2)) & 0xff), 
		float((v >> (8 * 3)) & 0xff)) / 255.0f;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

inline uint f32_to_u8_unorm(const float v)
{
	return uint(saturate(v) * 255 + 0.5f);
}

inline float u8_unorm_to_f32(const uint v)
{
	return float(v & 0xff) / 255;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

inline uint f32x2_to_u16x2_unorm(const float2 &v)
{
	return 
		uint(saturate(v.x) * 65535 + 0.5f) | 
		uint(saturate(v.y) * 65535 + 0.5f) << 16;
}

inline float2 u16x2_unorm_to_f32x2(const uint v)
{
	return float2(
		float((v & 0x0000ffff)),
		float((v & 0xffff0000) >> 16)) / 65535;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

inline uint f32x2_to_u16x2_snorm(const float2 &v)
{
	return 
		(int(std::trunc(std::clamp(v.x, -1.0f, 1.0f) * 32767 + ((v.x >= 0) ? 0.5f : -0.5f))) & 0xffff) |
		(int(std::trunc(std::clamp(v.y, -1.0f, 1.0f) * 32767 + ((v.y >= 0) ? 0.5f : -0.5f))) << 16);
}

inline float2 u16x2_snorm_to_f32x2(const uint v)
{
	return float2(
		std::max((int(v << 16) >> 16) / 32767.0f, -1.0f),
		std::max((int(v) >> 16) / 32767.0f, -1.0f));
}

///////////////////////////////////////////////////////////////////////////////////////////////////

inline uint f32x2_to_f16x2(const float2 &v)
{
	return reinterpret<uint16_t>(half(v.x)) | (reinterpret<uint16_t>(half(v.x)) << 16);
}

inline float2 f16x2_to_f32x2(const uint v)
{
	return float2(float(reinterpret<half>(uint16_t(v & 0xffff))), float(reinterpret<half>(uint16_t(v >> 16))));
}

///////////////////////////////////////////////////////////////////////////////////////////////////

inline uint f32x3_to_r9g9b9e5(const float3 &col)
{
	static const float max_val = float(0x1FF << 7);
	static const float min_val = float(1.f / (1 << 16));

	const float r = std::clamp(col[0], 0.0f, max_val);
	const float g = std::clamp(col[1], 0.0f, max_val);
	const float b = std::clamp(col[2], 0.0f, max_val);
	const float max_channel = std::max(std::max(r, g), std::max(b, min_val));

	union { float f; int32_t i; } R, G, B, E;
	E.f = max_channel;
	E.i += 0x07804000;
	E.i &= 0x7F800000;

	R.f = r + E.f;
	G.f = g + E.f;
	B.f = b + E.f;

	E.i <<= 4;
	E.i += 0x10000000;
	return E.i | B.i << 18 | G.i << 9 | R.i & 511;
}

inline float3 r9g9b9e5_to_f32x3(const uint v)
{
	return float3(
		ldexp(float((v >> (9 * 0)) & 0x1ff), int(v >> 27) - 24),
		ldexp(float((v >> (9 * 1)) & 0x1ff), int(v >> 27) - 24),
		ldexp(float((v >> (9 * 2)) & 0x1ff), int(v >> 27) - 24));
}

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

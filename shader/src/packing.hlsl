
#ifndef PACKING_HLSL
#define PACKING_HLSL

///////////////////////////////////////////////////////////////////////////////////////////////////

float2 f32x3_to_oct(float3 v)
{
	float2 p = v.xy / (abs(v.x) + abs(v.y) + abs(v.z));
	return (v.z <= 0) ? (1 - abs(p.yx)) * sign(p) : p; //-1~1
}

float3 oct_to_f32x3(float2 e)
{
	float3 v = float3(e.xy, 1 - abs(e.x) - abs(e.y));
	if(v.z < 0){ v.xy = (1 - abs(v.yx)) * sign(v.xy); }
	return normalize(v);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

uint f32x4_to_u8x4_unorm(float4 v)
{
	return 
		(uint(saturate(v.x) * 255 + 0.5) << (8 * 0)) | 
		(uint(saturate(v.y) * 255 + 0.5) << (8 * 1)) | 
		(uint(saturate(v.z) * 255 + 0.5) << (8 * 2)) | 
		(uint(saturate(v.w) * 255 + 0.5) << (8 * 3));
}

float4 u8x4_unorm_to_f32x4(uint v)
{
	return float4(
		float((v >> (8 * 0)) & 0xff), 
		float((v >> (8 * 1)) & 0xff), 
		float((v >> (8 * 2)) & 0xff), 
		float((v >> (8 * 3)) & 0xff)) / 255.0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

uint f32_to_u8_unorm(float v)
{
	return uint(saturate(v) * 255 + 0.5);
}

float u8_unorm_to_f32(uint v)
{
	return float(v & 0xff) / 255;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

uint f32_to_u16_unorm(float v)
{
	return uint(saturate(v) * 65535 + 0.5);
}

float u16_unorm_to_f32(uint v)
{
	return float(v & 0xffff) / 65535;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

uint f32_to_f16(float v)
{
	return f32tof16(v);
}

float f16_to_f32(uint v)
{
	return f16tof32(v);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

uint f32x2_to_f16x2(float2 v)
{
	uint2 u = f32tof16(v);
	return u.x | (u.y << 16);
}

float2 f16x2_to_f32x2(uint v)
{
	return f16tof32(uint2(v & 0xffff, v >> 16));
}

///////////////////////////////////////////////////////////////////////////////////////////////////

uint f32x2_to_u16x2_unorm(float2 v)
{
	return 
		uint(saturate(v.x) * 65535 + 0.5) | 
		uint(saturate(v.y) * 65535 + 0.5) << 16;
}

float2 u16x2_unorm_to_f32x2(uint v)
{
	return float2(
		float((v & 0x0000ffff)),
		float((v & 0xffff0000) >> 16)) / 65535;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

uint f32x2_to_u16x2_snorm(float2 v)
{
	return 
		(int(trunc(clamp(v.x, -1.0, 1.0) * 32767 + ((v.x >= 0) ? 0.5 : -0.5))) & 0xffff) |
		(int(trunc(clamp(v.y, -1.0, 1.0) * 32767 + ((v.y >= 0) ? 0.5 : -0.5))) << 16);
}

float2 u16x2_snorm_to_f32x2(const uint v)
{
	return float2(
		max((int(v << 16) >> 16) / 32767.0, -1.0),
		max((int(v) >> 16) / 32767.0, -1.0));
}

///////////////////////////////////////////////////////////////////////////////////////////////////

uint f32x4_to_r10g10b10a2(float4 v)
{
	return 
		(uint(saturate(v.x) * 1023 + 0.5) << (10 * 0)) |
		(uint(saturate(v.y) * 1023 + 0.5) << (10 * 1)) |
		(uint(saturate(v.z) * 1023 + 0.5) << (10 * 2)) |
		(uint(saturate(v.w) *    3 + 0.5) << (10 * 3));
}

float4 r10g10b10a2_to_f32x4(uint v)
{
	return float4(
		float((v >> (10 * 0)) & 0x3ff) / 1023, 
		float((v >> (10 * 1)) & 0x3ff) / 1023, 
		float((v >> (10 * 2)) & 0x3ff) / 1023, 
		float((v >> (10 * 3)) & 0x003) / 3);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

uint f32x3_to_r9g9b9e5(float3 col)
{
	static const float max_val = float(0x1FF << 7);
	static const float min_val = float(1.f / (1 << 16));

	col = clamp(col, 0, max_val);
	
	uint e = asuint(max(max(col.r, col.g), max(col.b, min_val)));
	e += 0x07804000;
	e &= 0x7F800000;

	col += asfloat(e);

	e <<= 4;
	e += 0x10000000;
	return e | (asuint(col.b) << 18) | (asuint(col.g) << 9) | (asuint(col.r) & 0x1ff);
}

float3 r9g9b9e5_to_f32x3(uint v)
{
	return ldexp(float3(
		(v >> (9 * 0)) & 0x1ff, 
		(v >> (9 * 1)) & 0x1ff, 
		(v >> (9 * 2)) & 0x1ff), int(v >> 27) - 24);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

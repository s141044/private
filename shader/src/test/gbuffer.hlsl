
#ifndef TEST_GBUFFER_HLSL
#define TEST_GBUFFER_HLSL

#include"../packing.hlsl"
#include"../global_constant.hlsl"

float4 encode_velocity(float3 screen_diff)
{
	return float4(screen_diff.xy * inv_screen_size, screen_diff.z, 0);
}

float3 decode_velocity(float4 val)
{
	return float3(val.xy * screen_size, val.z);
}

float4 encode_normal_roughness(float3 normal, float diffuse_roughness, float specular_roughness)
{
	//diffuse‚Ìƒ‰ƒtƒlƒX‚Íspecular‚É”ä‚×‚Ä‰e‹¿‚ª¬‚³‚¢‚Ì‚Å2bit‚Å...
	return float4(f32x3_to_oct(normal) * 0.5f + 0.5f, specular_roughness, diffuse_roughness);
}

void decode_normal_roughness(float4 val, out float3 normal, out float diffuse_roughness, out float specular_roughness)
{
	normal = oct_to_f32x3(val.xy * 2 - 1);
	diffuse_roughness = val.w;
	specular_roughness = val.z;
}

float4 encode_diffuse_color(float3 color)
{
	return float4(color, 0);
}

float3 decode_diffuse_color(float4 val)
{
	return val.xyz;
}

float4 encode_specular_color0(float3 color0)
{
	return float4(color0.xyz, 0);
}

float3 decode_specular_color0(float4 val)
{
	return val.xyz;
}

uint encode_subsurface_radius(float3 radius)
{
	return f32x3_to_r9g9b9e5(radius);
}

float3 decode_subsurface_radius(uint val)
{
	return r9g9b9e5_to_f32x3(val);
}

float encode_subsurface_weight(float weight)
{
	return weight;
}

float decode_subsurface_weight(float val)
{
	return val;
}

uint encode_subsurface_misc(float depth, float nwo)
{
	return uint(saturate(depth) * 0x00ffffff + 0.5f) | (uint(saturate(nwo) * 0xff + 0.5f) << 24);
}

void decode_subsurface_misc(uint val, out float depth, out float nwo)
{
	depth = float(val & 0x00ffffff) / 0x00ffffff;
	nwo = float((val >> 24) & 0xff) / 0xff;
}

#endif

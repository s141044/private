
#ifndef UTILITY_HLSL
#define UTILITY_HLSL

#include"packing.hlsl"
#include"global_constant.hlsl"

#define FLT_MAX asfloat(0x7F7FFFFF)

float3 world_to_screen(float3 pos)
{
	float4 tmp = mul(float4(pos, 1), view_proj_mat);
	tmp.xyz /= tmp.w;
	tmp.xy += float2(+1, -1);
	tmp.xy /= float2(+2, -2);
	tmp.xy *= screen_size;
	return tmp.xyz;
}

float3 proj_to_world(float3 pos)
{
	float4 tmp = mul(float4(pos, 1), inv_view_proj_mat);
	return tmp.xyz / tmp.w;
}

float3 screen_to_world(float2 pos, float z)
{
	pos *= inv_screen_size;
	pos *= float2(+2, -2);
	pos += float2(-1, +1);
	return proj_to_world(float3(pos, z));
}

uint2 calc_active_thread_count_and_offset()
{
	return uint2(
		WaveActiveCountBits(true),	//count
		WavePrefixCountBits(true));	//offset
}

float3 rec709_to_rec2020(float3 rec709)
{
	static const float3x3 m =
	{
		0.627402, 0.329292, 0.043306,
		0.069095, 0.919544, 0.011360,
		0.016394, 0.088028, 0.895578
	};
	return mul(m, rec709);
}

float3 bilinear_sample(Texture2D<uint> tex, sampler s, float2 uv)
{
	float2 size;
	tex.GetDimensions(size.x, size.y);
	float2 left_top = floor(uv * size - 0.5f);
	float2 bilinear_weight = uv * size - (left_top + 0.5f);
#if 0
	uint4 vals = tex.GatherRed(s, uv, 0).wzxy;
#else	
	uint4 vals;
	vals[0] = tex[left_top + int2(0,0)];
	vals[1] = tex[left_top + int2(1,0)];
	vals[2] = tex[left_top + int2(0,1)];
	vals[3] = tex[left_top + int2(1,1)];
#endif
	return 
		r9g9b9e5_to_f32x3(vals.x) * (1 - bilinear_weight.x) * (1 - bilinear_weight.y) + 
		r9g9b9e5_to_f32x3(vals.y) * bilinear_weight.x * (1 - bilinear_weight.y) + 
		r9g9b9e5_to_f32x3(vals.z) * (1 - bilinear_weight.x) * bilinear_weight.y + 
		r9g9b9e5_to_f32x3(vals.w) * bilinear_weight.x * bilinear_weight.y;
}

float3 extract_squared_scaling(float4x3 ltow)
{
	return float3(
		ltow[0][0] * ltow[0][0] + ltow[0][1] * ltow[0][1] + ltow[0][2] * ltow[0][2], 
		ltow[1][0] * ltow[1][0] + ltow[1][1] * ltow[1][1] + ltow[1][2] * ltow[1][2], 
		ltow[2][0] * ltow[2][0] + ltow[2][1] * ltow[2][1] + ltow[2][2] * ltow[2][2]);
}

float3 extract_scaling(float4x3 ltow)
{
	return sqrt(extract_squared_scaling(ltow));
}

void InterlockedAdd(RWByteAddressBuffer buf, uint offset, float v, out float original)
{
	while(true)
	{
		uint original_uint, old = buf.Load(offset);
		buf.InterlockedCompareExchange(offset, old, asuint(asfloat(old) + v), original_uint);
		if(original_uint == old){ original = asfloat(original_uint); break; }
	}
}

void InterlockedAdd(RWByteAddressBuffer buf, uint offset, float v)
{
	float original;
	InterlockedAdd(buf, offset, v, original);
}

#endif

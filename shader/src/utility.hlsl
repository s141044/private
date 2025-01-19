
#ifndef UTILITY_HLSL
#define UTILITY_HLSL

#include"global_constant.hlsl"

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

#endif

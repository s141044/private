
#include"global_constant.hlsl"

struct vs_input
{
	float3 position	: POSITION;
	float2 normal	: NORMAL;
	float2 tangent	: TANGENT;
	float2 uv		: TEXCOORD;
};

cbuffer draw_cb
{
	float4x4 ltow;
};

float4 draw_mask_vs(vs_input input) : SV_POSITION
{
	float3 position = mul(float4(input.position, 1), ltow).xyz;
	return mul(float4(position, 1), view_proj_mat);
}

float draw_mask_ps() : SV_TARGET
{
	return 1;
}

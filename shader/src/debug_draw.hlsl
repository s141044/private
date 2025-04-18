
#ifndef DEBUG_DRAW_HLSL
#define DEBUG_DRAW_HLSL

#include"global_constant.hlsl"

///////////////////////////////////////////////////////////////////////////////////////////////////

struct vs_input
{
	float3 position	: POSITION;
	float3 color	: COLOR;
};

struct vs_output
{
	float4 sv_position	: SV_POSITION;
	float3 color		: COLOR;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

vs_output debug_draw_vs(vs_input input)
{
	vs_output output;
	output.sv_position = mul(float4(input.position, 1), view_proj_mat);
	output.color = input.color;
	return output;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

float3 debug_draw_ps(vs_output input) : SV_TARGET
{
	return input.color;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

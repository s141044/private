
#ifndef GLOBAL_CONSTANT_HLSL
#define GLOBAL_CONSTANT_HLSL

///////////////////////////////////////////////////////////////////////////////////////////////////

cbuffer global_constant
{
	uint2		screen_size;
	float2		inv_screen_size;

	float3		camera_pos;
	uint		reserved0;

	float4x3	view_mat;
	float4x4	proj_mat;
	float4x4	view_proj_mat;
	float4x4	inv_view_proj_mat;

	uint		frame_count;
	uint3		reserved1;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

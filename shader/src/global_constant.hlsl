
#ifndef GLOBAL_CONSTANT_HLSL
#define GLOBAL_CONSTANT_HLSL

///////////////////////////////////////////////////////////////////////////////////////////////////

cbuffer camera_info
{
	uint2		screen_size;
	float2		inv_screen_size;

	float3		camera_pos;
	uint		reserved0;

	float4x3	view_mat;
	float4x4	proj_mat;
	float4x4	view_proj_mat;
	float4x4	inv_view_proj_mat;

	//å„Ç≈è¡Ç∑
	uint		frame_count;
	uint3		reserved1;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

cbuffer light_info
{
	float3	directional_light_direction; //ï®ëÃï\ñ Ç©ÇÁó£ÇÍÇÈï˚å¸
	float	directional_light_enable;
	float3	directional_light_power;
	float	directional_light_cos_max;
	float3	directional_light_axis_x;
	float	directional_light_sample_pdf;
	float3	directional_light_axis_y;
	float	directional_light_reserved;

	float	environment_light_log2_texel_size;
	uint	environment_light_presample_count;
	float	environment_light_power_scale;
	uint	environment_light_reserved;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

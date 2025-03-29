
#ifndef GLOBAL_CONSTANT_HLSL
#define GLOBAL_CONSTANT_HLSL

///////////////////////////////////////////////////////////////////////////////////////////////////

cbuffer scene_info
{
	uint2		screen_size;
	float2		inv_screen_size;
	float3		camera_pos;
	float		pixel_size;		//距離1におけるピクセルのサイズ
	float2		frustum_size;	//距離1におけるフラスタムのサイズ
	float2		scene_info_reserved0;
	float4x3	view_mat;
	float4x4	proj_mat;
	float4x3	inv_view_mat;
	float4x4	inv_proj_mat;
	float4x4	view_proj_mat;
	float4x4	inv_view_proj_mat;
	float4x4	prev_inv_view_proj_mat;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

cbuffer light_info
{
	float3	directional_light_direction;	//物体表面から離れる方向
	float	directional_light_enable;
	float3	directional_light_power;
	float	directional_light_cos_max;
	float3	directional_light_axis_x;
	float	directional_light_sample_pdf;	//1÷solid_angle
	float3	directional_light_axis_y;
	float	directional_light_solid_angle;

	float	environment_light_log2_texel_size;
	uint	environment_light_presample_count;
	float	environment_light_power_scale;
	uint	environment_light_reserved;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

cbuffer system_info
{
	float	delta_time;
	uint	frame_count;
	uint2	system_info_reserved0;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

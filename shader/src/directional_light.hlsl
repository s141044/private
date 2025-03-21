
#ifndef DIRECTIONAL_LIGHT_HLSL
#define DIRECTIONAL_LIGHT_HLSL

#include"sampling.hlsl"
#include"global_constant.hlsl"

bool exists_directional_light()
{
	return directional_light_enable;
}

float3 sample_directional_light(float u0, float u1)
{
	return sample_uniform_cone(directional_light_cos_max, directional_light_axis_x, directional_light_axis_y, directional_light_direction, u0, u1);
}

float sample_directional_light_pdf(float3 w)
{
	return directional_light_sample_pdf;
}

bool hit_directional_light(float3 w)
{
	return is_in_cone(w, directional_light_cos_max, directional_light_axis_x, directional_light_axis_y, directional_light_direction);
}

#endif
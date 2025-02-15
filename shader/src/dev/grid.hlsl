
#ifndef DEV_DRAW_HLSL
#define DEV_DRAW_HLSL

#include"../root_constant.hlsl"
#include"../global_constant.hlsl"

///////////////////////////////////////////////////////////////////////////////////////////////////

cbuffer grid_cbuffer
{
	float3	grid_color;
	float	grid_pad0;
	float3	grid_min;
	float	grid_pad1;
	float3	grid_max;
	float	grid_pad2;
	uint3	cell_count;
	float	cell_size;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

float4 draw_grid_vs(uint vertex_id : SV_VertexID, uint instance_id : SV_InstanceID) : SV_POSITION
{
	uint axis0 = root_constant;
	uint axis1 = axis0 + 1;
	uint axis2 = axis0 + 2;
	if(axis1 >= 3){ axis1 -= 3; }
	if(axis2 >= 3){ axis2 -= 3; }

	float3 pos;
	pos[axis0] = lerp(grid_min[axis0], grid_max[axis0], vertex_id & 1);
	pos[axis1] = grid_min[axis1] + (cell_size * (vertex_id / 2));
	pos[axis2] = grid_min[axis2] + (cell_size * instance_id);
	return mul(float4(pos, 1), view_proj_mat);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

float3 draw_grid_ps() : SV_TARGET
{
	return grid_color;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

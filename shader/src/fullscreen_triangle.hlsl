
#ifndef FULLSCREEN_TRIANGLE_HLSL
#define FULLSCREEN_TRIANGLE_HLSL

struct fullscreen_triangle_vs_output
{
	float4	sv_position : SV_POSITION;
	float2	uv			: TEXCOORD;
};

fullscreen_triangle_vs_output fullscreen_triangle_vs(uint vertex_id : SV_VertexID)
{
	fullscreen_triangle_vs_output output;
	output.uv.x = float(vertex_id & 1) * 2;
	output.uv.y = float(vertex_id & 2);
	output.sv_position.x = output.uv.x * +2 - 1;
	output.sv_position.y = output.uv.y * -2 + 1;
	output.sv_position.z = 0;
	output.sv_position.w = 1;
	return output;
}

#endif

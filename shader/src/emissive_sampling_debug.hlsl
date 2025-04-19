
#ifndef EMISSIVE_SAMPLING_DEBUG_HLSL
#define EMISSIVE_SAMPLING_DEBUG_HLSL

#include"math.hlsl"
#include"root_constant.hlsl"
#include"global_constant.hlsl"
#include"emissive_sampling.hlsl"

///////////////////////////////////////////////////////////////////////////////////////////////////

struct vs_input
{
	float3 position	: POSITION;
};

struct vs_output
{
	float4 sv_position	: SV_POSITION;
	float3 color		: COLOR;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

vs_output debug_draw_vs(vs_input input, uint instance_id : SV_InstanceID)
{
	emissive_sample s = decompress(emissive_sample_srv[instance_id]);

	float3 normal = s.normal, tangent, binormal;
	calc_orthonormal_basis(normal, tangent, binormal);
	
	float3 position = input.position * asfloat(root_constant);
	position = tangent * position.x + binormal * position.y + normal * position.z;
	position += s.position;

	vs_output output;
	output.sv_position = mul(float4(position, 1), view_proj_mat);
	output.color = s.L;
	return output;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

float3 debug_draw_ps(vs_output input) : SV_TARGET
{
	return input.color;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

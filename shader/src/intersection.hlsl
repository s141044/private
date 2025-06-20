
#ifndef INTERSECTION_HLSL
#define INTERSECTION_HLSL

#include"debug.hlsl"

struct intersection
{
	float3	position;
	float3	prev_position;
	float3	normal;
	float4	tangent;
	float3	geometry_normal;
	float2	uv;
	float3	dpdu;
	float3	dpdv;
	uint	material_handle;
	bool	is_front_face;
};

float3 get_binormal(intersection isect)
{
	return normalize(cross(isect.normal, isect.tangent.xyz) * isect.tangent.w);
}

#endif

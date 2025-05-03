
#ifndef PRISM_HLSL
#define PRISM_HLSL

///////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(CALC_PRISM_AABBS)

#include"mesh.hlsl"
#include"aabb.hlsl"
#include"bindless.hlsl"

cbuffer calc_prism_aabbs_cb
{
	uint	triangle_count;
	uint	base_vertex_location;
	uint	start_index_location;
	uint	bindless_geometry_handle;
	float	max_displacement;
	float3	padding;
};

RWStructuredBuffer<aabb> aabb_uav;

[numthreads(256, 1, 1)]
void calc_prism_aabbs(uint dtid : SV_DispatchThreadID)
{
	if(dtid >= triangle_count)
		return;

	geometry_desc geom = load_geometry_desc(bindless_geometry_handle);

	uint3 index = load_index(geom.ib_handle, start_index_location + 3 * dtid);
	index += base_vertex_location;

	ByteAddressBuffer vb0 = get_byteaddress_buffer(geom.vb_handles[0]);
	ByteAddressBuffer vb1 = get_byteaddress_buffer(geom.vb_handles[1]);

	float3 p0 = asfloat(vb0.Load3(geom.offsets[0] + (geom.strides[0] & 0xff) * index.x));
	float3 p1 = asfloat(vb0.Load3(geom.offsets[0] + (geom.strides[0] & 0xff) * index.y));
	float3 p2 = asfloat(vb0.Load3(geom.offsets[0] + (geom.strides[0] & 0xff) * index.z));

	float3 n0 = decode_normal(vb1.Load(geom.offsets[1] + ((geom.strides[0] >> 8) & 0xff) * index.x));
	float3 n1 = decode_normal(vb1.Load(geom.offsets[1] + ((geom.strides[0] >> 8) & 0xff) * index.y));
	float3 n2 = decode_normal(vb1.Load(geom.offsets[1] + ((geom.strides[0] >> 8) & 0xff) * index.z));

	aabb aabb = empty_aabb();
	aabb = merge(aabb, p0);
	aabb = merge(aabb, p1);
	aabb = merge(aabb, p2);
	aabb = merge(aabb, p0 + n0 * max_displacement);
	aabb = merge(aabb, p1 + n1 * max_displacement);
	aabb = merge(aabb, p2 + n2 * max_displacement);
	aabb_uav[start_index_location / 3 + dtid] = aabb;
}

#endif

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

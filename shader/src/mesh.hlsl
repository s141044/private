
#ifndef MESH_HLSL
#define MESH_HLSL

#include"packing.hlsl"
#include"bindless.hlsl"

///////////////////////////////////////////////////////////////////////////////////////////////////

struct geometry_desc
{
	uint num_vbs;
	uint ib_handle;
	uint vb_handles[8];
	uint offsets[8];
	uint strides[2];
};

///////////////////////////////////////////////////////////////////////////////////////////////////

geometry_desc load_geometry_desc(uint handle)
{
	ByteAddressBuffer buf = get_byteaddress_buffer(handle);
	uint4 data0 = buf.Load4(16 * 0);
	uint4 data1 = buf.Load4(16 * 1);
	uint4 data2 = buf.Load4(16 * 2);
	uint4 data3 = buf.Load4(16 * 3);
	uint4 data4 = buf.Load4(16 * 4);

	geometry_desc desc;
	desc.num_vbs = data0.x;
	desc.ib_handle = data0.y;
	desc.vb_handles[0] = data0.z;
	desc.vb_handles[1] = data0.w;
	desc.vb_handles[2] = data1.x;
	desc.vb_handles[3] = data1.y;
	desc.vb_handles[4] = data1.z;
	desc.vb_handles[5] = data1.w;
	desc.vb_handles[6] = data2.x;
	desc.vb_handles[7] = data2.y;
	desc.offsets[0] = data2.z;
	desc.offsets[1] = data2.w;
	desc.offsets[2] = data3.x;
	desc.offsets[3] = data3.y;
	desc.offsets[4] = data3.z;
	desc.offsets[5] = data3.w;
	desc.offsets[6] = data4.x;
	desc.offsets[7] = data4.y;
	desc.strides[0] = data4.z;
	desc.strides[1] = data4.w;
	return desc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

float3 decode_normal(float2 v)
{
	return oct_to_f32x3(v * 2 - 1);
}

float3 decode_normal(uint v)
{
	return decode_normal(u16x2_unorm_to_f32x2(v));
}

float4 decode_tangent(float2 v)
{
	return float4(oct_to_f32x3(abs(v) * 2 - 1), sign(v.x));
}

float4 decode_tangent(uint v)
{
	return decode_tangent(u16x2_snorm_to_f32x2(v));
}

//float2 decode_uv(float2 v)
//{
//	return sign(v) * pow(abs(v), 1.0f / 3.0f);
//}

//float2 decode_uv(uint v)
//{
//	return decode_uv(f16x2_to_f32x2(v));
//}

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

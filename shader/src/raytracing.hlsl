
#ifndef RAYTRACING_HLSL
#define RAYTRACING_HLSL

#include"mesh.hlsl"

#define INSTANCE_MASK_DEFAULT		1
#define INSTANCE_MASK_SUBSURFACE	2
#define INSTANCE_MASK_ALL			~0

struct ray
{
	float3	origin;
	float3	direction;
	float	tmin;
	float	tmax;
};

struct hit_info
{
	float	ray_t;
	float2	barycentrics; //p = p0 + (p1 - p0) * bx + (p2 - p0) * by
	uint	instance_id;
	uint	instance_index;
	uint	geometry_index;
	uint	primitive_index;
	bool	is_front_face;
};

struct bindless_instance_desc
{
	uint	bindless_geometry_handle;
	uint	bindless_material_handle;
	uint	start_index_location;
	uint	base_vertex_location;
};

struct raytracing_instance_desc
{
	float4x3	transform;
	uint		id		: 24;
	uint		mask	: 8;
	uint		unused	: 24;
	uint		flags	: 8;
	uint64_t	blas_address;
};

RaytracingAccelerationStructure				AS : register(s0, space1);
StructuredBuffer<bindless_instance_desc>	bindless_instance_descs;   //instance_id+geometry_indexでアクセス
StructuredBuffer<raytracing_instance_desc>	raytracing_instance_descs; //instance_indexでアクセス

//void closest_hit_shader(ray r, inout ray_payload payload, hit_info info);
//bool any_hit_shader(ray r, inout ray_payload payload, hit_info info);
//void miss_shader(ray r, inout ray_payload payload);

//自分の理解がよくないのか,反時計回りが表面の指定をしてるが逆の判定になる...
#define FLIP_FRONT_FACE true

#define trace_ray(ray, ray_payload, ray_flags, instance_mask, closest_hit_shader, any_hit_shader, miss_shader)	\
{																												\
	RayDesc RAY_DESC;																							\
	RAY_DESC.Origin = ray.origin;																				\
	RAY_DESC.Direction = ray.direction;																			\
	RAY_DESC.TMin = ray.tmin;																					\
	RAY_DESC.TMax = ray.tmax;																					\
																												\
	RayQuery<RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES | ray_flags> QUERY;											\
	QUERY.TraceRayInline(AS, RAY_FLAG_NONE, instance_mask, RAY_DESC);											\
																												\
	while(QUERY.Proceed())																						\
	{																											\
		hit_info INFO;																							\
		INFO.ray_t = QUERY.CandidateTriangleRayT();																\
		INFO.barycentrics = QUERY.CandidateTriangleBarycentrics();												\
		INFO.instance_id = QUERY.CandidateInstanceID();															\
		INFO.instance_index = QUERY.CandidateInstanceIndex();													\
		INFO.geometry_index = QUERY.CandidateGeometryIndex();													\
		INFO.primitive_index = QUERY.CandidatePrimitiveIndex();													\
		INFO.is_front_face = QUERY.CandidateTriangleFrontFace() != FLIP_FRONT_FACE;								\
		if(any_hit_shader(ray, ray_payload, INFO))																\
			QUERY.CommitNonOpaqueTriangleHit();																	\
	}																											\
	switch(QUERY.CommittedStatus())																				\
	{																											\
	case COMMITTED_TRIANGLE_HIT:																				\
	{																											\
		hit_info INFO;																							\
		INFO.ray_t = QUERY.CommittedRayT();																		\
		INFO.barycentrics = QUERY.CommittedTriangleBarycentrics();												\
		INFO.instance_id = QUERY.CommittedInstanceID();															\
		INFO.instance_index = QUERY.CommittedInstanceIndex();													\
		INFO.geometry_index = QUERY.CommittedGeometryIndex();													\
		INFO.primitive_index = QUERY.CommittedPrimitiveIndex();													\
		INFO.is_front_face = QUERY.CommittedTriangleFrontFace() != FLIP_FRONT_FACE;								\
		closest_hit_shader(ray, ray_payload, INFO);																\
		break;																									\
	}																											\
	case COMMITTED_NOTHING:																						\
	{																											\
		miss_shader(ray, ray_payload);																			\
		break;																									\
	}}																											\
}

struct intersection
{
	float3	position;
	float3	prev_position;
	float3	normal;
	float4	tangent;
	float3	geometry_normal;
	float2	uv;
	uint	material_handle;
	bool	is_front_face;
};

float3 get_binormal(intersection isect)
{
	return normalize(cross(isect.normal, isect.tangent.xyz) * isect.tangent.w);
}

uint3 load_index(uint ib_handle, uint offset)
{
	offset *= sizeof(uint16_t);
	
	ByteAddressBuffer ib = get_byteaddress_buffer(ib_handle);
	uint2 index = ib.Load2(offset & 0xfffffffc);
	if(offset & 2)
		return uint3(index.x >> 16, index.y & 0xffff, index.y >> 16);
	else
		return uint3(index.x & 0xffff, index.x >> 16, index.y & 0xffff);
}

intersection get_intersection(uint instance_id, uint geometry_index, uint primitive_index, float2 b12, bool is_front_face)
{
	float3 b;
	b.yz = b12;
	b.x = 1 - b.y - b.z;

	bindless_instance_desc instance = bindless_instance_descs[instance_id + geometry_index];
	geometry_desc geom = load_geometry_desc(instance.bindless_geometry_handle);

	uint3 index = load_index(geom.ib_handle, instance.start_index_location + 3 * primitive_index);
	index += instance.base_vertex_location;

	intersection isect;
	isect.is_front_face = is_front_face;
	isect.material_handle = instance.bindless_material_handle;

	ByteAddressBuffer vb0 = get_byteaddress_buffer(geom.vb_handles[0]);
	ByteAddressBuffer vb1 = get_byteaddress_buffer(geom.vb_handles[1]);
	ByteAddressBuffer vb2 = get_byteaddress_buffer(geom.vb_handles[2]);

	float3 p0 = asfloat(vb0.Load3(geom.offsets[0] + (geom.strides[0] & 0xff) * index.x));
	float3 p1 = asfloat(vb0.Load3(geom.offsets[0] + (geom.strides[0] & 0xff) * index.y));
	float3 p2 = asfloat(vb0.Load3(geom.offsets[0] + (geom.strides[0] & 0xff) * index.z));
	isect.position = p0 * b[0] + p1 * b[1] + p2 * b[2];
	isect.geometry_normal = normalize(cross(p1 - p0, p2 - p0));

	if(geom.vb_handles[4] == invalid_bindless_handle)
		isect.prev_position = isect.position; //本当はPrevのワールド変換行列の考慮が必要
	else
	{
		float3 p0 = asfloat(vb0.Load3(geom.offsets[4] + (geom.strides[1] & 0xff) * index.x));
		float3 p1 = asfloat(vb0.Load3(geom.offsets[4] + (geom.strides[1] & 0xff) * index.y));
		float3 p2 = asfloat(vb0.Load3(geom.offsets[4] + (geom.strides[1] & 0xff) * index.z));
		isect.prev_position = p0 * b[0] + p1 * b[1] + p2 * b[2];
	}

	float3 n0 = decode_normal(vb1.Load(geom.offsets[1] + ((geom.strides[0] >> 8) & 0xff) * index.x));
	float3 n1 = decode_normal(vb1.Load(geom.offsets[1] + ((geom.strides[0] >> 8) & 0xff) * index.y));
	float3 n2 = decode_normal(vb1.Load(geom.offsets[1] + ((geom.strides[0] >> 8) & 0xff) * index.z));
	isect.normal = normalize(n0 * b[0] + n1 * b[1] + n2 * b[2]);

	//tangentとuvは連続して配置される
	uint3 tuv0 = vb2.Load3(geom.offsets[2] + ((geom.strides[0] >> 16) & 0xff) * index.x);
	uint3 tuv1 = vb2.Load3(geom.offsets[2] + ((geom.strides[0] >> 16) & 0xff) * index.y);
	uint3 tuv2 = vb2.Load3(geom.offsets[2] + ((geom.strides[0] >> 16) & 0xff) * index.z);
	
	float4 t0 = decode_tangent(tuv0.x);
	float4 t1 = decode_tangent(tuv1.x);
	float4 t2 = decode_tangent(tuv2.x);
	isect.tangent = float4(normalize(t0.xyz * b[0] + t0.xyz * b[1] + t0.xyz * b[2]), t0.w);

	float2 uv0 = asfloat(tuv0.yz);
	float2 uv1 = asfloat(tuv1.yz);
	float2 uv2 = asfloat(tuv2.yz);
	isect.uv = uv0 * b[0] + uv1 * b[1] + uv2 * b[2];

	if(!is_front_face)
	{
		isect.normal = -isect.normal;
		isect.tangent = -isect.tangent;
		isect.geometry_normal = -isect.geometry_normal;
	}
	return isect;
}

#endif

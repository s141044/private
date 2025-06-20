
#ifndef RAYTRACING_HLSL
#define RAYTRACING_HLSL

#include"debug.hlsl"

#define INSTANCE_MASK_DEFAULT		1
#define INSTANCE_MASK_SUBSURFACE	2
#define INSTANCE_MASK_ALL			~0

#define HIT_TYPE_TRIANGLE		0
#define HIT_TYPE_DISPLACEMENT	1

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

#include"mesh.hlsl"
#include"intersection.hlsl"
#include"raytracing_displacement.hlsl"

//void closest_hit_shader(ray r, inout ray_payload payload, hit_info info);
//bool any_hit_shader(ray r, inout ray_payload payload, hit_info info);
//void miss_shader(ray r, inout ray_payload payload);

//自分の理解がよくないのか,反時計回りが表面の指定をしてるが逆の判定になる...
#define FLIP_FRONT_FACE true

#define trace_ray(ray, ray_payload, ray_flags, instance_mask, closest_hit_shader, any_hit_shader, procedural_closest_hit_shader, procedural_any_hit_shader, miss_shader)	\
{																																											\
	RayDesc RAY_DESC;																																						\
	RAY_DESC.Origin = ray.origin;																																			\
	RAY_DESC.Direction = ray.direction;																																		\
	RAY_DESC.TMin = ray.tmin;																																				\
	RAY_DESC.TMax = ray.tmax;																																				\
																																											\
	RayQuery<ray_flags> QUERY;																																				\
	QUERY.TraceRayInline(AS, RAY_FLAG_NONE, instance_mask, RAY_DESC);																										\
																																											\
	while(QUERY.Proceed())																																					\
	{																																										\
		switch(QUERY.CandidateType())																																		\
		{																																									\
		case CANDIDATE_NON_OPAQUE_TRIANGLE:																																	\
		{																																									\
			hit_info INFO;																																					\
			INFO.ray_t = QUERY.CandidateTriangleRayT();																														\
			INFO.barycentrics = QUERY.CandidateTriangleBarycentrics();																										\
			INFO.instance_id = QUERY.CandidateInstanceID();																													\
			INFO.instance_index = QUERY.CandidateInstanceIndex();																											\
			INFO.geometry_index = QUERY.CandidateGeometryIndex();																											\
			INFO.primitive_index = QUERY.CandidatePrimitiveIndex();																											\
			INFO.is_front_face = QUERY.CandidateTriangleFrontFace() != FLIP_FRONT_FACE;																						\
			if(any_hit_shader(ray, ray_payload, INFO))																														\
				QUERY.CommitNonOpaqueTriangleHit();																															\
			break;																																							\
		}																																									\
		case CANDIDATE_PROCEDURAL_PRIMITIVE:																																\
		{																																									\
			hit_info INFO;																																					\
			INFO.instance_id = QUERY.CandidateInstanceID();																													\
			INFO.instance_index = QUERY.CandidateInstanceIndex();																											\
			INFO.geometry_index = QUERY.CandidateGeometryIndex();																											\
			INFO.primitive_index = QUERY.CandidatePrimitiveIndex();																											\
			INFO.ray_t = QUERY.CommittedRayT();																																\
			if(procedural_any_hit_shader(ray, ray_payload, INFO))																											\
				QUERY.CommitProceduralPrimitiveHit(INFO.ray_t);																												\
			break;																																							\
		}}																																									\
	}																																										\
	switch(QUERY.CommittedStatus())																																			\
	{																																										\
	case COMMITTED_TRIANGLE_HIT:																																			\
	{																																										\
		hit_info INFO;																																						\
		INFO.ray_t = QUERY.CommittedRayT();																																	\
		INFO.barycentrics = QUERY.CommittedTriangleBarycentrics();																											\
		INFO.instance_id = QUERY.CommittedInstanceID();																														\
		INFO.instance_index = QUERY.CommittedInstanceIndex();																												\
		INFO.geometry_index = QUERY.CommittedGeometryIndex();																												\
		INFO.primitive_index = QUERY.CommittedPrimitiveIndex();																												\
		INFO.is_front_face = QUERY.CommittedTriangleFrontFace() != FLIP_FRONT_FACE;																							\
		closest_hit_shader(ray, ray_payload, INFO);																															\
		break;																																								\
	}																																										\
	case COMMITTED_PROCEDURAL_PRIMITIVE_HIT:																																\
	{																																										\
		hit_info INFO;																																						\
		INFO.ray_t = QUERY.CommittedRayT();																																	\
		INFO.instance_id = QUERY.CommittedInstanceID();																														\
		INFO.instance_index = QUERY.CommittedInstanceIndex();																												\
		INFO.geometry_index = QUERY.CommittedGeometryIndex();																												\
		INFO.primitive_index = QUERY.CommittedPrimitiveIndex();																												\
		procedural_closest_hit_shader(ray, ray_payload, INFO);																												\
		break;																																								\
	}																																										\
	case COMMITTED_NOTHING:																																					\
	{																																										\
		miss_shader(ray, ray_payload);																																		\
		break;																																								\
	}}																																										\
}

intersection get_intersection(uint instance_index, uint instance_id, uint geometry_index, uint primitive_index, float2 b12, bool is_front_face, uint hit_type = HIT_TYPE_TRIANGLE, uint2 dtid = 0)
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

	float3 e1 = p1 - p0;
	float3 e2 = p2 - p0;
	isect.geometry_normal = normalize(cross(e1, e2));

	if(geom.vb_handles[4] != invalid_bindless_handle)
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

	float2 euv1 = uv1 - uv0;
	float2 euv2 = uv2 - uv0;
	float inv_det = 1 / (euv1.x * euv2.y - euv1.y * euv2.x);
	isect.dpdu = (e1 * euv2.y - e2 * euv1.y) * inv_det;
	isect.dpdv = (e2 * euv1.x - e1 * euv2.x) * inv_det;

	if(hit_type == HIT_TYPE_DISPLACEMENT)
	{
		displacement_params params = load_displacement_params(instance.bindless_material_handle);

		float3 p = p0 * b[0] + p1 * b[1] + p2 * b[2];
		float3 n = n0 * b[0] + n1 * b[1] + n2 * b[2];
		isect.position = p + n * get_displacement(params, isect.uv);

		float delta_b = 0.001;
		float3 pb = p0 * (b[0] + delta_b) + p1 * b[1] + p2 * (b[2] - delta_b);
		float3 pc = p0 * b[0] + p1 * (b[1] + delta_b) + p2 * (b[2] - delta_b);
		float2 uvb = uv0 * (b[0] + delta_b) + uv1 * b[1] + uv2 * (b[2] - delta_b);
		float2 uvc = uv0 * b[0] + uv1 * (b[1] + delta_b) + uv2 * (b[2] - delta_b);
		float3 nb = n0 * (b[0] + delta_b) + n1 * b[1] + n2 * (b[2] - delta_b);
		float3 nc = n0 * b[0] + n1 * (b[1] + delta_b) + n2 * (b[2] - delta_b);
		pb += nb * get_displacement(params, uvb);
		pc += nc * get_displacement(params, uvc);
		//pb += n * get_displacement(params, uvb);
		//pc += n * get_displacement(params, uvc);

		float3 ng = normalize(cross(pc - isect.position, pb - isect.position));
		if(dot(ng, isect.geometry_normal) < 0)
			ng = -ng;

		//isect.normal = ng;
		isect.normal = ng - isect.geometry_normal + isect.normal;
		isect.normal = normalize(isect.normal);
		isect.geometry_normal = ng;
	}

	if(!is_front_face)
	{
		isect.normal = -isect.normal;
		isect.tangent = -isect.tangent;
		isect.geometry_normal = -isect.geometry_normal;
	}

	//スタティックはワールド空間に変換が必要
	if(geom.vb_handles[4] == invalid_bindless_handle)
	{
		float4x3 ltow = raytracing_instance_descs[instance_index].transform;
		isect.position = mul(float4(isect.position, 1), ltow);
		isect.prev_position = isect.position; //スタティックは動かない
		isect.normal = normalize(mul(isect.normal, (float3x3)ltow)); //等方スケーリングを仮定
		isect.geometry_normal = normalize(mul(isect.geometry_normal, (float3x3)ltow));
		isect.tangent.xyz = normalize(mul(isect.tangent.xyz, (float3x3)ltow));
		isect.dpdu = mul(float4(isect.dpdu, 0), ltow);
		isect.dpdv = mul(float4(isect.dpdv, 0), ltow);
	}
	return isect;
}

#endif

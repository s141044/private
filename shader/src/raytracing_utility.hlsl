
#ifndef RAYTRACING_UTILITY_HLSL
#define RAYTRACING_UTILITY_HLSL

struct ray_payload
{
	float	ray_t;
	uint	instance_id;
	uint	instance_index;
	uint	primitive_index;
	float2	barycentrics;
	bool	is_front_face;
};

uint4 encode_payload(uint instance_id, uint instance_index, uint geometry_index, uint primitive_index, bool is_front_face, float2 barycentrics, float ray_t)
{
	uint4 ret;
	ret.x = (instance_id + geometry_index) | instance_index;
	ret.y = primitive_index;
	ret.z = f32x2_to_u16x2_unorm(barycentrics);
	ret.w = asuint(ray_t) | (is_front_face ? 0x80000000 : 0);
	return ret;
}

ray_payload decode_payload(uint4 data)
{
	ray_payload ret;
	ret.instance_id = data.x & 0xffff;
	ret.instance_index = data.x >> 16;
	ret.primitive_index = data.y;
	ret.barycentrics = u16x2_unorm_to_f32x2(data.z);
	ret.ray_t = asfloat(data.w & 0x7fffffff);
	ret.is_front_face = data.w & 0x80000000;
	return ret;
}

intersection get_intersection(ray_payload payload)
{
	return get_intersection(payload.instance_id, 0, payload.primitive_index, payload.barycentrics, payload.is_front_face);
}

void ch_default(ray r, inout uint4 payload, hit_info info)
{
	payload = encode_payload(info.instance_id, info.instance_index, info.geometry_index, info.primitive_index, info.is_front_face, info.barycentrics, info.ray_t);
}

bool ah_default(ray r, inout uint4 payload, hit_info info)
{
	return true;
}

void ms_default(ray r, inout uint4 payload)
{
	payload.w = 0xffffffff;
}

bool is_hit(uint4 payload)
{
	return (payload.w != 0xffffffff);
}

bool find_closest(ray ray, out ray_payload payload)
{
	uint4 info;
	trace_ray(ray, info, RAY_FLAG_NONE, INSTANCE_MASK_ALL, ch_default, ah_default, ms_default);
	if(!is_hit(info))
		return false;

	payload = decode_payload(info);
	return true;
}

bool is_occluded(float3 o, float3 d, float dist)
{
	ray ray;
	ray.origin = o;
	ray.direction = d;
	ray.tmin = 1e-3f; //‚Ç‚¤‚·‚Á‚©‚È‚Ÿ
	ray.tmax = dist - 1e-3f;

	uint4 info = 0;
	trace_ray(ray, info, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER, INSTANCE_MASK_ALL, ch_default, ah_default, ms_default);
	return is_hit(info);
}

bool is_occluded(float3 x, float3 y)
{
	float3 d = y - x;
	float dist = length(d);
	return is_occluded(x, d / dist, dist);
}

#endif

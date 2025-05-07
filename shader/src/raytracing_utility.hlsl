
#ifndef RAYTRACING_UTILITY_HLSL
#define RAYTRACING_UTILITY_HLSL

#include"raytracing.hlsl"

struct ray_payload
{
	float	ray_t;
	uint	instance_id;
	uint	instance_index;
	uint	primitive_index;
	float2	barycentrics;
	bool	is_front_face;
	uint	hit_type;
};

uint4 encode_payload(uint instance_id, uint instance_index, uint geometry_index, uint primitive_index, bool is_front_face, float2 barycentrics, float ray_t, uint hit_type)
{
	uint4 ret;
	ret.x = (instance_id + geometry_index) | instance_index;
	ret.y = primitive_index | (hit_type << 31);
	ret.z = f32x2_to_u16x2_unorm(barycentrics);
	ret.w = asuint(ray_t) | (is_front_face ? 0x80000000 : 0);
	return ret;
}

uint4 encode_payload(ray_payload payload)
{
	return encode_payload(payload.instance_id, payload.instance_index, 0, payload.primitive_index, payload.is_front_face, payload.barycentrics, payload.ray_t, payload.hit_type);
}

ray_payload decode_payload(uint4 data)
{
	ray_payload ret;
	ret.instance_id = data.x & 0xffff;
	ret.instance_index = data.x >> 16;
	ret.primitive_index = data.y & 0x7fffffff;
	ret.barycentrics = u16x2_unorm_to_f32x2(data.z);
	ret.ray_t = asfloat(data.w & 0x7fffffff);
	ret.is_front_face = data.w & 0x80000000;
	ret.hit_type = data.y >> 31;
	return ret;
}

intersection get_intersection(ray_payload payload, uint2 dtid = 0)
{
	return get_intersection(payload.instance_index, payload.instance_id, 0, payload.primitive_index, payload.barycentrics, payload.is_front_face, payload.hit_type, dtid);
}

void ch_default(ray r, inout uint4 payload, hit_info info)
{
	payload = encode_payload(info.instance_id, info.instance_index, info.geometry_index, info.primitive_index, info.is_front_face, info.barycentrics, info.ray_t, HIT_TYPE_TRIANGLE);
}

bool ah_default(ray r, inout uint4 payload, hit_info info)
{
	//TODO: アルファマップ対応
	return true;
}

void ms_default(ray r, inout uint4 payload)
{
	payload.w = 0xffffffff;
}

void ch_displacement(ray ray, inout uint4 payload, hit_info info)
{
	payload = encode_payload(info.instance_id, info.instance_index, info.geometry_index, info.primitive_index, payload.z, asfloat(payload.xy), info.ray_t, HIT_TYPE_DISPLACEMENT);
}

void ch_displacement_disable(ray ray, inout uint4 payload, hit_info info)
{
	payload = encode_payload(info.instance_id, info.instance_index, info.geometry_index, info.primitive_index, payload.z, asfloat(payload.xy), info.ray_t, HIT_TYPE_TRIANGLE);
}

bool ah_displacement(ray ray, inout uint4 payload, inout hit_info info)
{
	bool is_front_face;
	float2 barycentrics;
	if(!displacement_intersection(info.instance_index, info.instance_id, info.geometry_index, info.primitive_index, ray.origin, ray.direction, info.ray_t, ray.tmin, info.ray_t, barycentrics, is_front_face, uint2(payload.w & 0xffff, payload.w >> 16)))
		return false;

	payload.xy = asuint(barycentrics);
	payload.z = is_front_face;
	return true;
}

bool ah_displacement_disable(ray ray, inout uint4 payload, inout hit_info info)
{
	bool is_front_face;
	float2 barycentrics;
	if(!triangle_intersection(info.instance_index, info.instance_id, info.geometry_index, info.primitive_index, ray.origin, ray.direction, info.ray_t, ray.tmin, info.ray_t, barycentrics, is_front_face))
		return false;

	payload.xy = asuint(barycentrics);
	payload.z = is_front_face;
	return true;
}

bool is_hit(uint4 payload)
{
	return (payload.w != 0xffffffff);
}

bool find_closest(ray ray, out ray_payload payload, bool enable_displacement = false, uint2 dtid = 0)
{
	uint4 info;
	info.w = dtid.x | (dtid.y << 16);
	if(enable_displacement)
	{
		trace_ray(ray, info, RAY_FLAG_NONE, INSTANCE_MASK_ALL, ch_default, ah_default, ch_displacement, ah_displacement, ms_default);
	}
	else
	{
		trace_ray(ray, info, RAY_FLAG_NONE, INSTANCE_MASK_ALL, ch_default, ah_default, ch_displacement_disable, ah_displacement_disable, ms_default);
	}
	if(!is_hit(info))
		return false;

	payload = decode_payload(info);
	return true;
}

bool is_occluded(float3 o, float3 d, float dist, bool enable_displacement = false)
{
	ray ray;
	ray.origin = o;
	ray.direction = d;
	ray.tmin = 1e-3f; //どうすっかなぁ
	ray.tmax = dist - 1e-3f;

	uint4 info = 0;
	if(enable_displacement)
	{
		trace_ray(ray, info, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER, INSTANCE_MASK_ALL, ch_default, ah_default, ch_displacement, ah_displacement, ms_default);
	}
	else
	{
		trace_ray(ray, info, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER, INSTANCE_MASK_ALL, ch_default, ah_default, ch_displacement_disable, ah_displacement_disable, ms_default);
	}
	return is_hit(info);
}

bool is_occluded(float3 x, float3 y, bool enable_displacement = true)
{
	float3 d = y - x;
	float dist = length(d);
	return is_occluded(x, d / dist, dist, enable_displacement);
}

#endif

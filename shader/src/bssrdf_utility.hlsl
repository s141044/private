
#ifndef BSSRDF_UTILITY_HLSL
#define BSSRDF_UTILITY_HLSL

#include"debug.hlsl"
#include"bssrdf.hlsl"
#include"raytracing_utility.hlsl"

struct sss_payload : ray_payload
{
	uint hit_count;
};

sss_payload decode_sss_payload(uint4 data)
{
	sss_payload ret;
	ret.instance_id = data.x & 0xffff;
	ret.instance_index = data.x >> 16;
	ret.primitive_index = data.y & 0x7fffffff;
	ret.barycentrics = u16x2_unorm_to_f32x2(data.z);
	ret.hit_type = data.y >> 31;
	ret.hit_count = data.w >> 24;
	ret.ray_t = 0;
	ret.is_front_face = true;
	return ret;
}

void ch_sss(ray r, inout uint4 payload, hit_info info)
{
}

bool ah_sss(ray r, inout uint4 payload, hit_info info, uint hit_type = HIT_TYPE_TRIANGLE)
{
	uint hit_count = (payload.w >> 24) + 1;
	float u = (payload.w & 0xffffff) / float(0xffffff);
	float pmf = 1 / float(hit_count);
	if(u < pmf)
	{
		u /= pmf;

		payload.x = (info.instance_id + info.geometry_index) | (info.instance_index << 16);
		payload.y = info.primitive_index | (hit_type << 31);
		payload.z = f32x2_to_u16x2_unorm(info.barycentrics);
	}
	else
	{
		u = (u - pmf) / (1 - pmf);
	}
	payload.w = uint(u * 0xffffff + 0.5) | (hit_count << 24);
	return false;
}

void ms_sss(ray r, inout uint4 payload)
{
}

void ch_displacement_sss(ray ray, inout uint4 payload, inout hit_info info)
{
}

bool ah_displacement_sss(ray ray, inout uint4 payload, inout hit_info info)
{
	bool is_front_face;
	if(triangle_intersection(info.instance_index, info.instance_id, info.geometry_index, info.primitive_index, ray.origin, ray.direction, info.ray_t, ray.tmin, info.ray_t, info.barycentrics, is_front_face))
	{
		ah_sss(ray, payload, info);
	}
	return false;
}

bool find_hit(ray ray, float u, out sss_payload payload, uint2 dtid = 0)
{
	uint4 info;
	info.w = uint(u * 0xffffff + 0.5);
	trace_ray(ray, info, RAY_FLAG_FORCE_NON_OPAQUE | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER, INSTANCE_MASK_SUBSURFACE, ch_sss, ah_sss, ch_displacement_sss, ah_displacement_sss, ms_sss);
	if((info.w >> 24) == 0)
		return false;

	payload = decode_sss_payload(info);
	return true;
}

intersection get_intersection(sss_payload payload)
{
	return get_intersection(payload.instance_index, payload.instance_id, 0, payload.primitive_index, payload.barycentrics, true);
}

#endif


#include"raytracing.hlsl"
#include"root_constant.hlsl"
#include"global_constant.hlsl"

RWByteAddressBuffer result;

void ch_default(ray r, inout uint payload, hit_info info)
{
	payload = info.instance_index;
}

bool ah_default(ray r, inout uint payload, hit_info info)
{
	return true;
}

void ms_default(ray r, inout uint payload)
{
	payload = 0xffffffff;
}

[numthreads(1, 1, 1)]
void pick()
{
	float4 pos;
	pos.x = root_constant & 0xffff;
	pos.y = root_constant >> 16;
	pos.x = (pos.x + 0.5f) * inv_screen_size.x;
	pos.y = (pos.y + 0.5f) * inv_screen_size.y;
	pos.xy *= float2(+2, -2);
	pos.xy += float2(-1, +1);
	pos.z = pos.w = 1;
	pos = mul(pos, inv_view_proj_mat);
	pos.xyz /= pos.w;

	float3 dir = normalize(pos.xyz - camera_pos);
	
	ray ray;
	ray.origin = camera_pos;
	ray.direction = dir;
	ray.tmin = 0;
	ray.tmax = 1000;

	uint index;
	trace_ray(ray, index, RAY_FLAG_NONE, INSTANCE_MASK_ALL, ch_default, ah_default, ms_default);
	result.Store(0, index);
}

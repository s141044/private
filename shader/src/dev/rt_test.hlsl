
#include"../utility.hlsl"
#include"../packing.hlsl"
#include"../raytracing.hlsl"
#include"../raytracing_utility.hlsl"
#include"../global_constant.hlsl"
#include"../root_constant.hlsl"

Texture2D<uint>		id_srv;
RWTexture2D<uint>	id_uav;
RWTexture2D<uint>	color_uav;
RWTexture2D<float>	depth_uav;

[numthreads(8, 4, 1)]
void rt_test(uint2 dtid : SV_DispatchThreadID)
{
	if(any(dtid >= screen_size))
		return;

	float3 pos = screen_to_world(dtid + 0.5f, 1);
	float3 dir = normalize(pos - camera_pos);
	
	ray ray;
	ray.origin = camera_pos;
	ray.direction = dir;
	ray.tmin = 0;
	ray.tmax = 1000;

	ray_payload payload;
	if(find_closest(ray, payload))
	{
		float3 wo = -ray.direction;
		float3 pos = ray.origin + ray.direction * payload.ray_t;
		intersection isect = get_intersection(payload);

		depth_uav[dtid] = world_to_screen(pos).z;

		float3 col;
#if defined(VIEW_DEFAULT)
		col = max(dot(isect.geometry_normal, wo), 0.2f);
		id_uav[dtid] = payload.primitive_index | (payload.instance_index << 16);
#elif defined(VIEW_NORMAL)
		col = isect.normal * 0.5f + 0.5f;
#elif defined(VIEW_GEOMETRY_NORMAL)
		col = isect.geometry_normal * 0.5f + 0.5f;
#elif defined(VIEW_TANGENT)
		col = isect.tangent.xyz * 0.5f + 0.5f;
#elif defined(VIEW_BINORMAL)
		col = normalize(cross(isect.normal, isect.tangent.xyz)) * isect.tangent.w * 0.5f + 0.5f;
#elif defined(VIEW_UV)
		col = float3(isect.uv, 0);
#endif
		color_uav[dtid] = f32x3_to_r9g9b9e5(col);
	}
}

[numthreads(16, 16, 1)]
void draw_wire(uint2 dtid : SV_DispatchThreadID)
{
	if(any(dtid >= screen_size))
		return;

	bool is_edge = false;
	uint id = id_srv[dtid];
	if(!is_edge && (dtid.x > 0)){ is_edge = (id != id_srv[uint2(dtid.x - 1, dtid.y)]); }
	if(!is_edge && (dtid.y > 0)){ is_edge = (id != id_srv[uint2(dtid.x, dtid.y - 1)]); }
	if(!is_edge && (dtid.x + 1 < screen_size.x)){ is_edge = (id != id_srv[uint2(dtid.x + 1, dtid.y)]); }
	if(!is_edge && (dtid.y + 1 < screen_size.y)){ is_edge = (id != id_srv[uint2(dtid.x, dtid.y + 1)]); }
	
	if(is_edge)
	{
		float3 col = r9g9b9e5_to_f32x3(color_uav[dtid]);
		color_uav[dtid] = f32x3_to_r9g9b9e5(col * 0.5f);
	}
}

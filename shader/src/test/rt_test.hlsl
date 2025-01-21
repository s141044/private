
#include"../random.hlsl"
#include"../raytracing.hlsl"
#include"../static_sampler.hlsl"
#include"../global_constant.hlsl"
#include"../environment.hlsl"
#include"../root_constant.hlsl"
#include"../bsdf/lambert.hlsl"
#include"../material/standard.hlsl"

#include"../bindless.hlsl"

TextureCube<float3>	env_cube;
Texture2D<float3>	env_panorama;

RWTexture2D<float3>	result;
RWTexture2D<float4>	accum;

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

[numthreads(8, 4, 1)]
void cs_main(uint2 dtid : SV_DispatchThreadID)
{
	if(any(dtid >= screen_size))
		return;
	
	debug_uav0[dtid] = 0;
	debug_uav1[dtid] = 0;
	debug_uav2[dtid] = 0;
	debug_uav3[dtid] = 0;

	rng rng = create_rng(dtid.x + screen_size.x * (dtid.y + screen_size.y * root_constant));

	float4 pos;
	pos.x = (dtid.x + randF(rng)) * inv_screen_size.x;
	pos.y = (dtid.y + randF(rng)) * inv_screen_size.y;
	pos.xy *= float2(+2, -2);
	pos.xy += float2(-1, +1);
	pos.z = pos.w = 1;
	pos = mul(pos, inv_view_proj_mat);
	pos.xyz /= pos.w;

	float3 dir = normalize(pos.xyz - camera_pos);
	//result[dtid] = max(dir, 0);
	
	ray ray;
	ray.origin = camera_pos;
	ray.direction = dir;
	ray.tmin = 0;
	ray.tmax = 1000;

	float3 col = 0;

	uint4 info;
	trace_ray(ray, info, RAY_FLAG_NONE, INSTANCE_MASK_ALL, ch_default, ah_default, ms_default);
	if(is_hit(info))
	{
		ray_payload payload = decode_payload(info);
		intersection isect = get_intersection(payload.instance_id, 0, payload.primitive_index, payload.barycentrics, payload.is_front_face);

		float3 wo = -dir;
		isect.normal = normal_correction(isect.normal, wo);

		standard_material mtl = load_standard_material(isect.material_handle, wo, isect.normal, isect.tangent.xyz, get_binormal(isect), isect.uv, 0, dtid);
		if(has_contribution(dot(wo, isect.normal), mtl))
		{
			bsdf_sample s = sample_bsdf(wo, isect.normal, mtl, randF(rng), randF(rng), randF(rng), dtid);
			if(s.is_valid)
			{
				if(!is_occluded(isect.position, s.w, 10000))
					col = s.weight * env_cube.SampleLevel(bilinear_clamp, s.w, 4);
			}
		}

		//bsdf_sample s = sample_lambert_brdf(wo, isect.normal, 1, randF(rng), randF(rng));
		//float3 albedo = 1;
		//uint albedo_map_index = mtl.Load(8);
		//if(albedo_map_index != 0xffffffff)
		//{
		//	Texture2D<float3> albedo_map = get_texture2d<float3>(albedo_map_index);
		//	//Texture2D<float3> albedo_map = ResourceDescriptorHeap[NonUniformResourceIndex(albedo_map_index)];
		//	albedo = albedo_map.SampleLevel(bilinear_wrap, isect.uv, 0);
		//}
		//
		//if(!is_occluded(isect.position, s.w, 10000))
		//	result[dtid] = albedo * env_cube.SampleLevel(bilinear_clamp, s.w, 4) * s.weight;
		//else
		//	result[dtid] = 0;

		//result[dtid] = float3(isect.uv, 0);

		//if(payload.is_front_face)
		//{
		//	result[dtid] = float3(1,0,0);
		//}
		//else
		//{
		//	result[dtid] = float3(1,1,0);
		//}
		//result[dtid] = max(isect.geometry_normal, 0);

		//ray.origin = isect.position;
		//ray.direction = s.w;
		//result[dtid] = saturate(dot(isect.normal, normalize(float3(1,1,1))));
		//result[dtid] = hdri.SampleLevel(bilinear_clamp, s.w, 0) * s.weight;
		//result[dtid] = float3(randF(rng), randF(rng), randF(rng));

		//result[dtid] = float3(payload.barycentrics, 0);
		//result[dtid] = max(normalize(ray.origin + ray.direction * payload.ray_t), 0);
		
		//result[dtid] = max(normalize(isect.position), 0);
		//result[dtid] = max(normalize(isect.normal), 0);
		//result[dtid] = float3(isect.uv, 0);
		//result[dtid] = max((normalize(isect.normal) + normalize(isect.position)) / 2, 0);
		//result[dtid] = max(isect.geometry_normal, 0);
	}
	else
	{
		col = env_panorama.SampleLevel(bilinear_clamp, panorama_uv(ray.direction), 0);
		//result[dtid] = hdri.SampleLevel(bilinear_clamp, ray.direction, 0);
		//result[dtid] = hdri.SampleLevel(bilinear_clamp, ray.direction, 3);

	}

	if(any(!isfinite(col)) || any(col < 0))
	{
		for(uint i = 0; i < 10; i++)
		{
			for(uint j = 0; j < 10; j++)
			{
				result[dtid + uint2(i, j)] = float3(100,0,0);
			}
		}
	}
	else

	result[dtid] = col;
}


[numthreads(16, 16, 1)]
void accumulation(uint2 dtid : SV_DispatchThreadID)
{
	if(any(dtid >= screen_size))
		return;

	float3 col = result[dtid];
	float4 sum = accum[dtid];
	sum += float4(col, 1);
	result[dtid] = sum.rgb / sum.w;
	accum[dtid] = sum;
}

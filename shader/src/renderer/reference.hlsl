
#ifndef RENDERER_REFERENCE_HLSL
#define RENDERER_REFERENCE_HLSL

#include"../debug.hlsl"
#include"../random.hlsl"
#include"../utility.hlsl"
#include"../packing.hlsl"
#include"../raytracing.hlsl"
#include"../bssrdf_utility.hlsl"
#include"../raytracing_utility.hlsl"
#include"../global_constant.hlsl"
#include"../root_constant.hlsl"
#include"../directional_light.hlsl"
#include"../emissive_sampling.hlsl"
#include"../environment_sampling.hlsl"
#include"realistic_camera.hlsl"

#include"../material/hair.hlsl"
#include"../material/standard.hlsl"

#define ENABLE_HIT_EVAL 1
#define ENABLE_NEE_EVAL 1
#define FORCE_MIS 0

cbuffer reference_cbuf
{
	uint	max_bounce;
	uint	max_accumulation;
	uint	emissive_presample_count;
	uint	environment_presample_count;
};

RWTexture2D<uint>	color_uav;
RWTexture2D<float>	depth_uav;
RWTexture2D<float4>	accum_uav;

Texture2D<uint4>	ray_info_srv;
RWTexture2D<uint4>	ray_info_uav;
Texture2D<uint4>	hit_info_srv;
RWTexture2D<uint4>	hit_info_uav;
Texture2D<float4>	throughput_pdf_srv;
RWTexture2D<float4>	throughput_pdf_uav;

ByteAddressBuffer	work_queue_srv;
RWByteAddressBuffer	work_queue_uav;
ByteAddressBuffer	work_queue_size_srv;
RWByteAddressBuffer	work_queue_size_uav;
ByteAddressBuffer	work_queue_offset_srv;
RWByteAddressBuffer	work_queue_offset_uav;
RWByteAddressBuffer	dispatch_arg_uav;

///////////////////////////////////////////////////////////////////////////////////////////////////

#if !defined(MATERIAL_TYPE)
#define MATERIAL_TYPE 0
#endif

#if MATERIAL_TYPE == MATERIAL_TYPE_HAIR
#define MATERIAL		hair_material
#define LOAD_MATERIAL	load_hair_material
#else
#define MATERIAL		standard_material
#define LOAD_MATERIAL	load_standard_material
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////

void add_job(uint2 pixel_pos, uint material_type = -1)
{
	uint offset, count = WaveActiveCountBits(1);
	if(WaveIsFirstLane())
		work_queue_size_uav.InterlockedAdd(0, count, offset);

	offset = WaveReadLaneFirst(offset);
	offset += WavePrefixCountBits(1);
	work_queue_uav.Store(4 * offset, pixel_pos.x | (pixel_pos.y << 12) | (material_type << 24));

	if(material_type == -1)
		return;

	for(uint i = 0; i < 32; i++)
	{
		uint current = WaveReadLaneFirst(material_type);
		uint count = WaveActiveCountBits(material_type == current);
		if(WaveIsFirstLane())
			work_queue_size_uav.InterlockedAdd(8 + 4 * material_type, count);
		if(material_type == current)
			break;
	
	}
}

void add_sss_job(uint2 pixel_pos)
{
	uint offset, count = WaveActiveCountBits(1);
	if(WaveIsFirstLane())
		work_queue_size_uav.InterlockedAdd(4, count, offset);

	offset = WaveReadLaneFirst(offset);
	offset = screen_size.x * screen_size.y - offset - count;
	offset += WavePrefixCountBits(1);
	work_queue_uav.Store(4 * offset, pixel_pos.x | (pixel_pos.y << 16));
}

void store_throughput_pdf(uint2 pixel_pos, float3 throughput, float pdf)
{
	throughput_pdf_uav[pixel_pos] = float4(throughput, pdf);
}

void store_ray_info(uint2 pixel_pos, float3 o, float3 d)
{
	ray_info_uav[pixel_pos] = uint4(asuint(o), encode_direction(d));
}

void store_hit_info(uint2 pixel_pos, ray_payload payload)
{
	hit_info_uav[pixel_pos] = encode_payload(payload);
}

void store_radiance(uint2 pixel_pos, float3 radiance, bool add = true)
{
	if(add){ radiance += r9g9b9e5_to_f32x3(color_uav[pixel_pos]); }
	color_uav[pixel_pos] = f32x3_to_r9g9b9e5(radiance);
}

uint get_material_type(uint enc)
{
	return enc >> 24;
}

uint2 get_job(uint index)
{
	uint offset = 0;
#if defined(MATERIAL_TYPE)
	offset = work_queue_offset_srv.Load(4 * MATERIAL_TYPE);
#endif
	uint enc = work_queue_srv.Load(4 * (offset + index));
	return uint2(enc & 0xfff, (enc >> 12) & 0xfff);
}

uint2 get_sss_job(uint index)
{
	uint size = work_queue_size_srv.Load(4);
	uint enc = work_queue_srv.Load(4 * (screen_size.x * screen_size.y - size + index));
	return uint2(enc & 0xffff, enc >> 16);
}

float4 get_throughput_pdf(uint2 pixel_pos)
{
	return throughput_pdf_srv[pixel_pos];
}

void get_ray_info(uint2 pixel_pos, out float3 o, out float3 d)
{
	uint4 enc = ray_info_srv[pixel_pos];
	o = asfloat(enc.xyz);
	d = decode_direction(enc.w);
}

ray_payload get_hit_info(uint2 pixel_pos)
{
	uint4 enc = hit_info_srv[pixel_pos];
	return decode_payload(enc);
}

namespace ref{ bool has_contribution(float nwi, MATERIAL mtl)
{
#if defined(SSS_LIGHTING_AND_SAMPLING)
	return (nwi > 0);
#else
	return ::has_contribution(nwi, mtl);
#endif
}}

namespace ref{ float4 calc_bsdf_pdf(float3 wo, float3 wi, float3 normal, MATERIAL mtl)
{
#if defined(SSS_LIGHTING_AND_SAMPLING)

	float3 tangent, binormal;
	calc_orthonormal_basis(normal, tangent, binormal);

	wi = float3(dot(tangent, wi), dot(binormal, wi), dot(normal, wi));
	return float4(calc_subsurface(mtl) * wi.z * inv_PI, sample_cosine_hemisphere_pdf(wi)); //本当はwiでmaterialを読みなおさないとダメ.重すぎるのでwi=normalのmaterialで近似.

#else
	return ::calc_bsdf_pdf(wo, wi, normal, mtl);
#endif
}}

namespace ref{ bsdf_sample sample_bsdf(float3 wo, float3 normal, MATERIAL mtl, float u0, float u1, uint2 dtid = 0)
{
#if defined(SSS_LIGHTING_AND_SAMPLING)

	float3 tangent, binormal;
	calc_orthonormal_basis(normal, tangent, binormal);
	
	bsdf_sample s;
	s.w = sample_cosine_hemisphere(u0, u1);
	s.w = s.w.x * tangent + s.w.y * binormal + s.w.z * normal;
	float4 bsdf_pdf = ref::calc_bsdf_pdf(wo, s.w, normal, mtl);
	s.weight = bsdf_pdf.rgb / bsdf_pdf.a;
	s.pdf = bsdf_pdf.a;
	s.is_valid = true;
	return s;

#else
	return ::sample_bsdf(wo, normal, mtl, u0, u1, dtid);
#endif
}}

///////////////////////////////////////////////////////////////////////////////////////////////////

float calc_mis_weight(float pdf_a, float pdf_b)
{
	return pdf_a / (pdf_a + pdf_b);
}

float3 emissive_lighting(intersection isect, MATERIAL mtl, float3 wo, inout rng rng)
{
	uint2 active_thread_count_and_offset = calc_active_thread_count_and_offset();
	uint index = WaveReadLaneFirst(rand(rng)) + active_thread_count_and_offset.y;
	index &= emissive_presample_count - 1;

	emissive_sample s = decompress(emissive_sample_srv[index]);
	if(s.pdf_approx <= 0)
		return 0;

	float3 wi = s.position - isect.position;
	float dist2 = dot(wi, wi);
	float inv_dist = rsqrt(dist2);
	wi *= inv_dist;

	float nwi = dot(isect.normal, wi);
	if(!ref::has_contribution(nwi, mtl))
		return 0;

	float light_nwo = -dot(s.normal, wi);
	if(light_nwo <= 0)
		return 0;

	if(is_occluded(isect.position, wi, 1 / inv_dist, randF(rng)))
		return 0;

	float4 bsdf_pdf = ref::calc_bsdf_pdf(wo, wi, isect.normal, mtl);

	float mis_weight = 1;
#if ENABLE_HIT_EVAL || FORCE_MIS
	mis_weight = calc_mis_weight(s.pdf_approx, bsdf_pdf.w * light_nwo * pow2(inv_dist));
#endif
	return s.L * bsdf_pdf.rgb * light_nwo * pow2(inv_dist) * mis_weight;
}

float3 directional_lighting(intersection isect, MATERIAL mtl, float3 wo, inout rng rng)
{
	float3 wi = sample_directional_light(randF(rng), randF(rng));
	if(!ref::has_contribution(dot(isect.normal, wi), mtl))
		return 0;

	if(is_occluded(isect.position, wi, FLT_MAX, randF(rng)))
		return 0;

	float4 bsdf_pdf = ref::calc_bsdf_pdf(wo, wi, isect.normal, mtl);

	float mis_weight = 1;
#if ENABLE_HIT_EVAL || FORCE_MIS
	mis_weight = calc_mis_weight(sample_directional_light_pdf(wi), bsdf_pdf.w);
#endif
	float inv_pdf = directional_light_solid_angle;
	return directional_light_power * bsdf_pdf.rgb * mis_weight * inv_pdf;
}

float3 environment_lighting(intersection isect, MATERIAL mtl, float3 wo, inout rng rng)
{
	uint2 active_thread_count_and_offset = calc_active_thread_count_and_offset();
	uint index = WaveReadLaneFirst(rand(rng)) + active_thread_count_and_offset.y;
	index &= environment_presample_count - 1;

	environment_sample s = decompress(environment_sample_srv[index]);

	float nwi = dot(isect.normal, s.w);
	if(!ref::has_contribution(nwi, mtl))
		return 0;

	if(is_occluded(isect.position, s.w, FLT_MAX, randF(rng)))
		return 0;

	float4 bsdf_pdf = ref::calc_bsdf_pdf(wo, s.w, isect.normal, mtl);

	float mis_weight = 1;
#if ENABLE_HIT_EVAL || FORCE_MIS
	mis_weight = calc_mis_weight(s.pdf, bsdf_pdf.w);
#endif
	return s.L * bsdf_pdf.rgb * mis_weight;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(FIRST)
[numthreads(8, 4, 1)]
#else
[numthreads(32, 1, 1)]
#endif
void tracing_and_miss_lighting(uint2 dtid : SV_DispatchThreadID)
{
	ray ray;
	ray.tmin = 0.001f;
	ray.tmax = 1000;

	rng rng;
	rng.state = dtid.x + screen_size.x * (dtid.y + screen_size.y * frame_count);

#if defined(FIRST)

	if(any(dtid >= screen_size))
		return;

	float2 pixel_pos;
	pixel_pos.x = dtid.x + randF(rng);
	pixel_pos.y = dtid.y + randF(rng);

	ray.origin = camera_pos;
	ray.direction = normalize(screen_to_world(pixel_pos, 1) - camera_pos);

	float pdf_w = 0; //bounce==0はMISしないのでpdfは何でもいい
	float3 throughput = 1;

#if defined(USE_REALISTIC_CAMERA)
	if(!generate_ray(pixel_pos, randF(rng), randF(rng), randF(rng), ray.origin, ray.direction, throughput))
		return;
#endif
#elif !defined(FIRST)

	if(dtid.x >= work_queue_size_srv.Load(0))
		return;

	dtid = get_job(dtid.x);
	get_ray_info(dtid, ray.origin, ray.direction);

	float4 throughput_pdf = get_throughput_pdf(dtid);
	float3 throughput = throughput_pdf.rgb;
	float pdf_w = throughput_pdf.a;

#endif

#if defined(FIRST)
	bool enable_displacement = true;
#else
	bool enable_displacement = false;
#endif

	ray_payload payload;
	if(!find_closest(ray, payload, randF(rng), enable_displacement, dtid))
	{
#if defined(FIRST)

		float3 radiance = 0;
		if(exists_environment_light())
			radiance = env_panorama_srv.SampleLevel(bilinear_clamp, panorama_uv(ray.direction), 0) * throughput;
		
		store_radiance(dtid, radiance, false);

#elif !defined(FIRST) && ENABLE_HIT_EVAL

		if(exists_environment_light() || (exists_directional_light() && hit_directional_light(ray.direction)))
		{
			float3 radiance = 0;
			if(exists_environment_light())
			{
				float mis_weight = 1;
#if ENABLE_NEE_EVAL || FORCE_MIS
				mis_weight = calc_mis_weight(pdf_w, sample_environment_pdf(ray.direction));
#endif
				float3 L = env_cube_srv.SampleLevel(bilinear_clamp, ray.direction, 0);
				radiance += mis_weight * L;
			}

			if(exists_directional_light() && hit_directional_light(ray.direction))
			{
				float mis_weight = 1;
#if ENABLE_NEE_EVAL || FORCE_MIS
				mis_weight = calc_mis_weight(pdf_w, sample_directional_light_pdf(ray.direction));
#endif
				radiance += mis_weight * directional_light_power;
			}

			store_radiance(dtid, radiance * throughput, true);
		}
#endif
	}
	else
	{
		float3 wo = -ray.direction;
		intersection isect = get_intersection(payload);
		isect.normal = normal_correction(isect.normal, wo);
		isect.position = ray.origin + ray.direction * payload.ray_t;

		material_header header = load_material_header(isect.material_handle);

#if !defined(LAST)

		add_job(dtid, header.material_type);
		store_hit_info(dtid, payload);

#if defined(FIRST)
		store_ray_info(dtid, ray.origin, ray.direction);
		store_throughput_pdf(dtid, throughput, pdf_w);
#endif

#elif defined(LAST) && ENABLE_HIT_EVAL

		if(header.material_type == MATERIAL_TYPE_STANDARD) //emissiveをcommonにしてもいい
		{
			standard_material mtl = load_standard_material(isect.material_handle, wo, isect.normal, isect.tangent.xyz, get_binormal(isect), isect.uv, 0);

			float3 Le = get_emissive_color(mtl);
			if(any(Le > 0))
			{
				float mis_weight = 1;
#if ENABLE_NEE_EVAL || FORCE_MIS
				mis_weight = calc_mis_weight(pdf_w * dot(wo, isect.geometry_normal) / pow2(payload.ray_t), emissive_sample_pdf(Le));
#endif
				store_radiance(dtid, Le * throughput * mis_weight, true);
			}
		}
#endif
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

[numthreads(32, 1, 1)]
void lighting_and_sampling(uint2 dtid : SV_DispatchThreadID)
{
	if(dtid.x >= work_queue_size_srv.Load(8 + 4 * MATERIAL_TYPE))
		return;

	dtid = get_job(dtid.x);

	ray ray;
	get_ray_info(dtid, ray.origin, ray.direction);

	float3 wo = -ray.direction;
	ray_payload payload = get_hit_info(dtid);
	intersection isect = get_intersection(payload);
	isect.normal = normal_correction(isect.normal, wo);

	float4 throughput_pdf = get_throughput_pdf(dtid);
	float3 throughput = throughput_pdf.rgb;
	float pdf_w = throughput_pdf.a;

	MATERIAL mtl = LOAD_MATERIAL(isect.material_handle, wo, isect.normal, isect.tangent.xyz, get_binormal(isect), isect.uv, 0);

	float3 radiance = 0;

#if MATERIAL_TYPE == MATERIAL_TYPE_STANDARD //emiisiveをcommonにしてもいい
#if defined(FIRST)

	radiance = get_emissive_color(mtl);

#elif !defined(FIRST) && ENABLE_HIT_EVAL

	float3 Le = get_emissive_color(mtl);
	if(any(Le > 0))
	{
		float mis_weight = 1;
#if ENABLE_NEE_EVAL || FORCE_MIS
		mis_weight = calc_mis_weight(pdf_w * dot(wo, isect.geometry_normal) / pow2(payload.ray_t), emissive_sample_pdf(Le));
#endif
		radiance = Le * mis_weight;
	}

#endif
#endif

	rng rng;
	rng.state = dtid.x + screen_size.x * (dtid.y + screen_size.y * frame_count);
	rng.state += root_constant;

#if ENABLE_NEE_EVAL

	if(exists_emissive())
		radiance += emissive_lighting(isect, mtl, wo, rng);

	if(exists_directional_light())
		radiance += directional_lighting(isect, mtl, wo, rng);

	if(exists_environment_light())
		radiance += environment_lighting(isect, mtl, wo, rng);

#endif

	bsdf_sample s = ref::sample_bsdf(wo, isect.normal, mtl, randF(rng), randF(rng));
	if(s.is_valid)
	{
#if MATERIAL_TYPE == MATERIAL_TYPE_STANDARD
		if((dot(s.w, isect.normal) <= 0) && !is_twoside(mtl))
		{
			//bssrdfの入射位置サンプリングはdisplacement無効にするので,出射位置もdisplacement無効状態の位置にする
			//こうしないと出射位置と入射位置が必ずある程度離れた状態になってしまう
			float3 position = isect.position;
			if(payload.hit_type == HIT_TYPE_DISPLACEMENT)
			{
				payload.hit_type = HIT_TYPE_TRIANGLE;
				position = get_intersection(payload).position;
			}

			add_sss_job(dtid);
			store_ray_info(dtid, position, isect.normal);
			store_throughput_pdf(dtid, throughput * s.weight, asfloat(f32x3_to_r9g9b9e5(get_subsurface_radius(mtl))));
		}
		else
#endif
		{
			add_job(dtid);
			store_ray_info(dtid, isect.position, s.w);
			store_throughput_pdf(dtid, throughput * s.weight, s.pdf);
		}
	}

	store_radiance(dtid, radiance * throughput, true);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

[numthreads(32, 1, 1)]
void sss_sampling_and_tracing(uint2 dtid : SV_DispatchThreadID)
{
	if(dtid.x >= work_queue_size_srv.Load(4))
		return;

	dtid = get_sss_job(dtid.x);
	
	float3 position, normal;
	get_ray_info(dtid, position, normal);

	float4 throughput_d = get_throughput_pdf(dtid);
	float3 throughput = throughput_d.rgb;
	float3 d = r9g9b9e5_to_f32x3(asuint(throughput_d.w));

	rng rng;
	rng.state = dtid.x + screen_size.x * (dtid.y + screen_size.y * frame_count);
	rng.state += root_constant;

	sss_payload payload;
	ray ray = sample_bssrdf(position, normal, d, randF(rng), randF(rng), dtid);
	if(find_hit(ray, randF(rng), payload, dtid))
	{
		intersection isect = get_intersection(payload);
		float pdf = sample_bssrdf_pdf(position, normal, isect.position, isect.geometry_normal, payload.hit_count, d, dtid);
		throughput *= min(bssrdf(length(position - isect.position), d) / pdf, 100);

		add_sss_job(dtid);
		store_hit_info(dtid, payload);
		store_throughput_pdf(dtid, throughput, 0);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

[numthreads(32, 1, 1)]
void sss_lighting_and_sampling(uint2 dtid : SV_DispatchThreadID)
{
	if(dtid.x >= work_queue_size_srv.Load(4))
		return;

	dtid = get_sss_job(dtid.x);
	ray_payload payload = get_hit_info(dtid);
	intersection isect = get_intersection(payload);
	float4 throughput_pdf = get_throughput_pdf(dtid);
	float3 throughput = throughput_pdf.rgb;

	float3 wo = isect.normal; //近似.本当はwiが決まるごとにmaterialを初期化
	MATERIAL mtl = LOAD_MATERIAL(isect.material_handle, wo, isect.normal, isect.tangent.xyz, get_binormal(isect), isect.uv, 0);

	rng rng;
	rng.state = dtid.x + screen_size.x * (dtid.y + screen_size.y * frame_count);
	rng.state += root_constant;

#if ENABLE_NEE_EVAL

	float3 radiance = 0;

	if(exists_emissive())
		radiance += emissive_lighting(isect, mtl, wo, rng);

	if(exists_directional_light())
		radiance += directional_lighting(isect, mtl, wo, rng);

	if(exists_environment_light())
		radiance += environment_lighting(isect, mtl, wo, rng);

	store_radiance(dtid, radiance * throughput, true);

#endif

	bsdf_sample s = ref::sample_bsdf(wo, isect.normal, mtl, randF(rng), randF(rng));
	if(s.is_valid)
	{
		add_job(dtid);
		store_ray_info(dtid, isect.position, s.w);
		store_throughput_pdf(dtid, throughput * s.weight, s.pdf);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

groupshared uint shared_offset[256];

[numthreads(256, 1, 1)]
void prepare_filtering(uint group_index : SV_GroupIndex)
{
	uint lane_index = WaveGetLaneIndex();
	uint lane_count = WaveGetLaneCount();
	uint wave_index = group_index / lane_count;

	uint count = work_queue_size_srv.Load(8 + 4 * group_index);
	dispatch_arg_uav.Store3(12 * (group_index + 1), uint3(ceil_div(count, 32), 1, 1));
	work_queue_size_uav.Store(8 + 4 * group_index, 0);
	
	if(group_index == 0)
	{
		uint count = work_queue_size_srv.Load(0);
		dispatch_arg_uav.Store3(0, uint3(ceil_div(ceil_div(count, 4), 256), 1, 1));
		work_queue_size_uav.Store(0, 0);
	}

	uint offset = WavePrefixSum(count) + count;
	if(lane_index == lane_count - 1)
		shared_offset[wave_index] = offset;
	GroupMemoryBarrierWithGroupSync();

	if(group_index < 256 / lane_count)
		shared_offset[group_index] = WavePrefixSum(shared_offset[group_index]);
	GroupMemoryBarrierWithGroupSync();

	offset += shared_offset[wave_index];
	work_queue_offset_uav.Store(4 * group_index, offset);
}

groupshared uint shared_count[256];

[numthreads(256, 1, 1)]
void filtering(uint gid : SV_GroupID, uint group_index : SV_GroupIndex)
{
	shared_count[group_index] = 0;
	GroupMemoryBarrierWithGroupSync();

	uint base = 256 * 4 * gid;
	uint size = work_queue_size_srv.Load(0);

	uint4 tasks = 0xffffffff;
	if(base + 4 * group_index + 3 < size)
		tasks.xyzw = work_queue_srv.Load4(4 * (base + 4 * group_index));
	else if(base + 4 * group_index + 2 < size)
		tasks.xyz = work_queue_srv.Load3(4 * (base + 4 * group_index));
	else if(base + 4 * group_index + 1 < size)
		tasks.xy = work_queue_srv.Load2(4 * (base + 4 * group_index));
	else if(base + 4 * group_index + 0 < size)
		tasks.x = work_queue_srv.Load(4 * (base + 4 * group_index));

	for(uint i = 0; i < 4; i++)
	{
		if(tasks[i] == 0xffffffff)
			break;

		for(uint j = 0; j < 32; j++)
		{
			uint material_type = get_material_type(tasks[i]);
			uint current = WaveReadLaneFirst(material_type);
			uint count = WaveActiveCountBits(material_type == current);
			if(WaveIsFirstLane())
				InterlockedAdd(shared_count[current], count);
			if(material_type == current)
				break;
		}
	}
	GroupMemoryBarrierWithGroupSync();

	uint offset, count = shared_count[group_index];
	work_queue_offset_uav.InterlockedAdd(4 * group_index, -count, offset);
	shared_count[group_index] = offset - count;
	GroupMemoryBarrierWithGroupSync();

	for(uint i = 0; i < 4; i++)
	{
		if(tasks[i] == 0xffffffff)
			break;

		for(uint j = 0; j < 32; j++)
		{
			uint material_type = get_material_type(tasks[i]);
			uint current = WaveReadLaneFirst(material_type);
			uint count = WaveActiveCountBits(material_type == current);
			uint offset = WavePrefixCountBits(material_type == current), wave_offset;
			if(WaveIsFirstLane())
				InterlockedAdd(shared_count[current], count, wave_offset);
		
			if(material_type == current)
			{
				offset += WaveReadLaneFirst(wave_offset);
				work_queue_uav.Store(4 * offset, tasks[i]);
				break;
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

[numthreads(256, 1, 1)]
void init_dispatch_argument(uint dtid : SV_DispatchThreadID)
{
#if defined(SSS)
	uint offset = 4;
#else
	uint offset = 0;
#endif
	if(dtid == 0)
	{
		uint size = work_queue_size_srv.Load(offset);
		dispatch_arg_uav.Store3(0, uint3(ceil_div(size, 32), 1, 1));
		work_queue_size_uav.Store(offset, 0);
	}
#if !defined(SSS)
	work_queue_size_uav.Store(8 + 4 * dtid, 0);
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////

[numthreads(16, 16, 1)]
void accumulate(uint2 dtid : SV_DispatchThreadID)
{
	if(any(dtid >= screen_size))
		return;

	float4 sum = accum_uav[dtid];
	if(sum.a >= max_accumulation)
	{
		sum.rgb *= (max_accumulation - 1) / sum.a;
		sum.a = max_accumulation - 1;
	}

	float3 col = r9g9b9e5_to_f32x3(color_uav[dtid]);
	sum += float4(col, 1);
	accum_uav[dtid] = sum;
	color_uav[dtid] = f32x3_to_r9g9b9e5(sum.rgb / sum.a);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

/*
[numthreads(8, 4, 1)]
void path_tracing(uint2 dtid : SV_DispatchThreadID)
{
	if(any(dtid >= screen_size))
		return;

	rng rng;
	rng.state = dtid.x + screen_size.x * (dtid.y + screen_size.y * frame_count);

	float2 pixel_pos;
	pixel_pos.x = dtid.x + randF(rng);
	pixel_pos.y = dtid.y + randF(rng);

	ray ray;
	ray.origin = camera_pos;
	ray.direction = normalize(screen_to_world(pixel_pos, 1) - camera_pos);
	ray.tmin = 0.001f;
	ray.tmax = 1000;

	float3 radiance = 0;
	float3 throughput = 1;
#if defined(USE_REALISTIC_CAMERA)
	if(generate_ray(pixel_pos, randF(rng), randF(rng), randF(rng), ray.origin, ray.direction, throughput))
#endif
	{
		float pdf_w = 0;
		uint bounce = 0;
		while(true)
		{
			ray_payload payload;
			if(!find_closest(ray, payload))
			{
				if(bounce == 0)
				{
					radiance += env_panorama_srv.SampleLevel(bilinear_clamp, panorama_uv(ray.direction), 0) * throughput;
				}
				else
				{
#if ENABLE_HIT_EVAL

					if(exists_directional_light() && hit_directional_light(ray.direction))
					{
						float mis_weight = 1;
#if ENABLE_NEE_EVAL || FORCE_MIS
						mis_weight = calc_mis_weight(pdf_w, sample_directional_light_pdf(ray.direction));
#endif
						radiance += mis_weight * directional_light_power * throughput;
					}

					if(exists_environment_light())
					{
						float mis_weight = 1;
#if ENABLE_NEE_EVAL || FORCE_MIS
						mis_weight = calc_mis_weight(pdf_w, sample_environment_pdf(ray.direction));
#endif
						float3 L = env_cube_srv.SampleLevel(bilinear_clamp, ray.direction, 0);
						radiance += mis_weight * L * throughput;
					}
#endif
				}
				break;
			}

			float3 wo = -ray.direction;
			intersection isect = get_intersection(payload);
			isect.position = ray.origin + ray.direction * payload.ray_t;
			isect.normal = normal_correction(isect.normal, wo);

			if(bounce == 0)
				depth_uav[dtid] = world_to_screen(isect.position).z;

			standard_material mtl = load_standard_material(isect.material_handle, wo, isect.normal, isect.tangent.xyz, get_binormal(isect), isect.uv, 0);

			if(bounce == 0)
				radiance += get_emissive_color(mtl);
			else
			{
#if ENABLE_HIT_EVAL
				float3 Le = get_emissive_color(mtl);
				if(any(Le > 0))
				{
					float mis_weight = 1;
#if ENABLE_NEE_EVAL || FORCE_MIS
					mis_weight = calc_mis_weight(pdf_w * dot(wo, isect.geometry_normal) / pow2(payload.ray_t), emissive_sample_pdf(Le));
#endif
					radiance += Le * throughput * mis_weight;
				}
#endif
			}

			if(++bounce > max_bounce)
				break;

#if ENABLE_NEE_EVAL

			if(exists_emissive())
				radiance += emissive_lighting(isect, mtl, wo, rng) * throughput;

			if(exists_directional_light())
				radiance += directional_lighting(isect, mtl, wo, rng) * throughput;

			if(exists_environment_light())
				radiance += environment_lighting(isect, mtl, wo, rng) * throughput;
#endif

			bsdf_sample s = sample_bsdf(wo, isect.normal, mtl, randF(rng), randF(rng));
			if(!s.is_valid)
				break;

			pdf_w = s.pdf;
			throughput *= s.weight;
		
			ray.origin = isect.position;
			ray.direction = s.w;
		}
	}

	float4 sum = accum_uav[dtid];
	if(sum.a >= max_accumulation)
	{
		sum.rgb *= (max_accumulation - 1) / sum.a;
		sum.a = max_accumulation - 1;
	}
	sum += float4(radiance, 1);
	accum_uav[dtid] = sum;
	color_uav[dtid] = f32x3_to_r9g9b9e5(sum.rgb / sum.a);
}
*/

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

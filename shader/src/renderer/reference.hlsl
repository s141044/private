
#ifndef RENDERER_REFERENCE_HLSL
#define RENDERER_REFERENCE_HLSL

#include"../random.hlsl"
#include"../utility.hlsl"
#include"../packing.hlsl"
#include"../raytracing.hlsl"
#include"../raytracing_utility.hlsl"
#include"../global_constant.hlsl"
#include"../root_constant.hlsl"
#include"../directional_light.hlsl"
#include"../emissive_sampling.hlsl"
#include"../environment_sampling.hlsl"
#include"realistic_camera.hlsl"

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
RWByteAddressBuffer	dispatch_arg_uav;

///////////////////////////////////////////////////////////////////////////////////////////////////

void add_job(uint2 pixel_pos)
{
	uint offset, count = WaveActiveCountBits(1);
	if(WaveIsFirstLane())
		work_queue_size_uav.InterlockedAdd(0, count, offset);

	offset = WaveReadLaneFirst(offset);
	offset += WavePrefixCountBits(1);
	work_queue_uav.Store(4 * offset, pixel_pos.x | (pixel_pos.y << 16));
}

void add_sss_job(uint2 pixel_pos)
{
	uint offset, count = WaveActiveCountBits(1);
	if(WaveIsFirstLane())
		work_queue_size_uav.InterlockedAdd(4, count, offset);

	offset = WaveReadLaneFirst(offset);
	offset += count - WavePrefixCountBits(1) - 1;
	offset += screen_size.x * screen_size.y - count;
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

uint2 get_job(uint index)
{
	uint enc = work_queue_srv.Load(4 * index);
	return uint2(enc & 0xffff, enc >> 16);
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

///////////////////////////////////////////////////////////////////////////////////////////////////

float calc_mis_weight(float pdf_a, float pdf_b)
{
	return pdf_a / (pdf_a + pdf_b);
}

float3 emissive_lighting(intersection isect, standard_material mtl, float3 wo, inout rng rng)
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
	if(!has_contribution(nwi, mtl))
		return 0;

	float light_nwo = -dot(s.normal, wi);
	if(light_nwo <= 0)
		return 0;

	if(is_occluded(isect.position, wi, 1 / inv_dist))
		return 0;

	float4 bsdf_pdf = calc_bsdf_pdf(wo, wi, isect.normal, mtl);

	float mis_weight = 1;
#if ENABLE_HIT_EVAL || FORCE_MIS
	mis_weight = calc_mis_weight(s.pdf_approx, bsdf_pdf.w * light_nwo * pow2(inv_dist));
#endif
	return s.L * bsdf_pdf.rgb * light_nwo * pow2(inv_dist) * mis_weight;
}

float3 directional_lighting(intersection isect, standard_material mtl, float3 wo, inout rng rng)
{
	float3 wi = sample_directional_light(randF(rng), randF(rng));
	if(!has_contribution(dot(isect.normal, wi), mtl))
		return 0;

	if(is_occluded(isect.position, wi, FLT_MAX))
		return 0;

	float4 bsdf_pdf = calc_bsdf_pdf(wo, wi, isect.normal, mtl);

	float mis_weight = 1;
#if ENABLE_HIT_EVAL || FORCE_MIS
	mis_weight = calc_mis_weight(sample_directional_light_pdf(wi), bsdf_pdf.w);
#endif
	float inv_pdf = directional_light_solid_angle;
	return directional_light_power * bsdf_pdf.rgb * mis_weight * inv_pdf;
}

float3 environment_lighting(intersection isect, standard_material mtl, float3 wo, inout rng rng)
{
	uint2 active_thread_count_and_offset = calc_active_thread_count_and_offset();
	uint index = WaveReadLaneFirst(rand(rng)) + active_thread_count_and_offset.y;
	index &= environment_presample_count - 1;

	environment_sample s = decompress(environment_sample_srv[index]);

	float nwi = dot(isect.normal, s.w);
	if(!has_contribution(nwi, mtl))
		return 0;

	if(is_occluded(isect.position, s.w, FLT_MAX))
		return 0;

	float4 bsdf_pdf = calc_bsdf_pdf(wo, s.w, isect.normal, mtl);

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


#if defined(FIRST)

	if(any(dtid >= screen_size))
		return;

	rng rng;
	rng.state = dtid.x + screen_size.x * (dtid.y + screen_size.y * frame_count);

	float2 pixel_pos;
	pixel_pos.x = dtid.x + randF(rng);
	pixel_pos.y = dtid.y + randF(rng);

	ray.origin = camera_pos;
	ray.direction = normalize(screen_to_world(pixel_pos, 1) - camera_pos);

	float pdf_w = 0; //bounce==0‚ÍMIS‚µ‚È‚¢‚Ì‚Åpdf‚Í‰½‚Å‚à‚¢‚¢
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

	ray_payload payload;
	if(!find_closest(ray, payload))
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
#if !defined(LAST)

		add_job(dtid);
		store_hit_info(dtid, payload);

#if defined(FIRST)
		store_ray_info(dtid, ray.origin, ray.direction);
		store_throughput_pdf(dtid, throughput, pdf_w);
#endif

#elif defined(LAST) && ENABLE_HIT_EVAL
	
		float3 wo = -ray.direction;
		intersection isect = get_intersection(payload);
		isect.normal = normal_correction(isect.normal, wo);
		isect.position = ray.origin + ray.direction * payload.ray_t;
		
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
#endif
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

[numthreads(32, 1, 1)]
void lighting_and_sampling(uint2 dtid : SV_DispatchThreadID)
{
	if(dtid.x >= work_queue_size_srv.Load(0))
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

	standard_material mtl = load_standard_material(isect.material_handle, wo, isect.normal, isect.tangent.xyz, get_binormal(isect), isect.uv, 0);

	float3 radiance = 0;

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

#if ENABLE_NEE_EVAL

	rng rng;
	rng.state = dtid.x + screen_size.x * (dtid.y + screen_size.y * frame_count);
	rng.state += root_constant;

	if(exists_emissive())
		radiance += emissive_lighting(isect, mtl, wo, rng);

	if(exists_directional_light())
		radiance += directional_lighting(isect, mtl, wo, rng);

	if(exists_environment_light())
		radiance += environment_lighting(isect, mtl, wo, rng);

	bsdf_sample s = sample_bsdf(wo, isect.normal, mtl, randF(rng), randF(rng), randF(rng));
	if(s.is_valid)
	{
		add_job(dtid);
		store_ray_info(dtid, isect.position, s.w);
		store_throughput_pdf(dtid, throughput * s.weight, s.pdf);
	}

#endif

	store_radiance(dtid, radiance * throughput, true);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

[numthreads(1, 1, 1)]
void init_dispatch_argument()
{
	uint size = work_queue_size_srv.Load(0);
	dispatch_arg_uav.Store3(0, uint3((size + 31) / 32, 1, 1));
	work_queue_size_uav.Store(0, 0);
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

			bsdf_sample s = sample_bsdf(wo, isect.normal, mtl, randF(rng), randF(rng), randF(rng));
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

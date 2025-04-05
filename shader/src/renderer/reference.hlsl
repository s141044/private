
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

cbuffer reference_cbuf
{
	uint	max_bounce;
	uint	emissive_presample_count;
	uint	environment_presample_count;
	uint	reference_cbuf_reserved;
};

RWTexture2D<uint>	color_uav;
RWTexture2D<float>	depth_uav;
RWTexture2D<float4>	accum_uav;

#define ENABLE_HIT_EVAL 1
#define ENABLE_NEE_EVAL 1
#define FORCE_MIS 0

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
	if(generate_ray(pixel_pos, randF(rng), randF(rng), randF(rng), ray.origin, ray.direction, throughput, dtid))
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

#if ENABLE_HIT_EVAL
			float3 Le = get_emissive_color(mtl);
			if(any(Le > 0))
			{
				float mis_weight = 1;
#if ENABLE_NEE_EVAL || FORCE_MIS
				if(bounce > 0)
					mis_weight = calc_mis_weight(pdf_w, emissive_sample_pdf(Le) * dot(wo, isect.geometry_normal) / pow2(payload.ray_t));
#endif
				radiance += Le * throughput * mis_weight;
			}
#endif

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
	sum += float4(radiance, 1);
	accum_uav[dtid] = sum;
	color_uav[dtid] = f32x3_to_r9g9b9e5(sum.rgb / sum.a);
}

#endif

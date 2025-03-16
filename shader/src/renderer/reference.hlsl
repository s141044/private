
#include"../random.hlsl"
#include"../utility.hlsl"
#include"../packing.hlsl"
#include"../raytracing.hlsl"
#include"../raytracing_utility.hlsl"
#include"../global_constant.hlsl"
#include"../root_constant.hlsl"
//#include"../environment.hlsl"
//#include"../environment_sampling.hlsl"
#include"../emissive_sampling.hlsl"

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
	return s.power * bsdf_pdf.rgb * light_nwo * pow2(inv_dist) * mis_weight;
}

[numthreads(8, 4, 1)]
void path_tracing(uint2 dtid : SV_DispatchThreadID)
{
	if(any(dtid >= screen_size))
		return;

	rng rng;
	rng.state = dtid.x + screen_size.x * (dtid.y + screen_size.y * frame_count);

	ray ray;
	ray.origin = camera_pos;
	ray.direction = normalize(screen_to_world(dtid + 0.5f, 1) - camera_pos);
	ray.tmin = 0.001f;
	ray.tmax = 1000;

	float pdf_w = 0;
	float3 radiance = 0;
	float3 throughput = 1;

	uint bounce = 0;
	while(true)
	{
		ray_payload payload;
		if(!find_closest(ray, payload))
		{
			//dir
			//env

			//float mis_weight = 
			//
			//float3 L = env_cube.SampleLevel(bilinear_clamp, s.w, cube_sample_level(s.w, s.pdf, M_BSDF, environment_light_log2_texel_size)) * environment_light_power_scale;
			//radiance += mis_weight * L * throughput;
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

		if(++bounce >= max_bounce)
			break;

#if ENABLE_NEE_EVAL

		if(exists_emissive())
			radiance += emissive_lighting(isect, mtl, wo, rng) * throughput;

		//if(1)
		//{
		//	radiance += directional_lighting(isect, mtl, wo, rng) * throughput;
		//}
		//if(1)
		//{
		//	radiance += environment_lighting(isect, mtl, wo, rng) * throughput;
		//}
#endif

		bsdf_sample s = sample_bsdf(wo, isect.normal, mtl, randF(rng), randF(rng), randF(rng));
		if(!s.is_valid)
			break;

		pdf_w = s.pdf;
		throughput *= s.weight;
		
		ray.origin = isect.position;
		ray.direction = s.w;
	}

	float4 sum = accum_uav[dtid];
	sum += float4(radiance, 1);
	accum_uav[dtid] = sum;
	color_uav[dtid] = f32x3_to_r9g9b9e5(sum.rgb / sum.a);
}


RWTexture2D<float4> debug_uav0;
RWTexture2D<float4> debug_uav1;
RWTexture2D<float4> debug_uav2;
RWTexture2D<float4> debug_uav3;

#include"../random.hlsl"
#include"../raytracing.hlsl"
#include"../raytracing_utility.hlsl"
#include"../static_sampler.hlsl"
#include"../global_constant.hlsl"
#include"../root_constant.hlsl"
#include"../bsdf/lambert.hlsl"
#include"../material/standard.hlsl"
#include"../bindless.hlsl"
#include"../environment.hlsl"
#include"../environment_sampling.hlsl"
#include"../utility.hlsl"
#include"../sampling.hlsl"

#include"gbuffer.hlsl"

#define ENABLE_NEE_EVAL		1
#define ENABLE_HIT_EVAL		1
#define INF_T				1000

float calc_mis_weight(float pdf_a, float pdf_b)
{
	return pdf_a / (pdf_a + pdf_b);
}

TextureCube<float3>						env_cube;
Texture2D<float3>						env_panorama;

RWTexture2D<float3>	diffuse_result;		//RISの全候補を使ったV=1のDiffuse結果
RWTexture2D<float3>	non_diffuse_result;	//
RWTexture2D<float>	shadowed_result;	//RISのサンプルのluminance(diffuse+non_diffuse)*Vの結果
RWTexture2D<float>	unshadowed_result;	//
RWTexture2D<uint>	subsurface_radius;
RWTexture2D<float>	subsurface_weight;
RWTexture2D<uint>	subsurface_misc;
RWTexture2D<float4>	diffuse_color;
RWTexture2D<float4>	specular_color0;
RWTexture2D<float4>	normal_roughness;
RWTexture2D<float4>	velocity;
RWTexture2D<float>	depth;

struct reservoir //ただのRIS用
{
	float	sum_weight;
	float3	sum_diffuse;
	float3	sum_non_diffuse;
	float	diffuse_weight;
	float	non_diffuse_weight;
	float3	sample_pos;
};

reservoir create_reservoir()
{
	reservoir r;
	r.sum_weight = 0;
	r.sum_diffuse = 0;
	r.sum_non_diffuse = 0;
	return r;
}

void update_reservoir(inout reservoir r, float3 sample_pos, float3 diffuse, float3 non_diffuse, inout float u)
{
	r.sum_diffuse += diffuse;
	r.sum_non_diffuse += non_diffuse;

	float weight = luminance(diffuse + non_diffuse);
	r.sum_weight += weight;

	float pmf = weight / r.sum_weight;
	if(u < pmf)
	{
		u /= pmf;
		float inv_weight = 1 / weight;
		r.diffuse_weight = luminance(diffuse) * inv_weight; //最後にsum_weightを乗算で÷PMFになる
		r.non_diffuse_weight = luminance(non_diffuse) * inv_weight;
		r.sample_pos = sample_pos;
	}
	else
	{
		u = (u - pmf) / (1 - pmf);
	}
}

void directional_lighting(float3 position, float3 wo, float3 normal, standard_material mtl, inout reservoir reservoir, inout rng rng, uint M_NEE, uint M_BSDF)
{
	if(!directional_light_enable)
		return;

	float u = randF(rng);
	for(int i = 0; i < M_NEE; i++)
	{
		float3 wi = sample_uniform_cone(directional_light_cos_max, directional_light_axis_x, directional_light_axis_y, directional_light_direction, randF(rng), randF(rng));
		if(!has_contribution(dot(normal, wi), mtl))
			continue;

		float pdf_bsdf;
		float3 diffuse, non_diffuse;
		calc_bsdf_pdf(wo, wi, normal, mtl, diffuse, non_diffuse, pdf_bsdf);

		float weight = 1 / float(M_NEE * directional_light_sample_pdf);
#if ENABLE_HIT_EVAL
		weight *= calc_mis_weight(M_NEE * directional_light_sample_pdf, M_BSDF * pdf_bsdf);
#endif
		diffuse *= directional_light_power * weight;
		non_diffuse *= directional_light_power * weight;
		update_reservoir(reservoir, wi * INF_T, diffuse, non_diffuse, u);
	}
}

void environment_lighting(float3 position, float3 wo, float3 normal, standard_material mtl, inout reservoir reservoir, inout rng rng, uint M_NEE, uint M_BSDF, uint2 dtid = 0)
{
	if(!environment_light_enable())
		return;

	uint index = rand(rng) & (environment_light_presample_count - 1);
	index = WaveReadLaneFirst(index);
	
	uint2 active_thread_count_and_offset = calc_active_thread_count_and_offset();
	index += active_thread_count_and_offset.y;

	float u = randF(rng);
	for(int i = 0; i < M_NEE; i++, index += active_thread_count_and_offset.x)
	{
		if(index >= environment_light_presample_count)
			index -= environment_light_presample_count;

		env_sample s = sample_environment_light(index);
		s.L *= environment_light_power_scale;

		float3 wi = s.w;
		if(!has_contribution(dot(normal, wi), mtl))
			continue;

		float pdf_bsdf;
		float3 diffuse, non_diffuse;
		calc_bsdf_pdf(wo, wi, normal, mtl, diffuse, non_diffuse, pdf_bsdf);

		float weight = 1 / float(M_NEE * s.pdf);
#if ENABLE_HIT_EVAL
		weight *= calc_mis_weight(M_NEE * s.pdf, M_BSDF * pdf_bsdf);
#endif
		diffuse *= s.L * weight;
		non_diffuse *= s.L * weight;
		update_reservoir(reservoir, s.w * INF_T, diffuse, non_diffuse, u);
	}
}

[numthreads(8, 4, 1)]
void trace_and_shade(uint2 dtid : SV_DispatchThreadID)
{
	if(any(dtid >= screen_size))
		return;
	
	debug_uav0[dtid] = 0;
	debug_uav1[dtid] = 0;
	debug_uav2[dtid] = 0;
	debug_uav3[dtid] = 0;

	rng rng = create_rng(dtid.x + screen_size.x * (dtid.y + screen_size.y * frame_count));

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
		float3 pos = ray.origin + ray.direction * payload.ray_t;
		float3 wo = -ray.direction;

		intersection isect = get_intersection(payload.instance_id, 0, payload.primitive_index, payload.barycentrics, payload.is_front_face);
		isect.normal = normal_correction(isect.normal, wo);

		standard_material mtl = load_standard_material(isect.material_handle, wo, isect.normal, isect.tangent.xyz, get_binormal(isect), isect.uv, 0, dtid);

		uint directional_light_M_NEE = 4; //cos_maxに依存するべき
		uint environment_light_M_NEE = 16;
		uint M_BSDF = 4;

		reservoir reservoir = create_reservoir();
#if ENABLE_NEE_EVAL
		directional_lighting(pos, wo, isect.normal, mtl, reservoir, rng, directional_light_M_NEE, M_BSDF);
		environment_lighting(pos, wo, isect.normal, mtl, reservoir, rng, environment_light_M_NEE, M_BSDF, dtid);
#endif

#if ENABLE_HIT_EVAL
		float u = randF(rng);
		for(uint i = 0; i < M_BSDF; i++)
		{
			bsdf_sample s = sample_bsdf(wo, isect.normal, mtl, randF(rng), randF(rng), randF(rng), dtid);
			if(!s.is_valid)
				continue;

			if(is_in_cone(s.w, directional_light_cos_max, directional_light_axis_x, directional_light_axis_y, directional_light_direction))
			{
				float weight = 1 / float(M_BSDF);
#if ENABLE_NEE_EVAL
				weight *= calc_mis_weight(M_BSDF * s.pdf, directional_light_M_NEE * directional_light_sample_pdf);
#endif
				float3 diffuse = s.diffuse_weight * directional_light_power * weight;
				float3 non_diffuse = s.non_diffuse_weight * directional_light_power * weight;
				update_reservoir(reservoir, s.w * INF_T, diffuse, non_diffuse, u);
			}

			if(environment_light_enable())
			{
				float weight = 1 / float(M_BSDF);
#if ENABLE_NEE_EVAL
				weight *= calc_mis_weight(M_BSDF * s.pdf, environment_light_M_NEE * sample_envmap_pdf(s.w));
#endif
				float3 L = env_cube.SampleLevel(bilinear_clamp, s.w, cube_sample_level(s.w, s.pdf, M_BSDF, environment_light_log2_texel_size)) * environment_light_power_scale;
				float3 diffuse = s.diffuse_weight * L * weight;
				float3 non_diffuse = s.non_diffuse_weight * L * weight;
				update_reservoir(reservoir, s.w * INF_T, diffuse, non_diffuse, u);
			}
		}
#endif

		float shadowed = 0;
		float unshadowed = 0;
		if(reservoir.sum_weight > 0)
		{
			unshadowed = (reservoir.diffuse_weight + reservoir.non_diffuse_weight) / reservoir.sum_weight;
			if(!is_occluded(isect.position, reservoir.sample_pos))
				shadowed = unshadowed;
		}
		shadowed_result[dtid] = shadowed;
		unshadowed_result[dtid] = unshadowed;

		float3 diff_color = get_diffuse_color(mtl);
		//diffuse_result[dtid] = reservoir.sum_diffuse;
		diffuse_result[dtid] = reservoir.sum_diffuse / (diff_color + 1e-6f); //diffuseColorはSSSSS後に乗算
		non_diffuse_result[dtid] = reservoir.sum_non_diffuse;

		float z = world_to_screen(pos).z;
		depth[dtid] = z;

		velocity[dtid] = encode_velocity(world_to_screen(isect.prev_position) - float3(dtid + 0.5f, z));
		normal_roughness[dtid] = encode_normal_roughness(isect.normal, get_diffuse_roughness(mtl), get_specular_roughness(mtl));

		diffuse_color[dtid] = encode_diffuse_color(diff_color);
		specular_color0[dtid] = encode_specular_color0(get_specular_color0(mtl));
	
		subsurface_radius[dtid] = encode_subsurface_radius(get_subsurface_radius(mtl));
		subsurface_weight[dtid] = encode_subsurface_weight(get_subsurface(mtl));
		subsurface_misc[dtid] = encode_subsurface_misc(z, dot(isect.normal, wo));
	}
	else
	{
		non_diffuse_result[dtid] = env_panorama.SampleLevel(bilinear_clamp, panorama_uv(ray.direction), 0) * environment_light_power_scale;
		subsurface_misc[dtid] = depth[dtid] = 0;
	}
}

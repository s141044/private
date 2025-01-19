
//#include"../raytracing.hlsl"
//#include"../raytracing_utility.hlsl"
//#include"../static_sampler.hlsl"
//#include"../root_constant.hlsl"
//#include"../bsdf/lambert.hlsl"
//#include"../material/standard.hlsl"
//#include"../bindless.hlsl"
//#include"../environment.hlsl"
//#include"../environment_sampling.hlsl"
//#include"../utility.hlsl"
//#include"../sampling.hlsl"


#include"gbuffer.hlsl"
#include"../random.hlsl"
#include"../utility.hlsl"
#include"../global_constant.hlsl"
#include"../material/subsurface.hlsl"

Texture2D<float3>	diffuse_result;
Texture2D<uint>		subsurface_radius;
Texture2D<float>	subsurface_weight;
Texture2D<uint>		subsurface_misc;
Texture2D<float4>	diffuse_color;
Texture2D<float4>	normal_roughness;
Texture2D<float>	depth;
RWTexture2D<float3>	result;

#define SSSSS_SAMPLE_COUNT 32

[numthreads(8, 8, 1)]
void sssss(uint2 dtid : SV_DispatchThreadID)
{
	if(any(dtid >= screen_size))
		return;

	float z, dummy;
	decode_subsurface_misc(subsurface_misc[dtid], z, dummy);
	if(z <= 0)
		return;

	//float weight = 0.1f;
	float weight = 0;
	//float weight = 1;
	//float weight = decode_subsurface_weight(subsurface_weight[dtid]);
	if(weight <= 0)
	{
		result[dtid] += decode_diffuse_color(diffuse_color[dtid]) * diffuse_result[dtid];
		return;
	}

	float3 pos = screen_to_world(dtid + 0.5f, z);
	//float dist = z_to_linear_depth(z);

	float3 normal, roughness;
	decode_normal_roughness(normal_roughness[dtid], normal, roughness.x, roughness.y);

	float3 tangent, binormal;
	calc_orthonormal_basis(normal, tangent, binormal);

	//float3 radius = 0.001;
	float3 radius = float3(1, 0.01, 0.01) * 0.001;
	//float3 radius = decode_subsurface_radius(subsurface_radius[dtid]);
	float max_radius = max(radius.x, max(radius.y, radius.z));

	float3 sum = 0, sum_weight = 0;
	for(int i = 0; i < SSSSS_SAMPLE_COUNT; i++)
	{
		float2 u = hammersley_sequence(i, SSSSS_SAMPLE_COUNT);
		float r = sample_bssrdf(max_radius, u[0]);
		float phi = 2 * PI * u[1];
		float sp = sin(phi);
		float cp = cos(phi);

		float3 sample_pos = pos + (r * cp) * tangent + (r * sp) * binormal;
		float jacobian = 1 / (dot(normal, camera_pos - sample_pos) * length(camera_pos - sample_pos));

		sample_pos = world_to_screen(sample_pos);

		float sample_z, sample_cos;
		decode_subsurface_misc(subsurface_misc[sample_pos.xy], sample_z, sample_cos);
		if(sample_z <= 0)
			continue;

		float3 col = diffuse_result[sample_pos.xy];

		sample_pos = screen_to_world(sample_pos.xy, sample_z);
		
		float d = length(pos - sample_pos);
		
		//sample_pos -= camera_pos;
		//jacobian *= sample_cos / dot(sample_pos, sample_pos);
		//float pdf = sample_bssrdf_pdf(r, max_radius) * jacobian;
		//float3 weight = calc_bssrdf(d, radius) / pdf;

		float pdf = sample_bssrdf_pdf(r, max_radius);
		float3 weight = calc_bssrdf(d, radius) / pdf;
		
		//float3 weight = 1;

		sum += weight * col;
		sum_weight += weight;
	}
	sum /= sum_weight + 1e-6f;
	sum = lerp(diffuse_result[dtid], sum, weight);
	sum *= decode_diffuse_color(diffuse_color[dtid]);

	//result[dtid] = sum;
	result[dtid] += sum;
}

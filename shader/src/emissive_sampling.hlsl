
#ifndef EMISSIVE_SAMPLING_HLSL
#define EMISSIVE_SAMPLING_HLSL

#include"utility.hlsl"
#include"packing.hlsl"

struct compressed_emissive_sample
{
	float3	position;
	uint	normal;
	uint	power;
	float	pdf_approx;
};

struct emissive_sample
{
	float3	position;
	float3	normal;
	float3	power;
	float	pdf_approx;
};

StructuredBuffer<compressed_emissive_sample>	sample_list_srv;

emissive_sample decompress(compressed_emissive_sample cs)
{
	emissive_sample s;
	s.position = cs.position;
	s.normal = oct_to_f32x3(u16x2_unorm_to_f32x2(cs.normal) * 2 - 1);
	s.power = r9g9b9e5_to_f32x3(cs.power);
	s.pdf_approx = cs.pdf_approx;
	return s;
}

compressed_emissive_sample compress(emissive_sample s)
{
	compressed_emissive_sample cs;
	cs.position = s.position;
	cs.normal = f32x2_to_u16x2_unorm(f32x3_to_oct(cs.normal) * 0.5f + 0.5f);
	cs.power = f32x3_to_r9g9b9e5(s.power);
	cs.pdf_approx = s.pdf_approx;
	return cs;
}

float emissive_sample_pdf(float power)
{
	return power / asfloat(emissive_total_power_srv.Load(4));
}

#if defined(EMISSIVE_SAMPLE)

#include"random.hlsl"
#include"sampling.hlsl"
#include"raytracing.hlsl"
#include"root_constant.hlsl"
#include"global_constant.hlsl"
#include"material/standard.hlsl"

ByteAddressBuffer								emissive_instance_list_srv;
ByteAddressBuffer								emissive_instance_list_size_srv;
RWStructuredBuffer<compressed_emissive_sample>	sample_list_uav;

[numthreads(256, 1, 1)]
void emissive_sample(uint dtid : SV_DispatchThreadID)
{
	//uint sample_count = root_constant;
	//if(dtid >= sample_count)
	//	return;
	//
	//uint instance_list_size = emissive_instance_list_size_srv.Load(0);
	//if(instance_list_size == 0)
	//	return;
	//
	//rng rng = create_rng(dtid + sample_count * frame_count);
	//float u0 = randF(u0);
	//
	//uint blas_info_xy;
	//float sum_weight = 0;
	//float sample_weight;
	//
	//const uint M = 4;
	//for(uint i = 0; i < M; i++)
	//{
	//	uint index = rand(rng) % instance_list_size;
	//	uint2 blas_info = emissive_instance_list_srv.Load(8 * index);
	//	
	//	float weight = asfloat(blas_info.y);
	//	sum_weight += weight;
	//
	//	float pmf = weight / sum_weight;
	//	if(u0 < pmf)
	//	{
	//		u0 /= pmf;
	//		blas_info_xy = blas_info.xy;
	//		sample_weight = weight;
	//	}
	//	else
	//	{
	//		u0 = (pmf - u0) / (1 - u0);
	//	}
	//}
	//
	//uint raytracing_instance_index = blas_info_xy[0] & 0xffff;
	//uint bindless_instance_index = blas_info_xy[0] >> 16;
	//uint primitive_count = blas_info_xy[1];
	//uint primitive_index = u0 * primitive_count;
	//float3 bary = sample_uniform_triangle(randF(rng), randF(rng));
	//
	//intersection isect = get_intersection(bindless_instance_index, 0, primitive_index, bary.yz, true);
	//
	//standard_material load_standard_material(uint handle, float3 wo, float3 normal, float3 tangent, float3 binormal, float2 uv, float lod = 0, uint2 dtid = 0)

}

#endif
#endif

#ifndef EMISSIVE_SAMPLING_HLSL
#define EMISSIVE_SAMPLING_HLSL

#include"utility.hlsl"
#include"packing.hlsl"
#include"sampling.hlsl"
#include"raytracing.hlsl"
#include"emissive_utility.hlsl"
#include"material/standard.hlsl"

struct compressed_emissive_sample
{
	float3	position;
	uint	normal;
	uint	L;
	float	pdf_approx;
};

struct emissive_sample
{
	float3	position;
	float3	normal;
	float3	L;
	float	pdf_approx;
};

ByteAddressBuffer								emissive_tlas_srv;
StructuredBuffer<compressed_emissive_sample>	emissive_sample_srv;
ByteAddressBuffer								emissive_blas_handle_srv;

emissive_sample decompress(compressed_emissive_sample cs)
{
	emissive_sample s;
	s.position = cs.position;
	s.normal = oct_to_f32x3(u16x2_unorm_to_f32x2(cs.normal) * 2 - 1);
	s.L = r9g9b9e5_to_f32x3(cs.L);
	s.pdf_approx = cs.pdf_approx;
	return s;
}

compressed_emissive_sample compress(emissive_sample s)
{
	compressed_emissive_sample cs;
	cs.position = s.position;
	cs.normal = f32x2_to_u16x2_unorm(f32x3_to_oct(cs.normal) * 0.5f + 0.5f);
	cs.L = f32x3_to_r9g9b9e5(s.L);
	cs.pdf_approx = s.pdf_approx;
	return cs;
}

bool exists_emissive()
{
	return (load_emissive_tlas_header(emissive_tlas_srv).blas_count > 0);
}

float emissive_sample_pdf(float3 L)
{
	return luminance(L) * load_emissive_tlas_header(emissive_tlas_srv).inv_total_power;
}

emissive_sample sample_emissive(float u0, float u1, uint dtid = 0)
{
	emissive_tlas_header tlas_header = load_emissive_tlas_header(emissive_tlas_srv);

	uint index = u0 * tlas_header.blas_count;
	u0 = frac(u0 * tlas_header.blas_count);

	emissive_bucket bucket = emissive_tlas_srv.Load<emissive_bucket>(EMISSIVE_TLAS_HEADER_SIZE + 8 * index);
	if(u0 >= bucket.weight)
		index = bucket.alias; //handle‚ª“ü‚Á‚Ä‚¢‚é
	else
		index = emissive_blas_handle_srv.Load(4 * index);

	ByteAddressBuffer blas = get_byteaddress_buffer(index);
	emissive_blas_header blas_header = load_emissive_blas_header(blas);

	index = u1 * blas_header.primitive_count;
	u1 = frac(u1 * blas_header.primitive_count);

	bucket = blas.Load<emissive_bucket>(EMISSIVE_BLAS_HEADER_SIZE + 8 * index);
	if(u1 >= bucket.weight)
		index = bucket.alias;

	float2 b12 = sample_uniform_triangle(u0, u1).yz;
	intersection isect = get_intersection(blas_header.raytracing_instance_index, blas_header.bindless_instance_index, 0, index, b12, true);

	emissive_sample s;
	s.position = isect.position;
	s.normal = isect.normal;

	standard_material mtl = load_standard_material(isect.material_handle, isect.normal, isect.normal, isect.tangent.xyz, get_binormal(isect), isect.uv);
	s.L = get_emissive_color(mtl);
	s.pdf_approx = emissive_sample_pdf(s.L);
	return s;
}

#if defined(EMISSIVE_PRESAMPLE)

#include"random.hlsl"
#include"root_constant.hlsl"
#include"global_constant.hlsl"

RWStructuredBuffer<compressed_emissive_sample>	emissive_sample_uav;

[numthreads(256, 1, 1)]
void emissive_presample(uint dtid : SV_DispatchThreadID)
{
	uint sample_count = root_constant;
	if(dtid >= sample_count)
		return;

	rng rng;
	rng.state = dtid + sample_count * frame_count;
	float u = randF(rng);

	float sum_weight = 0;
	emissive_sample s;

	const uint M = 4;
	for(uint i = 0; i < M; i++)
	{
		emissive_sample candidate = sample_emissive(randF(rng), randF(rng), dtid);

		float weight = luminance(candidate.L);
		if(weight <= 0)
			continue;

		sum_weight += weight;
		
		float pmf = weight / sum_weight;
		if(u < pmf)
		{
			u /= pmf;
			s = candidate;
			s.L /= weight; //ÅŒã‚Ésum_weight‚ðæŽZ‚µ‚Ä€pmf‚ðŠ®¬
		}
		else
		{
			u = (u - pmf) / (1 - pmf);
		}
	}

	if(sum_weight > 0)
		s.L *= sum_weight;
	else
		s = (emissive_sample)0;

	emissive_sample_uav[dtid] = compress(s);
}

#endif
#endif

#ifndef ENVIRONMENT_SAMPLING_HLSL
#define ENVIRONMENT_SAMPLING_HLSL

#include"packing.hlsl"
#include"environment.hlsl"
#include"static_sampler.hlsl"

ByteAddressBuffer		envmap_cdf;
Texture2DArray<float>	envmap_weight;
Texture2DArray<float>	envmap_weight1;
Texture2DArray<float>	envmap_weight2;
Texture2DArray<float>	envmap_weight3;
Texture2DArray<float>	envmap_weight4;
Texture2DArray<float>	envmap_weight5;
Texture2DArray<float>	envmap_weight6;
Texture2DArray<float>	envmap_weight7;
Texture2DArray<float>	envmap_weight8;
Texture2DArray<float>	envmap_weight9;

struct env_sample
{
	float3	L;
	float3	w;
	float	pdf;
};

struct compressed_env_sample
{
	uint	L;
	uint	w;
	float	pdf;
};

env_sample decompress(compressed_env_sample cs)
{
	env_sample s;
	s.L = r9g9b9e5_to_f32x3(cs.L);
	s.w = oct_to_f32x3(u16x2_unorm_to_f32x2(cs.w) * 2 - 1);
	s.pdf = cs.pdf;
	return s;
}

compressed_env_sample compress(env_sample s)
{
	compressed_env_sample cs;
	cs.L = f32x3_to_r9g9b9e5(s.L);
	cs.w = f32x2_to_u16x2_unorm(f32x3_to_oct(s.w) * 0.5f + 0.5f);
	cs.pdf = s.pdf;
	return cs;
}

struct envmap_face_cdf
{
	float val[6];
};

float4 sample_envmap(float u0, float u1)
{
	envmap_face_cdf cdf = envmap_cdf.Load<envmap_face_cdf>(0);
	u0 *= cdf.val[5];

	int face;
	[unroll] for(int i = 0; i < 6; i++)
	{
		if((i == 5) || (u0 < cdf.val[i]))
		{
			float min_val = (i > 0) ? cdf.val[i - 1] : 0;
			u0 = (u0 - min_val) / (cdf.val[i] - min_val);
			face = i;
			break;
		}
	}

	uint width, height, depth, mip_count;
	envmap_weight.GetDimensions(0, width, height, depth, mip_count);

	float weight;
	float2 pos = 0;
	[unroll] for(uint i = 1; i < 10; i++)
	{
		if(i >= mip_count)
			break;

		pos *= 2;
		float2 uv = (pos + 1) / (1u << i);

		float4 weights;
		if(i == 1) weights = envmap_weight1.GatherRed(bilinear_clamp, float3(uv, face)).wzxy;
		else if(i == 2) weights = envmap_weight2.GatherRed(bilinear_clamp, float3(uv, face)).wzxy;
		else if(i == 3) weights = envmap_weight3.GatherRed(bilinear_clamp, float3(uv, face)).wzxy;
		else if(i == 4) weights = envmap_weight4.GatherRed(bilinear_clamp, float3(uv, face)).wzxy;
		else if(i == 5) weights = envmap_weight5.GatherRed(bilinear_clamp, float3(uv, face)).wzxy;
		else if(i == 6) weights = envmap_weight6.GatherRed(bilinear_clamp, float3(uv, face)).wzxy;
		else if(i == 7) weights = envmap_weight7.GatherRed(bilinear_clamp, float3(uv, face)).wzxy;
		else if(i == 8) weights = envmap_weight8.GatherRed(bilinear_clamp, float3(uv, face)).wzxy;
		else if(i == 9) weights = envmap_weight9.GatherRed(bilinear_clamp, float3(uv, face)).wzxy;

		float pmf = (weights[0] + weights[2]) / (weights[0] + weights[1] + weights[2] + weights[3]);
		if(u0 < pmf)
		{
			u0 /= pmf;
			pmf = weights[0] / (weights[0] + weights[2]);
			weights[3] = weights[2];
		}
		else
		{
			pos.x++;
			u0 = (u0 - pmf) / (1 - pmf);
			pmf = weights[1] / (weights[1] + weights[3]);
			weights[0] = weights[1];
		}

		if(u1 < pmf)
		{
			u1 /= pmf;
			weight = weights[0];
		}
		else
		{
			pos.y++;
			u1 = (u1 - pmf) / (1 - pmf);
			weight = weights[3];
		}
	}

	pos.x += u0;
	pos.y += u1;

	float inv_res = 1 / float(width);
	float3 w = cubemap_direction(pos.x, pos.y, face, inv_res);
	float len2 = dot(w, w);
	float len = sqrt(len2);
	w /= len;

	float pdf = weight / cdf.val[5];
	pdf *= len2 * len;
	pdf *= pow2(width / 2);
	return float4(w, pdf);
}

float sample_envmap_pdf(float3 w)
{
	float3 uv_face = cubemap_uv_face(w);
	uv_face.x = +uv_face.x * 0.5f + 0.5f;
	uv_face.y = -uv_face.y * 0.5f + 0.5f;

	float weight = envmap_weight.SampleLevel(point_clamp, uv_face, 0);
	float pdf = weight / asfloat(envmap_cdf.Load(4 * 5));

	float3 abs_w = abs(w);
	w /= max(abs_w.x, max(abs_w.y, abs_w.z));
	float len2 = dot(w, w);
	pdf *= len2 * sqrt(len2);

	float width, height, depth;
	envmap_weight.GetDimensions(width, height, depth);
	pdf *= pow2(width / 2);
	return pdf;
}

#if defined(INITIALIZE_WEIGHT)

#include"packing.hlsl"
#include"root_constant.hlsl"

TextureCube<float3>		src;
RWTexture2DArray<float>	dst;

[numthreads(16, 16, 1)]
void initialize_weight(int3 dtid : SV_DispatchThreadID)
{
	uint res = root_constant >> 16;
	float inv_res = 1 / float(res);
	float u = +((dtid.x + 0.5f) * inv_res * 2 - 1);
	float v = -((dtid.y + 0.5f) * inv_res * 2 - 1);
	float3 d = cubemap_direction(u, v, dtid.z);
	float inv_len = rsqrt(dot(d, d));

	float offset = inv_res / 2;
	dst[dtid] = luminance(
		src.SampleLevel(bilinear_clamp, cubemap_direction(u + offset, v + offset, dtid.z), root_constant & 0xffff) +
		src.SampleLevel(bilinear_clamp, cubemap_direction(u - offset, v + offset, dtid.z), root_constant & 0xffff) +
		src.SampleLevel(bilinear_clamp, cubemap_direction(u + offset, v - offset, dtid.z), root_constant & 0xffff) +
		src.SampleLevel(bilinear_clamp, cubemap_direction(u - offset, v - offset, dtid.z), root_constant & 0xffff)) * (pow3(inv_len) / 4);
}

#elif defined(CALCULATE_CDF)

Texture2DArray<float>	src;
RWByteAddressBuffer		dst;

[numthreads(1, 1, 1)]
void calculate_cdf()
{
	envmap_face_cdf	cdf;
	cdf.val[0] = src[int3(0, 0, 0)];
	cdf.val[1] = src[int3(0, 0, 1)] + cdf.val[0];
	cdf.val[2] = src[int3(0, 0, 2)] + cdf.val[1];
	cdf.val[3] = src[int3(0, 0, 3)] + cdf.val[2];
	cdf.val[4] = src[int3(0, 0, 4)] + cdf.val[3];
	cdf.val[5] = src[int3(0, 0, 5)] + cdf.val[4];
	dst.Store<envmap_face_cdf>(0, cdf);
}

#elif defined(PRESAMPLE)

#include"random.hlsl"
#include"packing.hlsl"
#include"root_constant.hlsl"

TextureCube<float3>							envmap;
RWStructuredBuffer<compressed_env_sample>	samples;

[numthreads(256, 1, 1)]
void presample(int dtid : SV_DispatchThreadID)
{
	uint sample_count = root_constant >> 16;
	if(dtid >= sample_count)
		return;

	float2 u = hammersley_sequence(dtid, sample_count);
	float4 w_pdf = sample_envmap(u[0], u[1]);

	env_sample s;
	s.L = envmap.SampleLevel(bilinear_clamp, w_pdf.xyz, root_constant & 0xffff);
	s.w = w_pdf.xyz;
	s.pdf = w_pdf.w;

	samples[dtid] = compress(s);
}

#endif
#endif
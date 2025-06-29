
#ifndef BSDF_GLINT_HLSL
#define BSDF_GLINT_HLSL

#include"../bsdf.hlsl"
#include"../random.hlsl"
#include"../debug.hlsl"
#include"../static_sampler.hlsl"

///////////////////////////////////////////////////////////////////////////////////////////////////

Texture2D<float2>		glint_reflectance_table;
Texture1DArray<float>	glint_sdf_dictionary_srv;
Texture1DArray<float>	glint_cdf_dictionary_srv;

///////////////////////////////////////////////////////////////////////////////////////////////////

#define GLINT_MIP_LEVELS		16
#define GLINT_TABLE_RES			64
#define GLINT_TABLE_RES_LOG2	6
#define GLINT_ALPHA_DICT		0.5
#define GLINT_DICT_SIZE			128
#define GLINT_TABLE_MAX			(GLINT_ALPHA_DICT * 4 * 0.707107) //(ALPHA_DICT * 4 / sqrt(2))

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace glint{

///////////////////////////////////////////////////////////////////////////////////////////////////

float G1(float3 w, float3 wm)
{
	float w_wm = dot(w, wm);
	return (w_wm > 0) ? min(1, 2 * wm.z * w.z / w_wm) : 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

float G(float3 wo, float3 wi, float3 wm)
{
	return G1(wo, wm) * G1(wi, wm);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

float beckmann_D(float3 w, float alpha)
{
	float cos2 = w.z * w.z;
	float sin2 = 1 - cos2;
	float tan2 = sin2 / cos2;
	float a2 = alpha * alpha;
	return exp(-tan2 / a2) / (PI * a2 * cos2 * cos2);
}

float3 sample_beckmann(float alpha, float u0, float u1)
{
	float tan2 = -alpha * alpha * log(1 - u0);
	float phi = 2 * PI * u1;
	float ct = 1 / sqrt(1 + tan2);
	float st = sqrt(1 - ct * ct);
	return float3(st * sin(phi), st * cos(phi), ct);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void init_rng(int x, int y, int lod, out rng rng)
{
	//lod0: -1->-1, 0->0, 1->1
	//lod1: -1->-2, 0->0, 1->2
	//lod2: -1->-4, 0->0, 1->4
	rng.state = asuint(x * (int(1) << lod)) + asuint(y * (int(1) << lod)) * 123456;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

float sample_sdf(uint array_index, float u)
{
	uint beg = 0;                   //cdf[beg]<=u
	uint end = GLINT_TABLE_RES + 1; //cdf[end]>u

	for(uint i = 0; i < GLINT_TABLE_RES_LOG2 + 1; i++)
	{
		uint mid = (beg + end) / 2;
		float cdf = glint_cdf_dictionary_srv[uint2(mid, array_index)];

		if(cdf > u)
			end = mid;
		else
			beg = mid;
		
		if(end - beg <= 1)
			break;
	}

	float cdf0 = 0;
	if(beg > 0) 
		cdf0 = glint_cdf_dictionary_srv[uint2(beg + 0, array_index)];

	float cdf1 = 1;
	if(beg < GLINT_TABLE_RES) 
		cdf1 = glint_cdf_dictionary_srv[uint2(beg + 1, array_index)];

	u = (u - cdf0) / (cdf1 - cdf0);

	float ret;
	if(beg == 0)
	{
		ret = u / 2;
	}
	else if(beg == GLINT_TABLE_RES)
	{
		ret = beg - 0.5 + u / 2;
	}
	else
	{
		float a = glint_sdf_dictionary_srv[uint2(beg - 1, array_index)];
		float b = glint_sdf_dictionary_srv[uint2(beg + 0, array_index)];
		if(a == b)
			ret = beg - 0.5 + u;
		else
		{
			float A = b - a;
			float B = a;
			float C = -u * (a + b);
			float sqrt_D = sqrt(B * B - A * C);
			float t = (-B + sqrt_D) / A;
			ret = beg - 0.5 + t;
		}
	}
	
	float texel_size = 1 / float(GLINT_TABLE_RES);
	return ret * texel_size * GLINT_TABLE_MAX;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void calc_brdf_pdf(float3 wo, float3 wi, float3 color0, float roughness, float2 patch_center, float2 patch_axis0, float2 patch_axis1, float cell_size0, float lod_bias, out float3 brdf, out float pdf, out float3 weight, uint2 dtid = 0)
{
	float patch_size0 = length(patch_axis0);
	float patch_size1 = length(patch_axis1);
	float patch_size = (patch_size0 + patch_size1) / 2;
	float lod = max(0, log2(patch_size / cell_size0) - lod_bias);

	float3 h = normalize(wo + wi);
	float alpha = pow2(roughness);
	float D;

#if defined(FIRST)
	if(lod >= GLINT_MIP_LEVELS)
#else
	if(1)
#endif
	{
		D = beckmann_D(h, alpha);
	}
	else
	{
		float2 slope = -h.xy / h.z;

		float sdfs[2] = {0, 0};
		for(int i = 0; i < 2; i++)
		{
			float cell_size = cell_size0 * (int(1) << (int(lod) + i));
			float inv_cell_size = 1 / cell_size;

			int2 beg, end;
			beg.x = floor((patch_center.x - abs(patch_axis0.x) - abs(patch_axis1.x)) * inv_cell_size + 0.5);
			end.x = floor((patch_center.x + abs(patch_axis0.x) + abs(patch_axis1.x)) * inv_cell_size - 0.5);
			beg.y = floor((patch_center.y - abs(patch_axis0.y) - abs(patch_axis1.y)) * inv_cell_size + 0.5);
			end.y = floor((patch_center.y + abs(patch_axis0.y) + abs(patch_axis1.y)) * inv_cell_size - 0.5);

			float sum_weight = 0;
			for(int y = beg.y; y <= end.y; y++)
			{
				for(int x = beg.x; x <= end.x; x++)
				{
					float2 p;
					p.x = cell_size * (x + 0.5);
					p.y = cell_size * (y + 0.5);

					float2 diff;
					diff.x = dot(p - patch_center, patch_axis0);
					diff.y = dot(p - patch_center, patch_axis1);

					float diff2 = dot(diff, diff);
					if(diff2 > 1){ continue; }

					rng rng;
					init_rng(x, y, lod, rng);

					float2 slope_uv = slope / GLINT_TABLE_MAX;
					float theta = randF(rng) * 2 * PI;
					float ct = cos(theta);
					float st = sin(theta);
					slope_uv = mul(slope_uv, float2x2(ct, -st, st, ct));
					slope_uv *= (GLINT_ALPHA_DICT / alpha);
		
					uint random = rand(rng);
					uint dict_x = GLINT_MIP_LEVELS * ((random & 0xffff) % GLINT_DICT_SIZE) + uint(lod) + i;
					uint dict_y = GLINT_MIP_LEVELS * ((random >>    16) % GLINT_DICT_SIZE) + uint(lod) + i;

					float sdf = pow2(GLINT_ALPHA_DICT / alpha) * (
						glint_sdf_dictionary_srv.SampleLevel(bilinear_clamp, float2(abs(slope_uv.x), dict_x), 0) *
						glint_sdf_dictionary_srv.SampleLevel(bilinear_clamp, float2(abs(slope_uv.y), dict_y), 0));

					sdf *= pow2(GLINT_TABLE_RES / GLINT_TABLE_MAX); //unormで保存するためにした変換を打ち消す

					float w = exp(-diff2 / pow2(patch_size / 2));
					sdfs[i] += w * sdf;
					sum_weight += w;
				}
			}
			if(sum_weight > 0)
				sdfs[i] /= sum_weight;
		}

		float sdf = lerp(sdfs[0], sdfs[1], frac(lod));
		D = sdf / pow4(h.z);
	}

	float3 F = schlick_fresnel(dot(h, wi), color0);
	float G = glint::G(wo, wi, h);
	float G1 = glint::G1(wo, h);
	float common = D / (4 * wo.z);

	weight = F * G / G1;
	brdf = F * G * common;
	pdf = G1 * common;
}

float4 calc_brdf_pdf(float3 wo, float3 wi, float3 color0, float roughness, float2 patch_center, float2 patch_axis0, float2 patch_axis1, float cell_size, float lod_bias, uint2 dtid = 0)
{
	float3 weight;
	float4 brdf_pdf;
	calc_brdf_pdf(wo, wi, color0, roughness, patch_center, patch_axis0, patch_axis1, cell_size, lod_bias, brdf_pdf.xyz, brdf_pdf.w, weight, dtid);
	return brdf_pdf;
}

float3 calc_weight(float3 wo, float3 wi, float3 color0, float roughness, float2 patch_center, float2 patch_axis0, float2 patch_axis1, float cell_size, float lod_bias, uint2 dtid = 0)
{
	float3 weight;
	float4 brdf_pdf;
	calc_brdf_pdf(wo, wi, color0, roughness, patch_center, patch_axis0, patch_axis1, cell_size, lod_bias, brdf_pdf.xyz, brdf_pdf.w, weight, dtid);
	return weight;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

bool sample_brdf(float3 wo, out float3 wi, float roughness, float2 patch_center, float2 patch_axis0, float2 patch_axis1, float cell_size0, float lod_bias, float u0, float u1)
{
	float patch_size0 = length(patch_axis0);
	float patch_size1 = length(patch_axis1);
	float patch_size = (patch_size0 + patch_size1) / 2;
	float lod = max(0, log2(patch_size / cell_size0) - lod_bias);

	float alpha = pow2(roughness);
	float3 h;

#if defined(FIRST)
	if(lod >= GLINT_MIP_LEVELS)
#else
	if(1)
#endif
	{
		h = sample_beckmann(alpha, u0, u1);
	}
	else
	{
		int sample_lod;
		float pmf = 1 - frac(lod);
		if(u0 < pmf)
		{
			sample_lod = lod;
			u0 /= pmf;
		}
		else
		{
			sample_lod = lod + 1;
			u0 = (u0 - pmf) / (1 - pmf);
		}

		float cell_size = cell_size0 * (int(1) << sample_lod);
		float inv_cell_size = 1 / cell_size;

		int2 beg, end;
		beg.x = floor((patch_center.x - abs(patch_axis0.x) - abs(patch_axis1.x)) * inv_cell_size + 0.5);
		end.x = floor((patch_center.x + abs(patch_axis0.x) + abs(patch_axis1.x)) * inv_cell_size - 0.5);
		beg.y = floor((patch_center.y - abs(patch_axis0.y) - abs(patch_axis1.y)) * inv_cell_size + 0.5);
		end.y = floor((patch_center.y + abs(patch_axis0.y) + abs(patch_axis1.y)) * inv_cell_size - 0.5);

		int2 sample_cell;
		float sum_weight = 0;
		for(int y = beg.y; y <= end.y; y++)
		{
			for(int x = beg.x; x <= end.x; x++)
			{
				float2 p;
				p.x = cell_size * (x + 0.5);
				p.y = cell_size * (y + 0.5);

				float2 diff;
				diff.x = dot(p - patch_center, patch_axis0);
				diff.y = dot(p - patch_center, patch_axis1);

				float diff2 = dot(diff, diff);
				if(diff2 > 1){ continue; }

				float w = exp(-diff2 / pow2(patch_size / 2));
				sum_weight += w;

				float pmf = w / sum_weight;
				if(u1 < pmf)
				{
					u1 /= pmf;
					sample_cell = int2(x, y);
				}
				else
				{
					u1 = (u1 - pmf) / (1 - pmf);
				}

				float u = u1;
				u1 = u0;
				u0 = u;
			}
		}
		if(sum_weight == 0)
			return false;

		rng rng;
		init_rng(sample_cell.x, sample_cell.y, lod, rng);

		float theta = randF(rng) * 2 * PI;
		uint random = rand(rng);
		uint dict_x = GLINT_MIP_LEVELS * ((random & 0xffff) % GLINT_DICT_SIZE) + sample_lod;
		uint dict_y = GLINT_MIP_LEVELS * ((random >>    16) % GLINT_DICT_SIZE) + sample_lod;

		float2 slope_uv;
		slope_uv.x = sample_sdf(dict_x, u0);
		slope_uv.y = sample_sdf(dict_y, u1);

		slope_uv *= (alpha / GLINT_ALPHA_DICT);
		
		float ct = cos(theta);
		float st = sin(theta);
		slope_uv = mul(slope_uv, float2x2(ct, st, -st, ct));
		
		h = normalize(float3(-slope_uv, 1));
	}

	float3 h_ = float3(-h.xy, h.z);
	float hwo = saturate(dot(h, wo));
	float hwo_ = saturate(dot(h_, wo));
	float proj_area = hwo / (hwo + hwo_);
	if(proj_area < frac(u0 + u1))
		h = h_;

	wi = reflect(-wo, h);
	return (wi.z >= cosine_threshold);
}

bool sample_brdf(float3 wo, out float3 wi, float3 color0, float roughness, float2 patch_center, float2 patch_axis0, float2 patch_axis1, float cell_size, float lod_bias, float u0, float u1)
{
	return sample_brdf(wo, wi, roughness, patch_center, patch_axis0, patch_axis1, cell_size, lod_bias, u0, u1);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

float3 calc_reflectance(float3 wo, float3 color0, float roughness)
{
	float2 vals = glint_reflectance_table.SampleLevel(bilinear_clamp, float2(wo.z, roughness), 0);
	return color0 * vals.x + (1 - color0) * vals.y;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace glint

///////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(CALC_DICTIONARY)

#include"../random.hlsl"

RWTexture1DArray<float>	sdf_dictionary_uav;
RWTexture1DArray<float>	cdf_dictionary_uav;
groupshared uint		sdf_table[GLINT_MIP_LEVELS][GLINT_TABLE_RES + 1];
groupshared float		cdf_table[GLINT_MIP_LEVELS][GLINT_TABLE_RES + 1];

uint to_fixed_point(float f)
{
	//1<<(GLINT_MIP_LEVELS-1)回だけ加算できればいい
	//GLINT_MIP_LEVELS=16なら0~1を131071分割
	return f * (0xffffffff >> (GLINT_MIP_LEVELS - 1)) * 0.5;
}

float to_floating_point(uint u)
{
	return u / float(0xffffffff >> (GLINT_MIP_LEVELS - 1));
}

[numthreads(GLINT_TABLE_RES, GLINT_MIP_LEVELS, 1)]
void calc_dictionary(uint gid : SV_GroupID, uint2 gtid : SV_GroupThreadID, uint group_index : SV_GroupIndex)
{
	uint lane_index = WaveGetLaneIndex();
	uint lane_count = WaveGetLaneCount();
	uint wave_index = group_index / lane_count;
	
	sdf_table[gtid.y][gtid.x] = 0;
	GroupMemoryBarrierWithGroupSync();

	rng rng;
	rng.state = gid * GLINT_TABLE_RES * GLINT_MIP_LEVELS + group_index;
	
	//テーブルを埋める
	uint sample_count = 1 << (GLINT_MIP_LEVELS - 1);
	for(uint i = group_index; i < sample_count; i += GLINT_MIP_LEVELS * GLINT_TABLE_RES)
	{
		float u0 = randF(rng);
		float u1 = randF(rng);
		float3 normal = glint::sample_beckmann(GLINT_ALPHA_DICT, u0, u1);
		float slope_x = -normal.x / normal.z;

		float sigma = 0.01;
		float texel_size = 1 / float(GLINT_TABLE_RES);
		float radius = 3 * sigma;
	
		float p = abs(slope_x) / GLINT_TABLE_MAX;
		int beg = (p - radius) * GLINT_TABLE_RES + 0.5;
		int end = (p + radius) * GLINT_TABLE_RES - 0.5;
		beg = min(beg, GLINT_TABLE_RES - 1);
		end = min(end, GLINT_TABLE_RES - 1);
	
		uint level;
		for(level = 0; level < GLINT_MIP_LEVELS; level++)
		{
			if(i < (1u << level))
				break;
		}
	
		for(int j = beg; j <= end; j++)
		{
			float q = (j + 0.5) / GLINT_TABLE_RES;
			float v = exp(-pow2((p - q) / sigma));
			InterlockedAdd(sdf_table[level][abs(j)], to_fixed_point(v));
		}
	}
	GroupMemoryBarrierWithGroupSync();
	
	//低LODのサンプルのサンプルを高LODに共有
	for(uint level = 1; level < GLINT_MIP_LEVELS; level++)
	{
		if(group_index < GLINT_TABLE_RES)
			sdf_table[level][group_index] += sdf_table[level - 1][group_index];
		GroupMemoryBarrierWithGroupSync();
	}

	//重み計算(各区間の積分値)
	float sdf_a = to_floating_point(sdf_table[gtid.y][gtid.x]);
	float sdf_b = to_floating_point(sdf_table[gtid.y][min(gtid.x + 1, GLINT_TABLE_RES - 1)]);
	cdf_table[gtid.y][gtid.x] = ((gtid.x == 0) ? sdf_a : (sdf_a + sdf_b)) / 2;
	if(gtid.x == GLINT_TABLE_RES - 1)
		cdf_table[gtid.y][gtid.x + 1] = sdf_b / 2;
	GroupMemoryBarrierWithGroupSync();
	
	//積分値計算
	if(wave_index < GLINT_MIP_LEVELS)
	{
		float sum = cdf_table[wave_index][0];
		for(uint i = lane_index + 1; i <= GLINT_TABLE_RES; i += lane_count)
		{
			float val = cdf_table[wave_index][i];
			sum += WavePrefixSum(val) + val;

			cdf_table[wave_index][i] = sum;
			sum = WaveReadLaneAt(sum, lane_count - 1);
		}
	}
	GroupMemoryBarrierWithGroupSync();

	//保存
	float inv_sum = 1 / cdf_table[gtid.y][GLINT_TABLE_RES];
	sdf_dictionary_uav[uint2(gtid.x, gtid.y + GLINT_MIP_LEVELS * gid)] = sdf_a * inv_sum;
	cdf_dictionary_uav[uint2(gtid.x + 1, gtid.y + GLINT_MIP_LEVELS * gid)] = cdf_table[gtid.y][gtid.x] * inv_sum;
}

#endif

///////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(CALC_REFLECTANCE)

RWTexture2D<float2>	reflectance_table;
groupshared	float2	shared_sum[32];

[numthreads(1024, 1, 1)]
void calc_reflectance(uint group_index : SV_GroupIndex, uint2 gid : SV_GroupID)
{
	float2 patch_center = 0;
	float2 patch_axis0 = 0;
	float2 patch_axis1 = 0;
	float cell_size = 0;
	float lod_bias = 0;

	float nv = (gid.x + 0.5f) / 32;
	float roughness = (gid.y + 0.5f) / 32;
	float3 wo = float3(sqrt(1 - nv * nv), 0, nv);

	float2 sum = 0;

	uint N = 4;
	for(uint i = 0; i < N; i++)
	{
		float2 u = hammersley_sequence(N * group_index + i, N * 1024);

		float3 wi;
		if(!glint::sample_brdf(wo, wi, 1, roughness, patch_center, patch_axis0, patch_axis1, cell_size, lod_bias, u[0], u[1]))
			continue;
		float val = glint::calc_weight(wo, wi, 1, roughness, patch_center, patch_axis0, patch_axis1, cell_size, lod_bias).x;
		sum += val * float2(1, pow5(1 - dot(normalize(wo + wi), wi)));
	}

	sum = WaveActiveSum(sum);
	if(WaveGetLaneIndex() == 0)
		shared_sum[group_index / 32] = sum;

	GroupMemoryBarrierWithGroupSync();

	if(group_index < 32)
	{
		sum = WaveActiveSum(shared_sum[group_index]);
		if(WaveGetLaneIndex() == 0)
			reflectance_table[gid] = sum / (N * 1024);
	}
}

#endif

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

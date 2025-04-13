
#ifndef RENDERER_REALISTIC_CAMERA_HPP
#define RENDERER_REALISTIC_CAMERA_HPP

#include"../bsdf.hlsl"
#include"../spectrum.hlsl"
#include"../static_sampler.hlsl"
#include"../global_constant.hlsl"

RWTexture2D<float4> debug_uav0;
RWTexture2D<float4> debug_uav1;
RWTexture2D<float4> debug_uav2;
RWTexture2D<float4> debug_uav3;

struct realistic_camera_interface
{
	float aperture_radius;
	float curvature_radius;
	float thickness;
	float A[9];
};

cbuffer realistic_camera_cb
{
	float	realistic_camera_delta_z;
	float	realistic_camera_delta_r;
	float	realistic_camera_inv_delta_r;
	uint	realistic_camera_interface_count;
	float2	realistic_camera_sensor_min;
	float2	realistic_camera_sensor_size;
	float	realistic_camera_closure_ratio;
	float3	realistic_camera_padding;
};

ByteAddressBuffer								realistic_camera_pupil_srv;
Texture2D<float>								realistic_camera_aperture_srv;
StructuredBuffer<realistic_camera_interface>	realistic_camera_interface_srv;

uint2 encode(float2 pos, float aperture_radius)
{
	pos = ((pos / aperture_radius) + 1) / 2;
	return (pos * 0x7fffff + 0.5f);
}

float4 decode(uint4 enc, float aperture_radius)
{
	float4 pos = (enc / float(0x7fffff)) * 2 - 1;
	return pos * aperture_radius;
}

float calc_ior(realistic_camera_interface iface, float lambda)
{
	lambda *= 0.001;
	const float lambda2 = lambda * lambda;
	const float inv_lambda2 = 1 / lambda2;
	return sqrt(iface.A[0] + 
		lambda2 * (iface.A[1] + 
		lambda2 * (iface.A[2])) + 
		inv_lambda2 * (iface.A[3] + 
		inv_lambda2 * (iface.A[4] + 
		inv_lambda2 * (iface.A[5] + 
		inv_lambda2 * (iface.A[6] + 
		inv_lambda2 * (iface.A[7] + 
		inv_lambda2 * (iface.A[8])))))));
}

bool intersect(const float cz, const float r, const float ar, const float3 o, const float3 d, inout float t)
{
	const float3 co = float3(o.x, o.y, o.z - cz);
	const float B = dot(d, co);
	const float C = dot(co, co) - r * r;
	const float D = B * B - C;
	if(D <= 0)
		return false;

	const float sqrt_D = sqrt(D);
	const float t0 = -B - sqrt_D;
	const float t1 = -B + sqrt_D;
	t = (t0 > -1e-6f) ? t0 : t1;

	const float2 p = o.xy + t * d.xy;
	return (dot(p, p) < ar * ar);
}

bool intersect(const float cz, const float ar, const float3 o, const float3 d, inout float t, float2 axis_x = 0, float2 axis_y = 0)
{
	t = (cz - o.z) / d.z;

	float2 p = o.xy + t * d.xy;
	bool hit = (dot(p, p) >= ar * ar);
#if !defined(UNUSE_APERTURE_TEX)
	if(!hit)
	{
		p = (p.x * axis_x + p.y * axis_y) / ar;
		p = p * 0.5 + 0.5;
		hit = realistic_camera_aperture_srv.SampleLevel(bilinear_clamp, p, 0);
	}
#endif
	return hit;
}

bool trace(inout float3 o, inout float3 d, inout float throughput, float lambda, float2 axis_x = 0, float2 axis_y = 0)
{
	float ior = 1;
	for(uint i = 0; i < realistic_camera_interface_count; i++)
	{
		const realistic_camera_interface iface = realistic_camera_interface_srv[i];
		const float cz = iface.thickness + iface.curvature_radius;

		//絞り
		if(iface.curvature_radius == 0)
		{
			float t;
			if(intersect(cz, iface.aperture_radius * (1 - realistic_camera_closure_ratio), o, d, t, axis_x, axis_y))
				return false;

			o += d * t;
		}
		//レンズ
		else
		{
			float t;
			if(!intersect(cz, iface.curvature_radius, iface.aperture_radius, o, d, t))
				return false;

			o += d * t;

			float3 normal = normalize(float3(o.xy, o.z - cz));
			if(dot(normal, d) > 0)
				normal = -normal;

			float iface_ior = calc_ior(iface, lambda);
			throughput *= 1 - fresnel_dielectric(-dot(normal, d), iface_ior / ior);

			d = refract(d, normal, ior / iface_ior);
			ior = iface_ior;
		}
	}
	return true;
}

bool trace(inout float3 o, inout float3 d, float lambda)
{
	float throughput = 1;
	return trace(o, d, throughput, lambda);
}

bool generate_ray(float2 pixel_pos, float u0, float u1, float u2, out float3 origin, out float3 direction, out float3 throughput)
{
	//センサ上の位置を計算
	float2 sensor_pos = pixel_pos * inv_screen_size;
	sensor_pos.x = 1 - sensor_pos.x;
	origin.xy = realistic_camera_sensor_min + realistic_camera_sensor_size * sensor_pos;
	origin.z = realistic_camera_interface_srv[0].thickness + realistic_camera_delta_z;

	//中心からの距離&角度を計算
	float r = length(origin.xy);
	float inv_r = 1 / r;
	float2 axis_x = float2(+origin.x, origin.y) * inv_r;
	float2 axis_y = float2(-origin.y, origin.x) * inv_r;
	
	//ローカル空間に変換
	origin.x = r;
	origin.y = 0;

	//レンズ上の位置をサンプリング
	uint index = r * realistic_camera_inv_delta_r;
	float4 aabb = decode(realistic_camera_pupil_srv.Load4(16 * index), realistic_camera_interface_srv[0].aperture_radius);
	if(any(aabb.xy >= aabb.zw)){ return false; }
	float3 target = float3(lerp(aabb.xy, aabb.zw, float2(u0, u1)), realistic_camera_interface_srv[0].thickness);
	direction = normalize(target - origin);

	//波長をサンプリング
#if 0
	float lambda = uniform_sample_wavelength(u2);
	float pdf_lambda = uniform_sample_wavelength_pdf();
#else
	uint sample_count, stride;
	wavelength_sample_srv.GetDimensions(sample_count, stride);
	uint sample_index = WaveReadLaneFirst(u2 * sample_count) + WaveGetLaneIndex();
	if(sample_index >= sample_count){ sample_index -= sample_count; }
	wavelength_sample s = decompress(wavelength_sample_srv[sample_index]);
	if(s.pdf <= 0){ return false; }
	float lambda = s.lambda;
	float pdf_lambda = s.pdf;
#endif

	//スループットを計算
	//float pdf_x = 1 / ((aabb.z - aabb.x) * (aabb.w - aabb.y));
	//float pdf_w = pdf_x * pow3(realistic_camera_delta_z) / pow2(direction.z);
	//throughput = abs(direction.z) / (pdf_w * pdf_lambda);
	throughput.r = ((aabb.z - aabb.x) * (aabb.w - aabb.y)) * pow3(abs(direction.z)) / (pow3(realistic_camera_delta_z) * pdf_lambda);

	//トレース
	if(!trace(origin, direction, throughput.r, lambda, axis_x, axis_y))
		return false;
	
	float lambda_min = CIE_LAMBDA_MIN - (CIE_LAMBDA_MAX - CIE_LAMBDA_MIN) / (CIE_SAMPLES - 1) / 2;
	float lambda_max = CIE_LAMBDA_MAX + (CIE_LAMBDA_MAX - CIE_LAMBDA_MIN) / (CIE_SAMPLES - 1) / 2;
	float u = (lambda - lambda_min) / (lambda_max - lambda_min);

	//RGBに変換
	throughput = throughput.r * wavelength_to_rgb(lambda);

	//ローカルからワールドに
	origin.xy = origin.x * axis_x + origin.y * axis_y;
	direction.xy = direction.x * axis_x + direction.y * axis_y;
	origin = mul(float4(origin, 1), inv_view_mat);
	direction = mul(direction, (float3x3)inv_view_mat);
	return true;
}

#if defined(CALC_PUPIL) || defined(INIT_PUPIL)

#include"../random.hlsl"
#include"../sampling.hlsl"
#include"../root_constant.hlsl"

RWByteAddressBuffer	realistic_camera_pupil_uav;
groupshared	uint2	shared_min_enc[8];
groupshared	uint2	shared_max_enc[8];

[numthreads(256, 1, 1)]
void init_pupil(uint dtid : SV_DispatchThreadID)
{
	uint size;
	realistic_camera_pupil_uav.GetDimensions(size);

	if(16 * dtid < size)
		realistic_camera_pupil_uav.Store4(16 * dtid, uint4(0xffffffff, 0xffffffff, 0, 0));
}

[numthreads(256, 1, 1)]
void calc_pupil(uint2 dtid : SV_DispatchThreadID, uint group_index : SV_GroupIndex)
{
	uint sample_count = root_constant;
	float ar = realistic_camera_interface_srv[0].aperture_radius;
	float u0 = (dtid.x + 0.5f) / sample_count;
	float u1 = van_der_corput_sequence(dtid.x);
	float u2 = van_der_corput_sequence(dtid.x, 3);
	float u3 = van_der_corput_sequence(dtid.x, 4);
	float lambda = uniform_sample_wavelength(u3);
	float3 target = float3(sample_uniform_disk(ar, u0, u1), realistic_camera_interface_srv[0].thickness);
	float3 o = float3(realistic_camera_delta_r * (dtid.y + u2), 0, realistic_camera_interface_srv[0].thickness + realistic_camera_delta_z);
	float3 d = normalize(target - o);

	bool success = trace(o, d, lambda);
	if(WaveActiveAnyTrue(success))
	{
		if(success)
		{
			uint2 enc = encode(target.xy, ar);
			uint2 min_enc = WaveActiveMin(enc);
			uint2 max_enc = WaveActiveMax(enc);

			if(WaveIsFirstLane())
			{
				shared_min_enc[group_index / 32] = min_enc;
				shared_max_enc[group_index / 32] = max_enc;
			}
		}
	}
	else if(WaveIsFirstLane())
	{
		shared_min_enc[group_index / 32] = 0xffffffff;
		shared_max_enc[group_index / 32] = 0;
	}
	GroupMemoryBarrierWithGroupSync();

	if(group_index < 8)
	{
		uint2 min_enc = WaveActiveMin(shared_min_enc[group_index]);
		uint2 max_enc = WaveActiveMax(shared_max_enc[group_index]);

		if(group_index == 0)
		{
			realistic_camera_pupil_uav.InterlockedMin(16 * dtid.y + 4 * 0, min_enc.x, min_enc.x);
			realistic_camera_pupil_uav.InterlockedMin(16 * dtid.y + 4 * 1, min_enc.y, min_enc.y);
			realistic_camera_pupil_uav.InterlockedMax(16 * dtid.y + 4 * 2, max_enc.x, max_enc.x);
			realistic_camera_pupil_uav.InterlockedMax(16 * dtid.y + 4 * 3, max_enc.y, max_enc.y);
		}
	}
}

#elif defined(CALC_APERTURE)

cbuffer realistic_camera_aperture_cb
{
	float	realistic_camera_angle;
	float	realistic_camera_cos_angle_x05;
	float2	realistic_camera_inv_size;
};

RWTexture2D<float>	realistic_camera_aperture_uav;

[numthreads(16, 16, 1)]
void calc_aperture(uint2 dtid : SV_DispatchThreadID)
{
	float2 pos = dtid * realistic_camera_inv_size * 2 - 1;

	float theta = atan2(pos.y, pos.x);
	if(theta < 0){ theta += 2 * PI; }
	theta = fmod(theta, realistic_camera_angle);

	float len = realistic_camera_cos_angle_x05 / cos(theta - realistic_camera_angle / 2);
	realistic_camera_aperture_uav[dtid] = (dot(pos, pos) > len * len);
}

#endif
#endif

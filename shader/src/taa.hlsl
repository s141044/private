
#ifndef TAA_HLSL
#define TAA_HLSL

#include"math.hlsl"
#include"gbuffer.hlsl"
#include"static_sampler.hlsl"
#include"global_constant.hlsl"

cbuffer constant_buffer
{
	float	update_rate;
	float	sigma_scale;
	float	depth_threshold;
	float	reserved;
};

#define BORDER	1
#define SIZE	16

Texture2D<float>	depth;
Texture2D<float>	prev_depth;
Texture2D<float4>	velocity;
Texture2D<float3>	src;
Texture2D<float3>	history;
RWTexture2D<float3>	dst;
groupshared float3	shared_col[SIZE + 2 * BORDER][SIZE + 2 * BORDER];

[numthreads(SIZE, SIZE, 1)]
void taa(int2 dtid : SV_DispatchThreadID, int2 gtid : SV_GroupThreadID, int2 gid : SV_GroupID, int group_index : SV_GroupIndex)
{
	if(any(dtid >= screen_size))
		return;

	int2 base = SIZE * gid - BORDER;
	for(int i = group_index; i < (SIZE + 2 * BORDER) * (SIZE + 2 * BORDER); i += SIZE * SIZE)
	{
		int2 offset;
		offset.y = i / (SIZE + 2 * BORDER);
		offset.x = i - (SIZE + 2 * BORDER) * offset.y;
		shared_col[offset.y][offset.x] = src[clamp(base + offset, 0, screen_size)];
	}
	GroupMemoryBarrierWithGroupSync();

	float3 current_color = shared_col[gtid.y + BORDER][gtid.x + BORDER];

	float z = depth[dtid];
	if(z <= 0)
	{
		dst[dtid] = current_color;
		return;
	}

	float3 vel = decode_velocity(velocity[dtid]);
	float3 prev_pos = float3(dtid + 0.5f, z) + vel;
	if(any(prev_pos.xy < 0) || any(prev_pos.xy >= screen_size) || (prev_pos.z > 1))
	{
		dst[dtid] = current_color;
		return;
	}

	float3 sum1 = 0;
	float3 sum2 = 0;
	for(int y = 0; y < 2 * BORDER + 1; y++)
	{
		for(int x = 0; x < 2 * BORDER + 1; x++)
		{
			float3 col = shared_col[gtid.y + y][gtid.x + x];
			sum1 += col;
			sum2 += col * col;
		}
	}
	sum1 /= (2 * BORDER + 1) * (2 * BORDER + 1);
	sum2 /= (2 * BORDER + 1) * (2 * BORDER + 1);

	float3 sigma = sqrt(max(sum2 - sum1 * sum1, 0)) * sigma_scale;
	float3 min_col = sum1 - sigma;
	float3 max_col = sum1 + sigma;

	float2 prev_uv = prev_pos.xy * inv_screen_size;
	float3 history_color = history.SampleLevel(bilinear_clamp, prev_uv, 0);
	history_color = clamp(history_color, min_col, max_col);

	//float history_weight = 0.0f;
	//float history_weight = 0.99f;
	float history_weight = 1 - update_rate;

	//prev‚Æ–¾‚é‚³‚Ì·‚ª‘å‚«‚¢‚È‚çhistory‚Ì‰e‹¿‚ð¬‚³‚­
	float lumi_cur = luminance(sum1);
	float lumi_prev = luminance(history_color);
	float lumi_sigma = luminance(sigma);
	float rel_error = (abs(lumi_cur - lumi_prev) - lumi_sigma) / (max(lumi_cur, lumi_prev) + lumi_sigma);
	history_weight *= 1 - saturate(rel_error);

	dst[dtid] = lerp(current_color, history_color, history_weight);

}

#endif
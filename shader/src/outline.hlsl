
#include"static_sampler.hlsl"

cbuffer outline_cb
{
	int2	size;
	float2	inv_size;
	float3	color;
	int		offset;
};

#if defined(DRAW_OUTLINE)

Texture2D<float>	src;
RWTexture2D<float>	dst;
RWByteAddressBuffer	counter0;
RWByteAddressBuffer	counter1;
RWByteAddressBuffer	tiles;

groupshared int shared_draw_flag;

[numthreads(16, 16, 1)]
void draw_outline(int2 dtid : SV_DispatchThreadID, int2 gid : SV_GroupID, int group_index : SV_GroupIndex)
{
	if(any(dtid >= size))
		return;

	if(group_index == 0)
		shared_draw_flag = 0;
	GroupMemoryBarrierWithGroupSync();

	bool draw = false;
	if(src[dtid] != 0)
	{
		if(!draw && (dtid.x - offset >= 0)){ draw = (src[int2(dtid.x - offset, dtid.y)] == 0); }
		if(!draw && (dtid.y - offset >= 0)){ draw = (src[int2(dtid.x, dtid.y - offset)] == 0); }
		if(!draw && (dtid.x + offset < size.x)){ draw = (src[int2(dtid.x + offset, dtid.y)] == 0); }
		if(!draw && (dtid.y + offset < size.y)){ draw = (src[int2(dtid.x, dtid.y + offset)] == 0); }
	}
	dst[dtid] = draw;

	int add = WaveActiveAnyTrue(draw);
	if(WaveIsFirstLane())
		InterlockedAdd(shared_draw_flag, add);
	GroupMemoryBarrierWithGroupSync();

	if(shared_draw_flag)
	{
		if(group_index == 0)
		{
			uint index;
			counter0.InterlockedAdd(8, 1, index);
			tiles.Store(4 * index, gid.x | (gid.y << 16));
		}
	}

	if(dtid.x + dtid.y == 0)
		counter1.Store(8, 0);
}

#elif defined(ANTI_ALIASING)

Texture2D<float>	src;
RWTexture2D<float3>	dst;
ByteAddressBuffer	tiles;

[numthreads(16, 16, 1)]
void anti_aliasing(int3 dtid : SV_DispatchThreadID)
{
	int2 gid;
	int tile = tiles.Load(4 * dtid.z);
	gid.x = tile & 0xffff;
	gid.y = tile >> 16;
	dtid.xy += 16 * gid;

	float2 uv = (dtid.xy + 0.5f) * inv_size;
	float m = src.SampleLevel(bilinear_clamp, uv, 0);
	if(m > 0)
	{
		dst[dtid.xy] = color;
		return;
	}

	float nw = src.SampleLevel(bilinear_clamp, float2(dtid.x + 0.0f, dtid.y + 0.0f) * inv_size, 0);
	float ne = src.SampleLevel(bilinear_clamp, float2(dtid.x + 1.0f, dtid.y + 0.0f) * inv_size, 0);
	float sw = src.SampleLevel(bilinear_clamp, float2(dtid.x + 0.0f, dtid.y + 1.0f) * inv_size, 0);
	float se = src.SampleLevel(bilinear_clamp, float2(dtid.x + 1.0f, dtid.y + 1.0f) * inv_size, 0);
	if(nw + ne + sw + se == 0)
		return;
	float min_val = min(m, min(nw, min(ne, min(sw, se))));
	float max_val = max(m, max(nw, max(ne, max(sw, se))));

	float2 dir;
	dir.x = -((nw + ne) - (sw + se));
	dir.y = +((ne + sw) - (ne + se));

	float max_span = 8;
	float reduce_min = 1.0 / 128.0;
	float reduce_mul = 1.0 / 8.0;
	float bias = max((nw + ne + sw + se) * (0.25f * reduce_mul), reduce_min);
	float scale = 1 / (min(abs(dir.x), abs(dir.y)) + bias);
	dir = clamp(dir * scale, -max_span, max_span) * inv_size;
	
	float a = 0.5f * (
		src.SampleLevel(bilinear_clamp, uv + dir * (1.0f / 3.0f - 0.5f), 0) + 
		src.SampleLevel(bilinear_clamp, uv + dir * (2.0f / 3.0f - 0.5f), 0));
	float b = 0.5f * a + 0.25f * (
		src.SampleLevel(bilinear_clamp, uv + dir * (0.0f / 3.0f - 0.5f), 0) + 
		src.SampleLevel(bilinear_clamp, uv + dir * (3.0f / 3.0f - 0.5f), 0));
	
	float t;
	if((b > min_val) && (b < max_val))
		t = b;
	else
		t = a;

	if(t > 0)
		dst[dtid.xy] = lerp(dst[dtid.xy], color, t);
}

#endif

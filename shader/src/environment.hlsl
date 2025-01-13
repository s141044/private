
#ifndef ENVIRONMENT_HLSL
#define ENVIRONMENT_HLSL

#include"math.hlsl"

///////////////////////////////////////////////////////////////////////////////////////////////////

#define CUBEMAP_POSITIVE_X 0
#define CUBEMAP_NEGATIVE_X 1
#define CUBEMAP_POSITIVE_Y 2
#define CUBEMAP_NEGATIVE_Y 3
#define CUBEMAP_POSITIVE_Z 4
#define CUBEMAP_NEGATIVE_Z 5

///////////////////////////////////////////////////////////////////////////////////////////////////

//方向(未正規化)を返す
float3 cubemap_direction(float u, float v, int face)
{
	switch(face){
	case CUBEMAP_POSITIVE_X: return float3(+1, +v, -u);
	case CUBEMAP_NEGATIVE_X: return float3(-1, +v, +u);
	case CUBEMAP_POSITIVE_Y: return float3(+u, +1, -v);
	case CUBEMAP_NEGATIVE_Y: return float3(+u, -1, +v);
	case CUBEMAP_POSITIVE_Z: return float3(+u, +v, +1);
	//case CUBEMAP_NEGATIVE_Z: return float3(+u, +v, +1);
	default: return float3(-u, +v, -1);
	}
}

float3 cubemap_direction(float x, float y, int face, float inv_res)
{
	float u = +(x * inv_res * 2 - 1);
	float v = -(y * inv_res * 2 - 1);
	return cubemap_direction(u, v, face);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//uv/面を返す
float3 cubemap_uv_face(float3 d)
{
	//uvは単位立方体に投影したときの各面のローカル空間の座標値(-1,1)
	//テクスチャ座標とする場合は(u*0.5+0.5,-v*0.5+0.5)

	float3 abs_d = abs(d);
	if((abs_d.x >= abs_d.y) && (abs_d.x >= abs_d.z))
	{
		float inv_abs_max = 1 / abs_d.x;
		return (d.x >= 0) ? 
			float3(-d.z * inv_abs_max, +d.y * inv_abs_max, CUBEMAP_POSITIVE_X) : 
			float3(+d.z * inv_abs_max, +d.y * inv_abs_max, CUBEMAP_NEGATIVE_X);
	}
	else if(abs_d.y >= abs_d.z)
	{
		float inv_abs_max = 1 / abs_d.y;
		return (d.y >= 0) ? 
			float3(+d.x * inv_abs_max, -d.z * inv_abs_max, CUBEMAP_POSITIVE_Y) : 
			float3(+d.x * inv_abs_max, +d.z * inv_abs_max, CUBEMAP_NEGATIVE_Y);
	}
	else
	{
		float inv_abs_max = 1 / abs_d.z;
		return (d.z >= 0) ? 
			float3(+d.x * inv_abs_max, +d.y * inv_abs_max, CUBEMAP_POSITIVE_Z) : 
			float3(-d.x * inv_abs_max, +d.y * inv_abs_max, CUBEMAP_NEGATIVE_Z);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//パノラマ画像のサンプリングのuvを返す
float2 panorama_uv(float3 dir)
{
	float t = acos(dir.y);
	float p = atan2(dir.z, dir.x);
	return float2((p * inv_PI + 1) / 2, t * inv_PI);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(INITIALIZE_LOD0)

#include"packing.hlsl"
#include"root_constant.hlsl"

Texture2D<float3>		src;
RWTexture2DArray<uint>	dst;
SamplerState			panorama_sampler;

[numthreads(16, 16, 1)]
void initialize_lod0(int3 dtid : SV_DispatchThreadID)
{
	float inv_res = asfloat(root_constant);
	float3 d = normalize(cubemap_direction(dtid.x + 0.5f, dtid.y + 0.5f, dtid.z, inv_res));
	float3 L = src.SampleLevel(panorama_sampler, panorama_uv(d), 0);
	dst[dtid] = f32x3_to_r9g9b9e5(L);
}

#endif
#endif
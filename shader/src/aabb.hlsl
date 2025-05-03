
#ifndef AABB_HLSL
#define AABB_HLSL

#include"math.hlsl"

///////////////////////////////////////////////////////////////////////////////////////////////////

struct aabb
{
	float3 min, max;
};

aabb empty_aabb()
{
	aabb ret;
	ret.min = +FLT_MAX;
	ret.max = -FLT_MAX;
	return ret;
}

float area(aabb aabb)
{
	float3 diag = aabb.max - aabb.min;
	return (diag.x * diag.y + diag.x * diag.z + diag.y * diag.z) * 2;
}

aabb merge(aabb a, aabb b)
{
	aabb ret;
	ret.min = min(a.min, b.min);
	ret.max = max(a.max, b.max);
	return ret;
}

aabb merge(aabb a, float3 b)
{
	aabb ret;
	ret.min = min(a.min, b);
	ret.max = max(a.max, b);
	return ret;
}

aabb merge(float3 a, aabb b)
{
	aabb ret;
	ret.min = min(a, b.min);
	ret.max = max(a, b.max);
	return ret;
}

float square_distance(aabb aabb, float3 p)
{
	float dist2 = 0;
	if(p.x < aabb.min.x){ dist2 += pow2(aabb.min.x - p.x); }else if(p.x > aabb.max.x){ dist2 += pow2(p.x - aabb.max.x); }
	if(p.y < aabb.min.y){ dist2 += pow2(aabb.min.y - p.y); }else if(p.y > aabb.max.y){ dist2 += pow2(p.y - aabb.max.y); }
	if(p.z < aabb.min.z){ dist2 += pow2(aabb.min.z - p.z); }else if(p.z > aabb.max.z){ dist2 += pow2(p.z - aabb.max.z); }
	return dist2;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

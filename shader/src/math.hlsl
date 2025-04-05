
#ifndef MATH_HLSL
#define MATH_HLSL

///////////////////////////////////////////////////////////////////////////////////////////////////

static const float PI		= 3.1415926535897932385f;
static const float inv_PI	= 1 / (1 * PI);
static const float inv_2PI	= 1 / (2 * PI);

///////////////////////////////////////////////////////////////////////////////////////////////////

float sum(float2 v){ return v.x + v.y; }
float sum(float3 v){ return v.x + v.y + v.z; }
float sum(float4 v){ return v.x + v.y + v.z + v.w; }

///////////////////////////////////////////////////////////////////////////////////////////////////

float average(float2 v){ return sum(v) / 2; }
float average(float3 v){ return sum(v) / 3; }
float average(float4 v){ return sum(v) / 4; }

///////////////////////////////////////////////////////////////////////////////////////////////////

float pow2(float x){ return x * x; }
float2 pow2(float2 x){ return x * x; }
float3 pow2(float3 x){ return x * x; }
float4 pow2(float4 x){ return x * x; }

float pow3(float x){ return x * x * x; }
float2 pow3(float2 x){ return x * x * x; }
float3 pow3(float3 x){ return x * x * x; }
float4 pow3(float4 x){ return x * x * x; }

float pow4(float x){ return pow2(pow2(x)); }
float2 pow4(float2 x){ return pow2(pow2(x)); }
float3 pow4(float3 x){ return pow2(pow2(x)); }
float4 pow4(float4 x){ return pow2(pow2(x)); }

float pow5(float x){ return pow4(x) * x; }
float2 pow5(float2 x){ return pow4(x) * x; }
float3 pow5(float3 x){ return pow4(x) * x; }
float4 pow5(float4 x){ return pow4(x) * x; }

///////////////////////////////////////////////////////////////////////////////////////////////////

float atanh(float x)
{
	return log((1 + x) / (1 - x)) / 2;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

float luminance(float3 rgb)
{
	return dot(rgb, float3(0.2126f, 0.7152f, 0.0722f));
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void calc_orthonormal_basis(float3 normal, out float3 tangent, out float3 binormal)
{
	float s = sign(normal.z);
	float a = -1 / (s + normal.z);
	float b = normal.x * normal.y * a;
	tangent = float3(1 + s * normal.x * normal.x * a, s * b, -s * normal.x);
	binormal = float3(b, s + normal.y * normal.y * a, -normal.y);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

float3 normal_correction(float3 n, float3 wo, float threshold = 1e-3f)
{
	float cos = dot(n, wo);
	if(cos >= threshold)
		return n;
	
	float a = -cos + threshold * sqrt((1 - pow2(cos)) / (1 - pow2(threshold)));
	return normalize(n + a * wo);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

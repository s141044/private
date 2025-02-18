
#include"utility.hlsl"
#include"static_sampler.hlsl"
#include"fullscreen_triangle.hlsl"

#if defined(R9G9B9E5)
Texture2D<uint>		hdr_srv;
#else
Texture2D<float3>	hdr_srv;
#endif
Texture2D<float4>	ldr_srv;

float3 apply_pq_curve(float3 x)
{
	float m1 = 2610.0 / 4096.0 / 4;
	float m2 = 2523.0 / 4096.0 * 128;
	float c1 = 3424.0 / 4096.0;
	float c2 = 2413.0 / 4096.0 * 32;
	float c3 = 2392.0 / 4096.0 * 32;
	
	x = pow(x, m1);
	return pow((c1 + c2 * x) / (1 + c3 * x), m2);
}

float3 present_ps(fullscreen_triangle_vs_output input): SV_TARGET
{
	float4 col = ldr_srv.SampleLevel(bilinear_clamp, input.uv, 0);
#if defined(R9G9B9E5)
	col.rgb += bilinear_sample(hdr_srv, bilinear_clamp, input.uv) * (1 - col.a);
#else
	col.rgb += hdr_srv.SampleLevel(bilinear_clamp, input.uv, 0) * (1 - col.a);
#endif
	col *= 300.0 / 10000.0; //1を300nitにマッピング
	//col = rec709_to_rec2020(col);
	return apply_pq_curve(col.rgb);
}

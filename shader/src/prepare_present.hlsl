
#include"static_sampler.hlsl"
#include"fullscreen_triangle.hlsl"

Texture2D<float3> src;

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

float3 rec709_to_rec2020(float3 rec709)
{
	static const float3x3 m =
	{
		0.627402, 0.329292, 0.043306,
		0.069095, 0.919544, 0.011360,
		0.016394, 0.088028, 0.895578
	};
	return mul(m, rec709);
}

float3 ps_main(fullscreen_triangle_vs_output input): SV_TARGET
{
	float3 col = src.SampleLevel(bilinear_clamp, input.uv, 0);
	col *= 300.0 / 10000.0; //1を300nitにマッピング
	col = rec709_to_rec2020(col); //必要だけど色がくすむ
	return apply_pq_curve(col);
}

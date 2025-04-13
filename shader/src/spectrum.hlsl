
#ifndef SPECTRUM_HLSL
#define SPECTRUM_HLSL

#include"math.hlsl"
#include"packing.hlsl"
#include"static_sampler.hlsl"

#define CIE_LAMBDA_MIN 360.0
#define CIE_LAMBDA_MAX 830.0
#define CIE_SAMPLES 95

Texture1D<float3>	cie_xyz_srv;

float3 wavelength_to_xyz(float lambda)
{
	float lambda_min = CIE_LAMBDA_MIN - (CIE_LAMBDA_MAX - CIE_LAMBDA_MIN) / (CIE_SAMPLES - 1) / 2;
	float lambda_max = CIE_LAMBDA_MAX + (CIE_LAMBDA_MAX - CIE_LAMBDA_MIN) / (CIE_SAMPLES - 1) / 2;
	float u = (lambda - lambda_min) / (lambda_max - lambda_min);
	return cie_xyz_srv.SampleLevel(bilinear_clamp, u, 0);
}

float3 wavelength_to_rgb(float lambda)
{
	float3x3 xyz_to_rgb = float3x3(
		 1.7166512, -0.3556708, -0.2533663,
		-0.6666844,  1.6164812,  0.0157685,
		 0.0176399, -0.0427706,  0.9421031);
	float3 xyz = wavelength_to_xyz(lambda);
	return mul(xyz_to_rgb, xyz);
}

#define SAMPLE_LAMBDA_MIN 380.0
#define SAMPLE_LAMBDA_MAX 750.0

float uniform_sample_wavelength(float u)
{
	return SAMPLE_LAMBDA_MIN + (SAMPLE_LAMBDA_MAX - SAMPLE_LAMBDA_MIN) * u;
}

float uniform_sample_wavelength_pdf()
{
	return 1 / float(SAMPLE_LAMBDA_MAX - SAMPLE_LAMBDA_MIN);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

struct compressed_wavelength_sample
{
	uint v;
};

struct wavelength_sample
{
	float lambda;
	float pdf;
};

StructuredBuffer<compressed_wavelength_sample> wavelength_sample_srv;

compressed_wavelength_sample compress(wavelength_sample s)
{
	compressed_wavelength_sample cs;
	cs.v = f32_to_u16_unorm((s.lambda - SAMPLE_LAMBDA_MIN) / (SAMPLE_LAMBDA_MAX - SAMPLE_LAMBDA_MIN)) | (f32_to_f16(s.pdf) << 16);
	return cs;
}

wavelength_sample decompress(compressed_wavelength_sample cs)
{
	wavelength_sample s;
	s.lambda = u16_unorm_to_f32(cs.v) * (SAMPLE_LAMBDA_MAX - SAMPLE_LAMBDA_MIN) + SAMPLE_LAMBDA_MIN;
	s.pdf = f16_to_f32(cs.v >> 16);
	return s;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//An Improved Technique for Full Spectral Rendering
//‹É‚ß‚Ä”÷–­

static const float wavelength_pdf_A = 0.072;
static const float wavelength_pdf_B = 538;
static const float wavelength_pdf_K0 = tanh(wavelength_pdf_A * (SAMPLE_LAMBDA_MIN - wavelength_pdf_B));
static const float wavelength_pdf_K1 = tanh(wavelength_pdf_A * (SAMPLE_LAMBDA_MAX - wavelength_pdf_B));
static const float wavelength_pdf_N = (wavelength_pdf_K1 - wavelength_pdf_K0) / wavelength_pdf_A;

float sample_wavelength(float u)
{
	const float x = atanh((wavelength_pdf_K1 - wavelength_pdf_K0) * u + wavelength_pdf_K0);
	return x * (1 / wavelength_pdf_A) + wavelength_pdf_B;
}

float sample_wavelength_pdf(float lambda)
{
	const float x = wavelength_pdf_A * (lambda - wavelength_pdf_B);
	return 1 / (pow2(cosh(x)) * wavelength_pdf_N);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif


#ifndef WAVELENGTH_HLSL
#define WAVELENGTH_HLSL

#include"math.hlsl"
#include"random.hlsl"
#include"sampling.hlsl"
#include"spectrum.hlsl"

float integral(float scale, float mean, float stddev, bool log_normal)
{
	float ret = scale * stddev * sqrt(2 * PI);
	if(log_normal){ ret *= exp(mean + stddev * stddev / 2); }
	return ret;
}

static const float X0_mean		= 595.8;
static const float X0_stddev	= 33.33;
static const float X0_scale		= 1.065;
static const float X1_mean		= 446.8;
static const float X1_stddev	= 19.44;
static const float X1_scale		= 0.366;
static const float Y_mean		= log(556.3);
static const float Y_stddev		= 0.075;
static const float Y_scale		= 1.014;
static const float Z_mean		= log(449.8);
static const float Z_stddev		= 0.051;
static const float Z_scale		= 1.839;

static const float X0_integral	= integral(X0_scale, X0_mean, X0_stddev, false);
static const float X1_integral	= integral(X1_scale, X1_mean, X1_stddev, false);
static const float Y_integral	= integral(Y_scale, Y_mean, Y_stddev, true);
static const float Z_integral	= integral(Z_scale, Z_mean, Z_stddev, true);

cbuffer presample_wavelength_cbuffer
{
	uint	sample_count;
	float2	jitter;
	float	padding;
};

RWStructuredBuffer<compressed_wavelength_sample> wavelength_sample_uav;

[numthreads(256, 1, 1)]
void presample_wavelength(uint dtid : SV_DispatchThreadID)
{
	if(dtid * 2 >= sample_count)
		return;

	float2 u = hammersley_sequence(shuffle(dtid, sample_count / 2), sample_count / 2);
	u = frac(u + jitter);

	float integral = X0_integral + X1_integral + Y_integral + Z_integral;
	u[0] *= integral;

	bool log_normal;
	float mean, stddev, scale;
	if(u[0] < X0_integral)
	{
		mean = X0_mean;
		scale = X0_scale;
		stddev = X0_stddev;
		log_normal = false;
		u[0] /= X0_integral;
	}
	else if((u[0] -= X0_integral) < X1_integral)
	{
		mean = X1_mean;
		scale = X1_scale;
		stddev = X1_stddev;
		log_normal = false;
		u[0] /= X1_integral;
	}
	else if((u[0] -= X1_integral) < Y_integral)
	{
		mean = Y_mean;
		scale = Y_scale;
		stddev = Y_stddev;
		log_normal = true;
		u[0] /= Y_integral;
	}
	else
	{
		mean = Z_mean;
		scale = Z_scale;
		stddev = Z_stddev;
		log_normal = true;
		u[0] -= Y_integral;
		u[0] /= Z_integral;
	}

	float2 lambda = sample_normal(mean, stddev, u[0], u[1]);
	float2 log_lambda = lambda;
	if(log_normal)
		lambda = exp(lambda);
	else
		log_lambda = log(lambda);

	float2 pdf = 0;
	pdf[0] += sample_normal_pdf(lambda[0], X0_mean, X0_stddev) * X0_stddev;
	pdf[1] += sample_normal_pdf(lambda[1], X0_mean, X0_stddev) * X0_stddev;
	pdf[0] += sample_normal_pdf(lambda[0], X1_mean, X1_stddev) * X1_stddev;
	pdf[1] += sample_normal_pdf(lambda[1], X1_mean, X1_stddev) * X1_stddev;
	pdf[0] += sample_normal_pdf(log_lambda[0], Y_mean, Y_stddev) * Y_stddev;
	pdf[1] += sample_normal_pdf(log_lambda[1], Y_mean, Y_stddev) * Y_stddev;
	pdf[0] += sample_normal_pdf(log_lambda[0], Z_mean, Z_stddev) * Z_stddev;
	pdf[1] += sample_normal_pdf(log_lambda[1], Z_mean, Z_stddev) * Z_stddev;
	pdf *= sqrt(2 * PI) / integral;

	wavelength_sample s[2];
	s[0].lambda = lambda[0];
	s[1].lambda = lambda[1];
	s[0].pdf = pdf[0];
	s[1].pdf = pdf[1];

	if((lambda[0] < SAMPLE_LAMBDA_MIN) || (lambda[0] >= SAMPLE_LAMBDA_MAX)){ s[0].pdf = 0; }
	if((lambda[1] < SAMPLE_LAMBDA_MIN) || (lambda[1] >= SAMPLE_LAMBDA_MAX)){ s[1].pdf = 0; }

	wavelength_sample_uav[dtid + sample_count / 2 * 0] = compress(s[0]);
	wavelength_sample_uav[dtid + sample_count / 2 * 1] = compress(s[1]);
}

#endif

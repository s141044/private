
#ifndef RANDOM_HLSL
#define RANDOM_HLSL

///////////////////////////////////////////////////////////////////////////////////////////////////

struct rng
{
	uint state;
};

rng create_rng(uint seed)
{
	rng r;
	r.state = seed;
	return r;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

uint rand(inout rng r)
{
	uint z = (r.state += 0x6D2B79F5);
	z = (z ^ (z >> 15)) * (z | 1);
	z ^= z + (z ^ (z >> 7)) * (z | 61);
	return z ^ (z >> 14);
}

float randF(inout rng r)
{
	return asfloat(0x3f800000 | (rand(r) & 0x007fffff)) - 1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

float van_der_corput_sequence(uint n)
{
	float s = 1.0f / (1ull << 32);
	return reversebits(n) * s;
}

float2 hammersley_sequence(uint n, uint N)
{
	return float2(n / float(N), van_der_corput_sequence(n));
}

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

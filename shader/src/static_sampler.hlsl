
#ifndef STATIC_SAMPLER_HLSL
#define STATIC_SAMPLER_HLSL

///////////////////////////////////////////////////////////////////////////////////////////////////

SamplerState point_clamp		: register(s0, space1);
SamplerState point_wrap			: register(s1, space1);
SamplerState point_mirror		: register(s2, space1);
SamplerState point_border		: register(s3, space1);
SamplerState bilinear_clamp		: register(s4, space1);
SamplerState bilinear_wrap		: register(s5, space1);
SamplerState bilinear_mirror	: register(s6, space1);
SamplerState bilinear_border	: register(s7, space1);
SamplerState trilinear_clamp	: register(s8, space1);
SamplerState trilinear_wrap		: register(s9, space1);
SamplerState trilinear_mirror	: register(s10, space1);
SamplerState trilinear_border	: register(s11, space1);

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

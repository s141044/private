
#ifndef DEBUG_HLSL
#define DEBUG_HLSL

#define ENABLE_DEBUG_OUTPUT 1

RWTexture2D<float4>	debug_output0;
RWTexture2D<float4>	debug_output1;
RWTexture2D<float4>	debug_output2;
RWTexture2D<float4>	debug_output3;

void init_debug_output(uint2 dtid)
{
#if defined(ENABLE_DEBUG_OUTPUT)
	debug_output0[dtid] = 0;
	debug_output1[dtid] = 0;
	debug_output2[dtid] = 0;
	debug_output3[dtid] = 0;
#endif
}

void debug_output(uint i, uint2 dtid, float4 val)
{
#if defined(ENABLE_DEBUG_OUTPUT)
	if(i == 0)
		debug_output0[dtid] = val;
	else if(i == 1)
		debug_output1[dtid] = val;
	else if(i == 2)
		debug_output2[dtid] = val;
	else if(i == 3)
		debug_output3[dtid] = val;
#endif
}

#endif

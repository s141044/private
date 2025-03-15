
#ifndef EMISSIVE_UTILITY_HLSL
#define EMISSIVE_UTILITY_HLSL

struct emissive_blas_header
{
	float	area;
	float	base_power;
	uint	primitive_count;
	uint	raytracing_instance_index;
	uint	bindless_instance_index;
};

struct emissive_tlas_header
{
	float	total_power;
	float	inv_total_power;
	uint	blas_count;
	uint	reserved;
};

#define EMISSIVE_BLAS_HEADER_SIZE 16
#define EMISSIVE_TLAS_HEADER_SIZE 16
			
emissive_blas_header load_emissive_blas_header(ByteAddressBuffer blas)
{
	uint4 vals = blas.Load4(0);
	emissive_blas_header header;
	header.area = asfloat(vals.x);
	header.base_power = asfloat(vals.y);
	header.primitive_count = vals.z & 0xffff;
	header.raytracing_instance_index = vals.z >> 16;
	header.bindless_instance_index = vals.w & 0xffff;
	return header;
}

emissive_tlas_header load_emissive_tlas_header(ByteAddressBuffer tlas)
{
	return tlas.Load<emissive_tlas_header>(0);
}

#endif
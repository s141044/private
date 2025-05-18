
#ifndef MATERIAL_COMMON_HLSL
#define MATERIAL_COMMON_HLSL

///////////////////////////////////////////////////////////////////////////////////////////////////

#define MATERIAL_HEADER_SIZE 12

///////////////////////////////////////////////////////////////////////////////////////////////////

#define MATERIAL_TYPE_STANDARD	0
#define MATERIAL_TYPE_HAIR		1

///////////////////////////////////////////////////////////////////////////////////////////////////

#if !defined(RT_SAMPLER)
#define RT_SAMPLER bilinear_wrap
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////

struct material_header
{
	uint	material_type;
	uint	alpha_map_handle;
	uint	displacement_map_handle;
	float	max_displacement;
};

material_header load_material_header(uint material_handle)
{
	uint3 data = get_byteaddress_buffer(material_handle).Load3(0);

	material_header header;
	header.material_type = data.x & 0xff;
	header.alpha_map_handle = data.x >> 8;
	header.displacement_map_handle = data.y;
	header.max_displacement = asfloat(data.z);
	return header;
}

bool has_alpha_map(material_header header)
{
	return (header.alpha_map_handle != 0x00ffffff);
}

bool has_displacement_map(material_header header)
{
	return (header.displacement_map_handle != 0xffffffff);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

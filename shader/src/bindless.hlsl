
#ifndef BINDLESS_HLSL
#define BINDLESS_HLSL

///////////////////////////////////////////////////////////////////////////////////////////////////

static const uint invalid_bindless_handle = 0xffffffff;

///////////////////////////////////////////////////////////////////////////////////////////////////

ByteAddressBuffer get_byteaddress_buffer(uint handle)
{
	return ResourceDescriptorHeap[NonUniformResourceIndex(handle)];
}

///////////////////////////////////////////////////////////////////////////////////////////////////

template<class T> StructuredBuffer<T> get_structured_buffer(uint handle)
{
	return ResourceDescriptorHeap[NonUniformResourceIndex(handle)];
}

///////////////////////////////////////////////////////////////////////////////////////////////////

template<class T> Texture1D<T> get_texture1d(uint handle)
{
	return ResourceDescriptorHeap[NonUniformResourceIndex(handle)];
}

template<class T> Texture2D<T> get_texture2d(uint handle)
{
	return ResourceDescriptorHeap[NonUniformResourceIndex(handle)];
}

template<class T> Texture3D<T> get_texture3d(uint handle)
{
	return ResourceDescriptorHeap[NonUniformResourceIndex(handle)];
}

template<class T> TextureCube<T> get_texture_cube(uint handle)
{
	return ResourceDescriptorHeap[NonUniformResourceIndex(handle)];
}

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

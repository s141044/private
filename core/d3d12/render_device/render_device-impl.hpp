
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace d3d12{

///////////////////////////////////////////////////////////////////////////////////////////////////
//関数定義
///////////////////////////////////////////////////////////////////////////////////////////////////

//成功確認
inline void check_hresult(const HRESULT hresult)
{
#ifdef _DEBUG
	if(FAILED(hresult)){ __debugbreak(); }
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//リソースフラグの変換
inline D3D12_RESOURCE_FLAGS convert(const resource_flags flags)
{
	D3D12_RESOURCE_FLAGS ret = D3D12_RESOURCE_FLAG_NONE;
	if(flags & resource_flag_allow_depth_stencil)
	{
		ret |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		if((flags & resource_flag_allow_shader_resource) == 0)
			ret |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
	}
	if(flags & resource_flag_allow_render_target)
		ret |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	if(flags & resource_flag_allow_unordered_access)
		ret |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	if(flags & resource_flag_raytracing_acceleration_structure)
		ret |= D3D12_RESOURCE_FLAG_RAYTRACING_ACCELERATION_STRUCTURE;
	return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//DSVフラグの変換
inline D3D12_DSV_FLAGS convert(const dsv_flags flags)
{
	D3D12_DSV_FLAGS ret = D3D12_DSV_FLAG_NONE;
	if(flags & dsv_flag_readonly_depth)
		ret |= D3D12_DSV_FLAG_READ_ONLY_DEPTH;
	if(flags & dsv_flag_readonly_stencil)
		ret |= D3D12_DSV_FLAG_READ_ONLY_STENCIL;
	return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//DepthTextureのSRVフォーマット
inline DXGI_FORMAT convert(const DXGI_FORMAT format, const srv_flag flag)
{
	if(flag & srv_flag_depth)
	{
		switch(format)
		{
		case DXGI_FORMAT_R16_TYPELESS:
			return DXGI_FORMAT_R16_UNORM;
		case DXGI_FORMAT_R32_TYPELESS:
			return DXGI_FORMAT_R32_FLOAT;
		case DXGI_FORMAT_R24G8_TYPELESS:
			return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		case DXGI_FORMAT_R32G8X24_TYPELESS:
			return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
		}
	}
	else if(flag & srv_flag_stencil)
	{
		switch(format)
		{
		case DXGI_FORMAT_R24G8_TYPELESS:
			return DXGI_FORMAT_X24_TYPELESS_G8_UINT;
		case DXGI_FORMAT_R32G8X24_TYPELESS:
			return DXGI_FORMAT_X32_TYPELESS_G8X24_UINT;
		}
	}
	return format;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//DepthTextureのDSVフォーマット
inline DXGI_FORMAT convert(const DXGI_FORMAT format)
{
	switch(format)
	{
	case DXGI_FORMAT_R16_TYPELESS:
		return DXGI_FORMAT_D16_UNORM;
	case DXGI_FORMAT_R32_TYPELESS:
		return DXGI_FORMAT_D32_FLOAT;
	case DXGI_FORMAT_R24G8_TYPELESS:
		return DXGI_FORMAT_D24_UNORM_S8_UINT;
	case DXGI_FORMAT_R32G8X24_TYPELESS:
		return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
	default:
		return format;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//Plane数を返す
inline uint plane_count(const DXGI_FORMAT format)
{
	switch(format)
	{
	case DXGI_FORMAT_R24G8_TYPELESS:
	case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
		return 2;
	default:
		return 1;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//StaticSamplerを返す
static auto& get_static_samplers(const uint bind_point, const uint bind_space)
{
	static auto samplers = []()
	{
		const D3D12_FILTER filters[3] = 
		{
			D3D12_FILTER_MIN_MAG_MIP_POINT,			//Point
			D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT,	//Bilinear
			D3D12_FILTER_MIN_MAG_MIP_LINEAR,		//Trilinear
		};
		const D3D12_TEXTURE_ADDRESS_MODE modes[4] = 
		{
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			D3D12_TEXTURE_ADDRESS_MODE_MIRROR,
			D3D12_TEXTURE_ADDRESS_MODE_BORDER,
		};

		std::array<D3D12_STATIC_SAMPLER_DESC, _countof(filters) * _countof(modes)> descs = {};
		for(uint i = 0, idx = 0; i < _countof(filters); i++)
		{
			for(uint j = 0; j < _countof(modes); j++, idx++)
			{
				auto& desc = descs[idx];
				desc.MaxLOD = FLT_MAX;
				desc.MaxAnisotropy = 16;
				desc.RegisterSpace = 1;
				desc.ShaderRegister = idx;
				desc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
				desc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;

				desc.Filter = filters[i];
				desc.AddressU = modes[j];
				desc.AddressV = modes[j];
				desc.AddressW = modes[j];
			}
		}
		return descs;
	}();
	return samplers;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//サブリソースのインデックスを返す
inline uint subresource_index(resource& res, const uint mip_slice, const uint array_slice, const uint plane_slice)
{
	const auto& desc = res->GetDesc();
	const uint mip_levels = desc.MipLevels;
	const uint array_size = (desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D) ? 1 : desc.DepthOrArraySize;
	return mip_slice + mip_levels * (array_slice + array_size * plane_slice);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//文字列化
inline string state_string(uint state)
{
	string output;
	if(state == D3D12_RESOURCE_STATE_COMMON){ return "COMMON | "; }
	if(state & D3D12_RESOURCE_STATE_COPY_DEST){ output += "COPY_DEST | "; }
	if(state & D3D12_RESOURCE_STATE_COPY_SOURCE){ output += "COPY_SOURCE | "; }
	if(state & D3D12_RESOURCE_STATE_DEPTH_READ){ output += "DEPTH_READ | "; }
	if(state & D3D12_RESOURCE_STATE_DEPTH_WRITE){ output += "DEPTH_WRITE | "; }
	if(state & D3D12_RESOURCE_STATE_INDEX_BUFFER){ output += "INDEX_BUFFER | "; }
	if(state & D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT){ output += "INDIRECT_ARGUMENT | "; }
	if(state & D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE){ output += "NON_PIXEL_SHADER_RESOURCE | "; }
	if(state & D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE){ output += "PIXEL_SHADER_RESOURCE | "; }
	if(state & D3D12_RESOURCE_STATE_RENDER_TARGET){ output += "RENDER_TARGET | "; }
	if(state & D3D12_RESOURCE_STATE_UNORDERED_ACCESS){ output += "UNORDERED_ACCESS | "; }
	if(state & D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER){ output += "VERTEX_AND_CONSTANT_BUFFER | "; }
	if(output.empty()){ output = "OTHER"; }
	return output;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//定数
///////////////////////////////////////////////////////////////////////////////////////////////////

inline const auto raytracing_compaction_state_progress1 = raytracing_compaction_state(raytracing_compaction_state_progress0 + 1);
inline const auto raytracing_compaction_state_progress2 = raytracing_compaction_state(raytracing_compaction_state_progress1 + 1);

///////////////////////////////////////////////////////////////////////////////////////////////////
//resource
///////////////////////////////////////////////////////////////////////////////////////////////////

//初期化
inline void resource::init(ID3D12Device& device, const D3D12_HEAP_PROPERTIES& prop, const D3D12_HEAP_FLAGS flags, const D3D12_RESOURCE_DESC& desc, const D3D12_RESOURCE_STATES state, const D3D12_CLEAR_VALUE* clear_value)
{
	D3D12_CLEAR_VALUE tmp_clear_value = {};
	if((clear_value == nullptr) && (desc.Format != DXGI_FORMAT_UNKNOWN) && (desc.Flags & (D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)))
	{
		tmp_clear_value.Format = convert(desc.Format);
		clear_value = &tmp_clear_value;
	}

	ComPtr<ID3D12Resource> p_impl;
	check_hresult(device.CreateCommittedResource(&prop, flags, &desc, state, clear_value, IID_PPV_ARGS(&p_impl)));
	init(std::move(p_impl), desc, state);
}

inline void resource::init(ComPtr<ID3D12Resource> p_impl, const D3D12_RESOURCE_DESC& desc, const D3D12_RESOURCE_STATES state)
{
	mp_impl = std::move(p_impl);

	uint array_size = 1;
	if(desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE3D)
		array_size = desc.DepthOrArraySize;

	m_state_infos.resize(desc.MipLevels * array_size * plane_count(desc.Format));
	for(auto& info : m_state_infos)
	{
		info.state = state;
		info.implicit = (state == D3D12_RESOURCE_STATE_COMMON);
		info.last_used_pos = 0;
		info.last_used_frame = 0;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//バッファか
inline bool resource::is_buffer() const
{
	return (mp_impl->GetDesc().Dimension == D3D12_RESOURCE_DIMENSION_BUFFER);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//テクスチャか
inline bool resource::is_texture() const
{
	return (mp_impl->GetDesc().Dimension != D3D12_RESOURCE_DIMENSION_BUFFER);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//buffer
///////////////////////////////////////////////////////////////////////////////////////////////////

//コンストラクタ
inline buffer::buffer(render_device& device, const uint stride, const uint num_elements, const resource_flags flags, const void* data, const D3D12_RESOURCE_STATES state) : render::buffer(stride, num_elements)
{
	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Width = roundup(stride * num_elements, 4); //structuredをbyteaddressとして使うときにエラー出ないように
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count = 1;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.Flags = convert(flags);

	D3D12_HEAP_PROPERTIES prop = {};
	prop.Type = D3D12_HEAP_TYPE_DEFAULT;

	init(device, prop, D3D12_HEAP_FLAG_NONE, desc, state, nullptr);

	if(data != nullptr)
	{
		upload_buffer tmp(device, uint(desc.Width), data);
		device.copy_buffer(*this, 0, tmp, 0, uint(desc.Width));
		tmp.enable_immediate_delete();
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//structured_buffer
///////////////////////////////////////////////////////////////////////////////////////////////////

//srvDescを生成
inline D3D12_SHADER_RESOURCE_VIEW_DESC structured_buffer::create_srv_desc(const buffer_srv_desc& desc) const
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
	srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srv_desc.Buffer.FirstElement = desc.first_element;
	srv_desc.Buffer.NumElements = desc.num_elements;
	srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	if(desc.force_byteaddress)
	{
		srv_desc.Format = DXGI_FORMAT_R32_TYPELESS;
		srv_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
	}
	else
	{
		srv_desc.Format = DXGI_FORMAT_UNKNOWN;
		srv_desc.Buffer.StructureByteStride = stride();
	}
	return srv_desc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//uavDescを生成
inline D3D12_UNORDERED_ACCESS_VIEW_DESC structured_buffer::create_uav_desc(const buffer_uav_desc& desc) const
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
	uav_desc.Format = DXGI_FORMAT_UNKNOWN;
	uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uav_desc.Buffer.FirstElement = desc.first_element;
	uav_desc.Buffer.NumElements = desc.num_elements;
	uav_desc.Buffer.StructureByteStride = stride();
	return uav_desc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//byteaddress_buffer
///////////////////////////////////////////////////////////////////////////////////////////////////

//srvDescを生成
inline D3D12_SHADER_RESOURCE_VIEW_DESC byteaddress_buffer::create_srv_desc(const buffer_srv_desc& desc) const
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
	srv_desc.Format = DXGI_FORMAT_R32_TYPELESS;
	srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srv_desc.Buffer.FirstElement = desc.first_element;
	srv_desc.Buffer.NumElements = desc.num_elements;
	srv_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
	srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	return srv_desc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//uavDescを生成
inline D3D12_UNORDERED_ACCESS_VIEW_DESC byteaddress_buffer::create_uav_desc(const buffer_uav_desc& desc) const
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
	uav_desc.Format = DXGI_FORMAT_R32_TYPELESS;
	uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uav_desc.Buffer.FirstElement = desc.first_element;
	uav_desc.Buffer.NumElements = desc.num_elements;
	uav_desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
	return uav_desc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//constant_buffer
///////////////////////////////////////////////////////////////////////////////////////////////////

//コンストラクタ
inline constant_buffer::constant_buffer(const uint size, cpu_descriptor view) : render::constant_buffer(size), m_view(std::move(view))
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//Viewを返す
inline constant_buffer::operator D3D12_CPU_DESCRIPTOR_HANDLE() const
{
	return m_view;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//static_constant_buffer
///////////////////////////////////////////////////////////////////////////////////////////////////

//コンストラクタ
inline static_constant_buffer::static_constant_buffer(render_device& device, const uint size, const void* data) : constant_buffer(size)
{
	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Width = size;
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count = 1;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	D3D12_HEAP_PROPERTIES prop = {};
	prop.Type = D3D12_HEAP_TYPE_UPLOAD;

	init(device, prop, D3D12_HEAP_FLAG_NONE, desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);
	m_view = device.create_constant_buffer_view(mp_impl->GetGPUVirtualAddress(), uint(size));

	mp_impl->Map(0, nullptr, &m_data);
	if(data != nullptr){ memcpy(m_data, data, size); }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//temporary_constant_buffer
///////////////////////////////////////////////////////////////////////////////////////////////////

//コンストラクタ
inline temporary_constant_buffer::temporary_constant_buffer(render_device& device, const uint size, const void* src_data, void* dst_data, cpu_descriptor view) : constant_buffer(size, std::move(view))
{
	m_data = dst_data;
	if(src_data != nullptr){ memcpy(m_data, src_data, size); }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//upload_buffer
///////////////////////////////////////////////////////////////////////////////////////////////////

//コンストラクタ
inline upload_buffer::upload_buffer(render_device& device, const uint size, const void* data) : render::upload_buffer(size)
{
	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Width = size;
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count = 1;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	D3D12_HEAP_PROPERTIES prop = {};
	prop.Type = D3D12_HEAP_TYPE_UPLOAD;

	init(device, prop, D3D12_HEAP_FLAG_NONE, desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);

	mp_impl->Map(0, nullptr, &m_data);
	if(data != nullptr){ memcpy(m_data, data, size); }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//デストラクタ
inline upload_buffer::~upload_buffer()
{
	mp_impl->Unmap(0, nullptr);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//readback_buffer
///////////////////////////////////////////////////////////////////////////////////////////////////

//コンストラクタ
inline readback_buffer::readback_buffer(render_device& device, const uint size) : render::readback_buffer(size)
{
	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Width = size;
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count = 1;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	D3D12_HEAP_PROPERTIES prop = {};
	prop.Type = D3D12_HEAP_TYPE_READBACK;

	init(device, prop, D3D12_HEAP_FLAG_NONE, desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr);

	mp_impl->Map(0, nullptr, &m_data);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//デストラクタ
inline readback_buffer::~readback_buffer()
{
	mp_impl->Unmap(0, nullptr);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//texture
///////////////////////////////////////////////////////////////////////////////////////////////////

//コンストラクタ
inline texture::texture(render_device& device, const texture_format format, const uint width, const uint height, const uint depth, const uint mip_levels, const resource_flags flags, const void* data, const D3D12_RESOURCE_DIMENSION dimension) : render::texture(format, width, height, depth, mip_levels, dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
{
	D3D12_RESOURCE_DESC desc = {};
	desc.Width = width;
	desc.Height = height;
	desc.DepthOrArraySize = depth;
	desc.MipLevels = mip_levels;
	desc.SampleDesc.Count = 1;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Flags = convert(flags);
	desc.Dimension = dimension;

	switch(format)
	{
	case texture_format_d16_unorm:
		desc.Format = DXGI_FORMAT_R16_TYPELESS;
		break;
	case texture_format_d32_float:
		desc.Format = DXGI_FORMAT_R32_TYPELESS;
		break;
	case texture_format_d24_unorm_s8_uint:
		desc.Format = DXGI_FORMAT_R24G8_TYPELESS;
		break;
	case texture_format_d32_float_s8x24_uint:
		desc.Format = DXGI_FORMAT_R32G8X24_TYPELESS;
		break;
	default:
		desc.Format = DXGI_FORMAT(format);
		break;
	}
	
	D3D12_HEAP_PROPERTIES prop = {};
	prop.Type = D3D12_HEAP_TYPE_DEFAULT;

	init(device, prop, D3D12_HEAP_FLAG_NONE, desc, D3D12_RESOURCE_STATE_COMMON, nullptr);

	if(data != nullptr)
	{
		const uint plane_count = d3d12::plane_count(desc.Format);
		if((plane_count == 2) || (dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D)) //必要になったら対応する
			throw;

		uint64_t total_bytes;
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT layouts[256];
		const uint num_subresources = depth * mip_levels * plane_count;
		device->GetCopyableFootprints(&desc, 0, num_subresources, 0, layouts, nullptr, nullptr, &total_bytes);

		upload_buffer upload(device, uint(total_bytes), nullptr);
		auto* dst = upload.data<uint8_t>();
		auto* src = static_cast<const uint8_t*>(data);
		const uint texel_size = render::texel_size(texture_format(desc.Format));
		
		uint64_t src_offset = 0; //dataはlod(i)のテクスチャ配列の後ろにlod(i+1)が並ぶようにする
		for(uint p = 0; p < plane_count; p++)
		{
			for(uint l = 0; l < desc.MipLevels; l++)
			{
				for(uint i = 0; i < desc.DepthOrArraySize; i++)
				{
					const auto& layout = layouts[subresource_index(*this, l, i, p)];
					const auto& footprint = layout.Footprint;

					uint64_t dst_offset = layout.Offset;
					for(uint y = 0; y < footprint.Height; y++)
					{
						memcpy(dst + dst_offset, src + texel_size * src_offset, texel_size * footprint.Width);
						dst_offset += footprint.RowPitch;
						src_offset += footprint.Width;
					}
				}
			}
		}
		device.copy_texture(*this, upload, layouts, num_subresources);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//コンストラクタ(BackBuffer用)
inline texture::texture(ComPtr<ID3D12Resource> p_target) : render::texture(texture_format(p_target->GetDesc().Format), uint(p_target->GetDesc().Width), uint(p_target->GetDesc().Height), 1, 1, false)
{
	auto desc = p_target->GetDesc();
	init(std::move(p_target), desc, D3D12_RESOURCE_STATE_COMMON);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//texture1d
///////////////////////////////////////////////////////////////////////////////////////////////////

//SrvDescを生成
inline D3D12_SHADER_RESOURCE_VIEW_DESC texture1d::create_srv_desc(const texture_srv_desc& desc) const
{
	assert(desc.flag == srv_flag_none);
	D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
	srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
	srv_desc.Format = convert(mp_impl->GetDesc().Format, srv_flag_none);
	srv_desc.Texture1D.MipLevels = desc.mip_levels;
	srv_desc.Texture1D.MostDetailedMip = desc.most_detailed_mip;
	srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	return srv_desc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//UavDescを生成
inline D3D12_UNORDERED_ACCESS_VIEW_DESC texture1d::create_uav_desc(const texture_uav_desc& desc) const
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
	uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
	uav_desc.Format = mp_impl->GetDesc().Format;
	uav_desc.Texture1D.MipSlice = desc.mip_slice;
	return uav_desc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//texture2d
///////////////////////////////////////////////////////////////////////////////////////////////////

//RtvDescを生成
inline D3D12_RENDER_TARGET_VIEW_DESC texture2d::create_rtv_desc(const texture_rtv_desc& desc) const
{
	D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {};
	rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtv_desc.Format = mp_impl->GetDesc().Format;
	rtv_desc.Texture2D.MipSlice = desc.mip_slice;
	return rtv_desc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//DsvDescを生成
inline D3D12_DEPTH_STENCIL_VIEW_DESC texture2d::create_dsv_desc(const texture_dsv_desc& desc) const
{
	D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
	dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsv_desc.Format = convert(mp_impl->GetDesc().Format);
	dsv_desc.Texture2D.MipSlice = desc.mip_slice;
	dsv_desc.Flags = convert(desc.flags);
	return dsv_desc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//SrvDescを生成
inline D3D12_SHADER_RESOURCE_VIEW_DESC texture2d::create_srv_desc(const texture_srv_desc& desc) const
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
	srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srv_desc.Format = convert(mp_impl->GetDesc().Format, desc.flag);
	srv_desc.Texture2D.MipLevels = desc.mip_levels;
	srv_desc.Texture2D.MostDetailedMip = desc.most_detailed_mip;
	srv_desc.Texture2D.PlaneSlice = (desc.flag & srv_flag_stencil) ? 1 : 0;
	srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	return srv_desc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//UavDescを生成
inline D3D12_UNORDERED_ACCESS_VIEW_DESC texture2d::create_uav_desc(const texture_uav_desc& desc) const
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
	uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uav_desc.Format = mp_impl->GetDesc().Format;
	uav_desc.Texture2D.MipSlice = desc.mip_slice;
	return uav_desc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//texture3d
///////////////////////////////////////////////////////////////////////////////////////////////////

//RtvDescを生成
inline D3D12_RENDER_TARGET_VIEW_DESC texture3d::create_rtv_desc(const texture_rtv_desc& desc) const
{
	D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {};
	rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
	rtv_desc.Format = mp_impl->GetDesc().Format;
	rtv_desc.Texture3D.MipSlice = desc.mip_slice;
	rtv_desc.Texture3D.WSize = desc.array_size;
	rtv_desc.Texture3D.FirstWSlice = desc.first_array_index;
	return rtv_desc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//SrvDescを生成
inline D3D12_SHADER_RESOURCE_VIEW_DESC texture3d::create_srv_desc(const texture_srv_desc& desc) const
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
	srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
	srv_desc.Format = convert(mp_impl->GetDesc().Format, desc.flag);
	srv_desc.Texture3D.MipLevels = desc.mip_levels;
	srv_desc.Texture3D.MostDetailedMip = desc.most_detailed_mip;
	srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	return srv_desc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//UavDescを生成
inline D3D12_UNORDERED_ACCESS_VIEW_DESC texture3d::create_uav_desc(const texture_uav_desc& desc) const
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
	uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
	uav_desc.Format = mp_impl->GetDesc().Format;
	uav_desc.Texture3D.MipSlice = desc.mip_slice;
	uav_desc.Texture3D.WSize = desc.array_size;
	uav_desc.Texture3D.FirstWSlice = desc.first_array_index;
	return uav_desc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//texture_cube
///////////////////////////////////////////////////////////////////////////////////////////////////

//RtvDescを生成
inline D3D12_RENDER_TARGET_VIEW_DESC texture_cube::create_rtv_desc(const texture_rtv_desc& desc) const
{
	D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {};
	rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
	rtv_desc.Format = mp_impl->GetDesc().Format;
	rtv_desc.Texture2DArray.MipSlice = desc.mip_slice;
	rtv_desc.Texture2DArray.ArraySize = desc.array_size;
	rtv_desc.Texture2DArray.FirstArraySlice = desc.first_array_index;
	return rtv_desc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//DsvDescを生成
inline D3D12_DEPTH_STENCIL_VIEW_DESC texture_cube::create_dsv_desc(const texture_dsv_desc& desc) const
{
	D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
	dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
	dsv_desc.Format = convert(mp_impl->GetDesc().Format);
	dsv_desc.Texture2DArray.MipSlice = desc.mip_slice;
	dsv_desc.Texture2DArray.ArraySize = desc.array_size;
	dsv_desc.Texture2DArray.FirstArraySlice = desc.first_array_index;
	dsv_desc.Flags = convert(desc.flags);
	return dsv_desc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//SrvDescを生成
inline D3D12_SHADER_RESOURCE_VIEW_DESC texture_cube::create_srv_desc(const texture_srv_desc& desc) const
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
	srv_desc.Format = convert(mp_impl->GetDesc().Format, desc.flag);
	srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	if(desc.flag & srv_flag_cube_as_array)
	{
		srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		srv_desc.Texture2DArray.MipLevels = desc.mip_levels;
		srv_desc.Texture2DArray.MostDetailedMip = desc.most_detailed_mip;
		srv_desc.Texture2DArray.PlaneSlice = (desc.flag & srv_flag_stencil) ? 1 : 0;
		srv_desc.Texture2DArray.ArraySize = 6;
		srv_desc.Texture2DArray.FirstArraySlice = desc.first_array_index;
	}
	else
	{
		srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srv_desc.TextureCube.MipLevels = desc.mip_levels;
		srv_desc.TextureCube.MostDetailedMip = desc.most_detailed_mip;
	}
	return srv_desc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//UavDescを生成
inline D3D12_UNORDERED_ACCESS_VIEW_DESC texture_cube::create_uav_desc(const texture_uav_desc& desc) const
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
	uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
	uav_desc.Format = mp_impl->GetDesc().Format;
	uav_desc.Texture2DArray.MipSlice = desc.mip_slice;
	uav_desc.Texture2DArray.ArraySize = desc.array_size;
	uav_desc.Texture2DArray.FirstArraySlice = desc.first_array_index;
	return uav_desc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//texture1d_array
///////////////////////////////////////////////////////////////////////////////////////////////////

//SrvDescを生成
inline D3D12_SHADER_RESOURCE_VIEW_DESC texture1d_array::create_srv_desc(const texture_srv_desc& desc) const
{
	assert(desc.flag == srv_flag_none);
	D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
	srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
	srv_desc.Format = convert(mp_impl->GetDesc().Format, srv_flag_none);
	srv_desc.Texture1DArray.MipLevels = desc.mip_levels;
	srv_desc.Texture1DArray.MostDetailedMip = desc.most_detailed_mip;
	srv_desc.Texture1DArray.ArraySize = desc.array_size;
	srv_desc.Texture1DArray.FirstArraySlice = desc.first_array_index;
	srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	return srv_desc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//UavDescを生成
inline D3D12_UNORDERED_ACCESS_VIEW_DESC texture1d_array::create_uav_desc(const texture_uav_desc& desc) const
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
	uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
	uav_desc.Format = mp_impl->GetDesc().Format;
	uav_desc.Texture1DArray.MipSlice = desc.mip_slice;
	uav_desc.Texture1DArray.ArraySize = desc.array_size;
	uav_desc.Texture1DArray.FirstArraySlice = desc.first_array_index;
	return uav_desc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//texture2d_array
///////////////////////////////////////////////////////////////////////////////////////////////////

//RtvDescを生成
inline D3D12_RENDER_TARGET_VIEW_DESC texture2d_array::create_rtv_desc(const texture_rtv_desc& desc) const
{
	D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {};
	rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
	rtv_desc.Format = mp_impl->GetDesc().Format;
	rtv_desc.Texture2DArray.MipSlice = desc.mip_slice;
	rtv_desc.Texture2DArray.ArraySize = desc.array_size;
	rtv_desc.Texture2DArray.FirstArraySlice = desc.first_array_index;
	return rtv_desc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//DsvDescを生成
inline D3D12_DEPTH_STENCIL_VIEW_DESC texture2d_array::create_dsv_desc(const texture_dsv_desc& desc) const
{
	D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
	dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
	dsv_desc.Format = convert(mp_impl->GetDesc().Format);
	dsv_desc.Texture2DArray.MipSlice = desc.mip_slice;
	dsv_desc.Texture2DArray.ArraySize = desc.array_size;
	dsv_desc.Texture2DArray.FirstArraySlice = desc.first_array_index;
	dsv_desc.Flags = convert(desc.flags);
	return dsv_desc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//SrvDescを生成
inline D3D12_SHADER_RESOURCE_VIEW_DESC texture2d_array::create_srv_desc(const texture_srv_desc& desc) const
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
	srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
	srv_desc.Format = convert(mp_impl->GetDesc().Format, desc.flag);
	srv_desc.Texture2DArray.MipLevels = desc.mip_levels;
	srv_desc.Texture2DArray.MostDetailedMip = desc.most_detailed_mip;
	srv_desc.Texture2DArray.PlaneSlice = (desc.flag & srv_flag_stencil) ? 1 : 0;
	srv_desc.Texture2DArray.ArraySize = desc.array_size;
	srv_desc.Texture2DArray.FirstArraySlice = desc.first_array_index;
	srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	return srv_desc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//UavDescを生成
inline D3D12_UNORDERED_ACCESS_VIEW_DESC texture2d_array::create_uav_desc(const texture_uav_desc& desc) const
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
	uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
	uav_desc.Format = mp_impl->GetDesc().Format;
	uav_desc.Texture2DArray.MipSlice = desc.mip_slice;
	uav_desc.Texture2DArray.ArraySize = desc.array_size;
	uav_desc.Texture2DArray.FirstArraySlice = desc.first_array_index;
	return uav_desc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//texture_cube_array
///////////////////////////////////////////////////////////////////////////////////////////////////

//RtvDescを生成
inline D3D12_RENDER_TARGET_VIEW_DESC texture_cube_array::create_rtv_desc(const texture_rtv_desc& desc) const
{
	throw;
	//D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {};
	//rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
	//rtv_desc.Format = mp_impl->GetDesc().Format;
	//rtv_desc.Texture2DArray.MipSlice = desc.mip_slice;
	//rtv_desc.Texture2DArray.ArraySize = desc.array_size;
	//rtv_desc.Texture2DArray.FirstArraySlice = desc.first_array_index;
	//return rtv_desc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//DsvDescを生成
inline D3D12_DEPTH_STENCIL_VIEW_DESC texture_cube_array::create_dsv_desc(const texture_dsv_desc& desc) const
{
	throw;
	//D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
	//dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
	//dsv_desc.Format = convert(mp_impl->GetDesc().Format);
	//dsv_desc.Texture2DArray.MipSlice = desc.mip_slice;
	//dsv_desc.Texture2DArray.ArraySize= desc.array_size;
	//dsv_desc.Texture2DArray.FirstArraySlice = desc.first_array_index;
	//dsv_desc.Flags = convert(desc.flags);
	//return dsv_desc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//SrvDescを生成
inline D3D12_SHADER_RESOURCE_VIEW_DESC texture_cube_array::create_srv_desc(const texture_srv_desc& desc) const
{
	throw;
	//D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
	//srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
	//srv_desc.Format = convert(mp_impl->GetDesc().Format, desc.flag);
	//srv_desc.TextureCubeArray.MipLevels = desc.mip_levels;
	//srv_desc.TextureCubeArray.MostDetailedMip = desc.most_detailed_mip;
	//srv_desc.TextureCubeArray.NumCubes = desc.array_size;
	//srv_desc.TextureCubeArray.First2DArrayFace = desc.first_array_index;
	//srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	//return srv_desc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//UavDescを生成
inline D3D12_UNORDERED_ACCESS_VIEW_DESC texture_cube_array::create_uav_desc(const texture_uav_desc& desc) const
{
	throw;
	//D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
	//uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
	//uav_desc.Format = mp_impl->GetDesc().Format;
	//uav_desc.Texture2DArray.MipSlice = desc.mip_slice;
	//uav_desc.Texture2DArray.ArraySize = desc.array_size;
	//uav_desc.Texture2DArray.FirstArraySlice = desc.first_array_index;
	//return uav_desc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//top_level_acceleration_structure
///////////////////////////////////////////////////////////////////////////////////////////////////

//コンストラクタ
inline top_level_acceleration_structure::top_level_acceleration_structure(render_device& device, const uint num_instances)
{
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs;
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.NumDescs = num_instances;
	inputs.InstanceDescs = 0; //メモリ使用量を知るだけの場合は0でよい
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
	device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Width = info.ResultDataMaxSizeInBytes;
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count = 1;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.Flags = D3D12_RESOURCE_FLAG_RAYTRACING_ACCELERATION_STRUCTURE | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	D3D12_HEAP_PROPERTIES prop = {};
	prop.Type = D3D12_HEAP_TYPE_DEFAULT;

	init(device, prop, D3D12_HEAP_FLAG_NONE, desc, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr);

	mp_scratch.reset(new structured_buffer(device, uint(info.ScratchDataSizeInBytes), 1, resource_flag_allow_unordered_access, nullptr, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	mp_instance_descs.reset(new structured_buffer(device, uint(sizeof(raytracing_instance_desc)), num_instances, resource_flag_allow_shader_resource, nullptr));
	render::top_level_acceleration_structure::mp_instance_descs = mp_instance_descs.get();
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTSを返す
inline D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS top_level_acceleration_structure::build_inputs(const uint num_instances) const
{
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs;
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.NumDescs = num_instances;
	inputs.InstanceDescs = (*mp_instance_descs)->GetGPUVirtualAddress();
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	return inputs;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//bottom_level_acceleration_structure
///////////////////////////////////////////////////////////////////////////////////////////////////

//コンストラクタ
inline bottom_level_acceleration_structure::bottom_level_acceleration_structure(render_device& device, const geometry_state& gs, const raytracing_geometry_desc* descs, const uint num_descs, const bool allow_update) : m_allow_update(allow_update)
{
	auto& ib = gs.index_buffer();
	auto& vb = gs.vertex_buffer(0);
	assert(ib.stride() == 2);
	assert(gs.stride(0) >= 12);
	m_vb_address = vb->GetGPUVirtualAddress();
	m_ib_address = ib->GetGPUVirtualAddress();
	
	m_descs.resize(num_descs);
	for(uint i = 0; i < num_descs; i++)
	{
		m_descs[i].Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		m_descs[i].Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
		m_descs[i].Triangles.IndexBuffer = ib->GetGPUVirtualAddress() + ib.stride() * descs[i].start_index_location;
		m_descs[i].Triangles.IndexCount = descs[i].index_count;
		m_descs[i].Triangles.IndexFormat = DXGI_FORMAT_R16_UINT;
		m_descs[i].Triangles.VertexBuffer.StartAddress = vb->GetGPUVirtualAddress() + gs.offset(0) + gs.stride(0) * descs[i].base_vertex_location;
		m_descs[i].Triangles.VertexBuffer.StrideInBytes = gs.stride(0);
		m_descs[i].Triangles.VertexCount = descs[i].vertex_count;
		m_descs[i].Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
	}
	
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = build_inputs();
	device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);
	
	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Width = info.ResultDataMaxSizeInBytes;
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count = 1;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.Flags = D3D12_RESOURCE_FLAG_RAYTRACING_ACCELERATION_STRUCTURE | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	
	D3D12_HEAP_PROPERTIES prop = {};
	prop.Type = D3D12_HEAP_TYPE_DEFAULT;
	
	init(device, prop, D3D12_HEAP_FLAG_NONE, desc, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr);
	m_gpu_virtual_address = mp_impl->GetGPUVirtualAddress();

	mp_scratch.reset(new structured_buffer(device, uint(info.ScratchDataSizeInBytes), 1, resource_flag_allow_unordered_access, nullptr, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	if(not(allow_update))
	{
		mp_compacted_size.reset(new structured_buffer(device, uint(sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE_DESC)), 1, resource_flag_allow_unordered_access, nullptr, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
		mp_compacted_size_readback.reset(new readback_buffer(device, uint(sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE_DESC))));
		mp_compacted_size->enable_immediate_delete();
		mp_compacted_size_readback->enable_immediate_delete();
		m_compaction_state = raytracing_compaction_state_not_ready;
	}
}
	
///////////////////////////////////////////////////////////////////////////////////////////////////

//ジオメトリを更新
inline void bottom_level_acceleration_structure::update_geometry(const geometry_state& gs)
{
	auto& ib = gs.index_buffer();
	auto& vb = gs.vertex_buffer(0);
	for(size_t i = 0; i < m_descs.size(); i++)
	{
		m_descs[i].Triangles.IndexBuffer -= m_ib_address;
		m_descs[i].Triangles.IndexBuffer += ib->GetGPUVirtualAddress();
		m_descs[i].Triangles.VertexBuffer.StartAddress -= m_vb_address;
		m_descs[i].Triangles.VertexBuffer.StartAddress += vb->GetGPUVirtualAddress();
	}
	m_vb_address = vb->GetGPUVirtualAddress();
	m_ib_address = ib->GetGPUVirtualAddress();
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTSを返す
inline D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS bottom_level_acceleration_structure::build_inputs() const
{
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs;
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.NumDescs = uint(m_descs.size());
	inputs.pGeometryDescs = m_descs.data();
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	if(m_allow_update)
		inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
	else
		inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION;
	return inputs;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//コンパクションの状態を設定
inline void bottom_level_acceleration_structure::set_compaction_state(render_device& device, const raytracing_compaction_state state)
{
	m_compaction_state = state;
	if(m_compaction_state == raytracing_compaction_state_progress2)
	{
		auto desc = mp_compacted_size_readback->data<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE_DESC>();
		mp_compacted_result.reset(new structured_buffer(device, uint(desc->CompactedSizeInBytes), 1, resource_flags(resource_flag_raytracing_acceleration_structure | resource_flag_allow_shader_resource | resource_flag_allow_unordered_access), nullptr, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE));
		mp_compacted_size.reset();
		mp_compacted_size_readback.reset();
	}
	else if(m_compaction_state == raytracing_compaction_state_completed)
	{
		std::swap(static_cast<resource&>(*this), static_cast<resource&>(*mp_compacted_result)); //元のが即死しないようにswap
		m_gpu_virtual_address = mp_impl->GetGPUVirtualAddress();
		mp_compacted_result.reset();
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//graphics_pipeline_state
///////////////////////////////////////////////////////////////////////////////////////////////////

//コンストラクタ
inline graphics_pipeline_state::graphics_pipeline_state(const string& name, shader_ptr p_vs, shader_ptr p_ps, const input_layout_desc& il, const rasterizer_desc& rs, const depth_stencil_desc& dss, const blend_desc& bs) : pipeline_state(name), mp_vs(std::move(p_vs)), mp_ps(std::move(p_ps)), m_il(il), m_rs(rs), m_dss(dss), m_bs(bs)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//パイプラインステートを返す
inline const graphics_pipeline_state::record &graphics_pipeline_state::get(render_device& device, const target_state& ts, const geometry_state& gs) const
{
	size_t hash;
	{
		uint key[10] = {};
		for(uint i = 0; i < ts.num_rtvs(); i++)
			key[i] = ts.rtv(i).format();
		if(ts.has_dsv())
			key[8] = ts.dsv().format();
		if((&gs != nullptr) && gs.has_index_buffer())
		//if(gs.has_index_buffer())
			key[9] = gs.index_buffer().stride();
		hash = calc_hash(key);
	}

	auto it = std::find_if(m_records.begin(), m_records.end(), [&](const auto& r){ return (r.hash == hash); });
	auto& record = (it == m_records.end()) ? m_records.emplace_back(hash) : *it;
	if(record.p_pipeline_state == nullptr)
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
		desc.SampleDesc.Count = 1;
        desc.SampleMask = UINT_MAX;

		D3D12_INPUT_ELEMENT_DESC input_element_descs[8];
		for(uint i = 0; i < m_il.num_elements; i++)
		{
			input_element_descs[i].Format = DXGI_FORMAT(m_il.element_descs[i].format);
			input_element_descs[i].InputSlot = m_il.element_descs[i].slot;
			input_element_descs[i].SemanticName = m_il.element_descs[i].semantic_name.c_str();
			input_element_descs[i].SemanticIndex = m_il.element_descs[i].semantic_index;
			input_element_descs[i].AlignedByteOffset = m_il.element_descs[i].offset;
			input_element_descs[i].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
			input_element_descs[i].InstanceDataStepRate = 0;
		}
		desc.InputLayout.NumElements = m_il.num_elements;
		desc.InputLayout.pInputElementDescs = input_element_descs;
		
		if((&gs != nullptr) && gs.has_index_buffer())
		//if(gs.has_index_buffer())
		{
			if(gs.index_buffer().stride() == 2)
				desc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFF;
			else if(gs.index_buffer().stride() == 4)
				desc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFFFFFF;
		}

		switch(m_il.topology)
		{
		case topology_type_point_list:
			desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
			break;
		case topology_type_line_list:
		case topology_type_line_strip:
			desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
			break;
		case topology_type_triangle_list:
		case topology_type_triangle_strip:
			desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			break;
		default:
			desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
		}
		
		desc.NumRenderTargets = ts.num_rtvs();
		for(uint i = 0; i < ts.num_rtvs(); i++)
			desc.RTVFormats[i] = ts.rtv(i).format();
		if(ts.has_dsv())
			desc.DSVFormat = ts.dsv().format();

		desc.RasterizerState.ConservativeRaster = m_rs.conservative ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
		desc.RasterizerState.CullMode = D3D12_CULL_MODE(m_rs.cull_mode);
		desc.RasterizerState.DepthClipEnable = true;
		desc.RasterizerState.DepthBias = m_rs.depth_bias;
		desc.RasterizerState.SlopeScaledDepthBias = m_rs.slope_scaled_depth_bias;
		desc.RasterizerState.DepthBiasClamp = m_rs.depth_bias_clamp;
		desc.RasterizerState.FillMode = m_rs.fill ? D3D12_FILL_MODE_SOLID : D3D12_FILL_MODE_WIREFRAME;
		desc.RasterizerState.FrontCounterClockwise = true;
		desc.RasterizerState.MultisampleEnable = false;
		desc.RasterizerState.AntialiasedLineEnable = false;
		desc.RasterizerState.ForcedSampleCount = 0;

		desc.DepthStencilState.DepthEnable = m_dss.depth_enabale;
		desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC(m_dss.depth_func);
		desc.DepthStencilState.DepthWriteMask = m_dss.depth_write ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
		desc.DepthStencilState.StencilEnable = m_dss.stencil_enable;
		desc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC(m_dss.front.func);
		desc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP(m_dss.front.pass_op);
		desc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP(m_dss.front.stencil_fail_op);
		desc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP(m_dss.front.depth_fail_op);
		desc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC(m_dss.back.func);
		desc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP(m_dss.back.pass_op);
		desc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP(m_dss.back.stencil_fail_op);
		desc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP(m_dss.back.depth_fail_op);
		desc.DepthStencilState.StencilReadMask = m_dss.stencil_read_mask;
		desc.DepthStencilState.StencilWriteMask = m_dss.stencil_write_mask;

		desc.BlendState.AlphaToCoverageEnable = false;
		desc.BlendState.IndependentBlendEnable = m_bs.independent_blend;
		
		for(uint i = 0, num = desc.BlendState.IndependentBlendEnable ? ts.num_rtvs() : 1; i < num; i++)
		{
			desc.BlendState.RenderTarget[i].BlendEnable = m_bs.render_target[i].enable;
			desc.BlendState.RenderTarget[i].BlendOp = D3D12_BLEND_OP(m_bs.render_target[i].op);
			desc.BlendState.RenderTarget[i].BlendOpAlpha = D3D12_BLEND_OP(m_bs.render_target[i].op_alpha);
			desc.BlendState.RenderTarget[i].SrcBlend = D3D12_BLEND(m_bs.render_target[i].src);
			desc.BlendState.RenderTarget[i].SrcBlendAlpha = D3D12_BLEND(m_bs.render_target[i].src_alpha);
			desc.BlendState.RenderTarget[i].DestBlend = D3D12_BLEND(m_bs.render_target[i].dst);
			desc.BlendState.RenderTarget[i].DestBlendAlpha = D3D12_BLEND(m_bs.render_target[i].dst_alpha);
			desc.BlendState.RenderTarget[i].RenderTargetWriteMask = m_bs.render_target[i].write_mask;
			desc.BlendState.RenderTarget[i].LogicOpEnable = false;
			desc.BlendState.RenderTarget[i].LogicOp = D3D12_LOGIC_OP_CLEAR;
		}

		desc.VS.BytecodeLength = static_cast<d3d12::shader*>(mp_vs.get())->blob().GetBufferSize();
		desc.VS.pShaderBytecode = static_cast<d3d12::shader*>(mp_vs.get())->blob().GetBufferPointer();
		desc.PS.BytecodeLength = static_cast<d3d12::shader*>(mp_ps.get())->blob().GetBufferSize();
		desc.PS.pShaderBytecode = static_cast<d3d12::shader*>(mp_ps.get())->blob().GetBufferPointer();

		const shader* shader_ptrs[] = 
		{
			static_cast<const d3d12::shader*>(mp_vs.get()), 
			static_cast<const d3d12::shader*>(mp_ps.get()) 
		};
		auto rs = device.create_root_signature(shader_ptrs, 2);
		desc.pRootSignature = rs.first.Get();

		record.p_root_signature = std::move(rs.first);
		record.pipeline_resource_count = rs.second;
		auto hr = device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&record.p_pipeline_state));
		//if(!SUCCEEDED(hr))
		//	watch(hr);
	}
	return record;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//compute_pipeline_state
///////////////////////////////////////////////////////////////////////////////////////////////////

//コンストラクタ
inline compute_pipeline_state::compute_pipeline_state(render_device& device, const string& name, shader_ptr p_cs) : pipeline_state(name), mp_cs(std::move(p_cs))
{
	auto p_shader = static_cast<const d3d12::shader*>(mp_cs.get());
	auto rs = device.create_root_signature(&p_shader, 1);
	m_resource_count = rs.second.shader_resource_count_table[0];
	mp_rs = std::move(rs.first);

	D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
	desc.CS.BytecodeLength = p_shader->blob().GetBufferSize();
	desc.CS.pShaderBytecode = p_shader->blob().GetBufferPointer();
	desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	desc.pRootSignature = mp_rs.Get();

	device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&mp_impl));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//shader
///////////////////////////////////////////////////////////////////////////////////////////////////

//コンストラクタ
inline shader::shader(ComPtr<ID3DBlob> p_blob, ComPtr<ID3D12ShaderReflection> p_reflection) : mp_blob(std::move(p_blob)), mp_reflection(std::move(p_reflection))
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//バイナリを返す
inline ID3DBlob& shader::blob() const
{
	return *mp_blob.Get();
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//リフレクションを返す
inline ID3D12ShaderReflection& shader::reflection() const
{
	return *mp_reflection.Get();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//shader_compiler::include_handler
///////////////////////////////////////////////////////////////////////////////////////////////////

class shader_compiler::include_handler : public IDxcIncludeHandler
{
public:

	include_handler(IDxcUtils& utils) : mp_utils(&utils)
	{
	}

	unordered_set<wstring>& include_files()
	{
		return m_include_files;
	}

	HRESULT LoadSource(LPCWSTR filename, IDxcBlob** pp_result) override
	{
		ComPtr<IDxcBlobEncoding> p_encoding;
		
		const wstring path = format(shader_src_dir + filename);
		const wstring relative_path = std::filesystem::relative(path, shader_src_dir);

		//インクルード済み
		if(not(m_include_files.emplace(relative_path).second))
		{
			static const char nullStr[] = " ";
			mp_utils->CreateBlobFromPinned(nullStr, ARRAYSIZE(nullStr), DXC_CP_ACP, p_encoding.GetAddressOf());
			*pp_result = p_encoding.Detach();
			return S_OK;
		}
		//新規
		else
		{
			HRESULT hr = mp_utils->LoadFile(path.c_str(), nullptr, p_encoding.GetAddressOf());
			if(SUCCEEDED(hr))
				*pp_result = p_encoding.Detach();
			return hr;
		}
	}

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject) override { return E_NOINTERFACE; }
	ULONG STDMETHODCALLTYPE AddRef(void) override { return 0; }
	ULONG STDMETHODCALLTYPE Release(void) override { return 0; }

private:

	IDxcUtils*				mp_utils;
	unordered_set<wstring>	m_include_files;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//shader_compiler
///////////////////////////////////////////////////////////////////////////////////////////////////

//コンストラクタ
inline shader_compiler::shader_compiler()
{
	DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&mp_utils));
	DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&mp_compiler));
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//コンパイル
inline shader_ptr shader_compiler::compile(const wstring& filename, const wstring& entry_point, const wstring& target, const wstring& orig_defines, unordered_set<wstring>& dependent)
{
	const wstring stem = filename.substr(0, filename.find_last_of(L'.'));
	//const wstring bin_filename = stem + L".bin";
	//const wstring pdb_filename = stem + L".pdb"; //pixが探すファイル名と一致しなくなるので指定しない(なんか設定いる？)

	size_t num_defines = 0;
	wstring defines = orig_defines;
	for(size_t offset = 0; ; num_defines++)
	{
		if(const size_t pos = defines.find(L"-D", offset); pos != defines.npos)
			offset = pos + 2;
		else
			break;
	}

	std::vector<const wchar_t*> args;
	args.reserve(12 + 2 * num_defines);
	args.push_back(filename.c_str());
	args.push_back(L"-T");
	args.push_back(target.c_str());
	args.push_back(L"-I");
	args.push_back(L"shader/src/");
	//args.push_back(L"-Fo");
	//args.push_back(bin_filename.c_str());
	//args.push_back(L"-Fd");
	//args.push_back(pdb_filename.c_str());
	args.push_back(L"-Zs");
	args.push_back(L"-enable-16bit-types");

	if(entry_point.size()) //ライブラリの場合はentry_pointなし
	{
		args.push_back(L"-E");
		args.push_back(entry_point.c_str());
	}

	for(size_t offset = 0; ; num_defines++)
	{
		const size_t pos = defines.find(L"-D", offset);
		if(pos == defines.npos)
			break;

		offset = pos + 2;
		while(iswspace(defines[offset])){ offset++; }

		size_t end = offset + 1;
		for(; end < defines.size(); end++)
		{
			if(iswspace(defines[end]))
			{
				defines[end] = L'\0';
				break;
			}
		}
		args.push_back(L"-D");
		args.push_back(&defines[offset]);
		offset = end + 1;
	}

	ComPtr<IDxcBlobEncoding> p_src;
	mp_utils->LoadFile((shader_src_dir + filename).c_str(), nullptr, &p_src);
	if(p_src == nullptr)
		throw std::runtime_error(utf16_to_utf8(shader_src_dir + filename + L"の読み込みに失敗しました"));

	DxcBuffer buf;
	buf.Ptr = p_src->GetBufferPointer();
	buf.Size = p_src->GetBufferSize();
	buf.Encoding = DXC_CP_ACP;

	ComPtr<IDxcResult> p_result;
	include_handler include_handler(*mp_utils.Get());
	mp_compiler->Compile(&buf, args.data(), uint(args.size()), &include_handler, IID_PPV_ARGS(&p_result));

	ComPtr<IDxcBlobUtf8> p_error;
	p_result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&p_error), nullptr);
	if((p_error != nullptr) && (p_error->GetStringLength() > 0))
	{
		throw std::runtime_error(p_error->GetStringPointer());
	}
	//if(HRESULT hr; p_result->GetStatus(&hr), FAILED(hr))
	//{
	//	return nullptr;
	//}

	ComPtr<ID3DBlob> p_blob;
	ComPtr<IDxcBlob> p_dxc_blob;
	p_result->GetResult(&p_dxc_blob);
	D3DCreateBlob(p_dxc_blob->GetBufferSize(), &p_blob);
	memcpy(p_blob->GetBufferPointer(), p_dxc_blob->GetBufferPointer(), p_dxc_blob->GetBufferSize());

	//使い道がわからないのでとりあえず放置
#if 0
	{
		ComPtr<IDxcBlob> p_shader;
		ComPtr<IDxcBlobUtf16> p_shader_name;
		p_result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&p_shader), &p_shader_name);
		check_result(p_shader != nullptr);

		std::wstring filename(p_shader_name->GetStringPointer());
		std::ofstream ofs((shader_bin_dir + filename).c_str(), std::ios::binary);
		ofs.write(static_cast<const char *>(p_shader->GetBufferPointer()), p_shader->GetBufferSize());
	}
#endif
	{
		ComPtr<IDxcBlob> p_pdb;
		ComPtr<IDxcBlobUtf16> p_pdb_name;
		p_result->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(&p_pdb), &p_pdb_name);
		check_result(p_pdb != nullptr);

		std::wstring filename(p_pdb_name->GetStringPointer());
		std::ofstream ofs((shader_pdb_dir + filename).c_str(), std::ios::binary);
		ofs.write(static_cast<const char *>(p_pdb->GetBufferPointer()), p_pdb->GetBufferSize());
	}

	ComPtr<IDxcBlob> p_reflection_blob;
	p_result->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(&p_reflection_blob), nullptr);
	check_result(p_reflection_blob != nullptr);

	DxcBuffer reflection_buf;
	reflection_buf.Ptr = p_reflection_blob->GetBufferPointer();
	reflection_buf.Size = p_reflection_blob->GetBufferSize();
	reflection_buf.Encoding = DXC_CP_ACP;

	ComPtr<ID3D12ShaderReflection> p_reflection;
	//ComPtr<ID3D12LibraryReflection> p_reflection;
	mp_utils->CreateReflection(&reflection_buf, IID_PPV_ARGS(&p_reflection));
	dependent.merge(std::move(include_handler.include_files()));
	return new shader(std::move(p_blob), std::move(p_reflection));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//render_device_impl
///////////////////////////////////////////////////////////////////////////////////////////////////

//コンストラクタ
inline render_device_impl::render_device_impl()
{
	ComPtr<IDXGIFactory> p_factory;
	check_hresult(CreateDXGIFactory2(0, IID_PPV_ARGS(&p_factory)));

	ComPtr<IDXGIAdapter1> p_adapter;
	for(uint i = 0; p_factory->EnumAdapters1(i, &p_adapter) != DXGI_ERROR_NOT_FOUND; i++)
	{
		DXGI_ADAPTER_DESC1 desc;
		p_adapter->GetDesc1(&desc);

		//ソフトウェアアダプタは無視
		if(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			continue;

		//D3D12非サポートは無視
		ComPtr<ID3D12Device> p_device;
		if(FAILED(D3D12CreateDevice(p_adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&p_device))))
			continue;

		//レイトレ非対応は無視
		D3D12_FEATURE_DATA_D3D12_OPTIONS5 feature_support = {};
		if(FAILED(p_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &feature_support, sizeof(feature_support))))
			continue;
		if(feature_support.RaytracingTier == D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
			continue;

		//デバイス採用
		mp_impl = std::move(p_device);
	}
	check_result(mp_impl != nullptr);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//swapchain
///////////////////////////////////////////////////////////////////////////////////////////////////

//コンストラクタ
inline swapchain::swapchain(ID3D12CommandQueue& queue, const HWND hwnd, const uint2 size) : m_hdr_mode()
{
	ComPtr<IDXGIFactory> p_factory;
	check_hresult(CreateDXGIFactory2(0, IID_PPV_ARGS(&p_factory)));

	BOOL allow_tearing = FALSE;
	check_hresult(p_factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allow_tearing, sizeof(allow_tearing)));
	check_result(allow_tearing == TRUE);

	DXGI_SWAP_CHAIN_DESC1 desc = {};
	desc.Width = size.x;
	desc.Height = size.y;
	desc.Format = DXGI_FORMAT_R10G10B10A2_UNORM;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.BufferCount = buffer_count();
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Scaling = DXGI_SCALING_NONE;
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
	desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING; //G-Sync？

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreen_desc = {};
	fullscreen_desc.Windowed = TRUE; //ボーダレスフルスクリーン？
	ComPtr<IDXGISwapChain1> p_swapchain;
	check_hresult(p_factory->CreateSwapChainForHwnd(&queue, hwnd, &desc, &fullscreen_desc, nullptr, &p_swapchain));
	check_hresult(p_swapchain.As<IDXGISwapChain>(&mp_impl));

	//ALT+Enterを処理させない
	p_factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);

	//HDRを有効化
	ComPtr<IDXGIOutput> p_output;
	ComPtr<IDXGIOutput6> p_output6;
	if(SUCCEEDED(mp_impl->GetContainingOutput(&p_output)) && SUCCEEDED(p_output.As(&p_output6)))
	{
		DXGI_OUTPUT_DESC1 desc;
		if(SUCCEEDED(p_output6->GetDesc1(&desc)) && (desc.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020))
		{
			UINT support;
			if(SUCCEEDED(mp_impl->CheckColorSpaceSupport(DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020, &support)) && (support & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT))
			{
				m_hdr_mode = SUCCEEDED(mp_impl->SetColorSpace1(DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020));
			}
		}
	}
	if(not(m_hdr_mode))
		throw;

	//バックバッファを取得
	get_back_buffers();
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//バックバッファのインデックスを返す
inline uint swapchain::back_buffer_index() const
{
	return mp_impl->GetCurrentBackBufferIndex();
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//リサイズ
inline void swapchain::resize(const uint2 size)
{
	DXGI_SWAP_CHAIN_DESC1 desc;
	mp_impl->GetDesc1(&desc);

	for(auto& p_back_buffer : m_back_buffer_ptrs)
		p_back_buffer.reset();

	mp_impl->ResizeBuffers(buffer_count(), size.x, size.y, desc.Format, desc.Flags);
	get_back_buffers();
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//フルスクリーンの切り替え
inline void swapchain::toggle_fullscreen(const bool fullscreen)
{
	mp_impl->SetFullscreenState(fullscreen, nullptr);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//バックバッファを取得
inline void swapchain::get_back_buffers()
{
	for(uint i = 0; i < buffer_count(); i++)
	{
		ComPtr<ID3D12Resource> p_target;
		mp_impl->GetBuffer(i, IID_PPV_ARGS(&p_target));
		m_back_buffer_ptrs[i] = new texture2d(std::move(p_target));
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//descriptor_heap
///////////////////////////////////////////////////////////////////////////////////////////////////

inline descriptor_heap::descriptor_heap(render_device& device, const D3D12_DESCRIPTOR_HEAP_TYPE type, const uint num) : m_size(device->GetDescriptorHandleIncrementSize(type)), m_allocator(num)
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = num;
	desc.Type = type;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	check_hresult(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mp_impl)));
	m_start = mp_impl->GetCPUDescriptorHandleForHeapStart().ptr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//割り当て
inline D3D12_CPU_DESCRIPTOR_HANDLE descriptor_heap::allocate()
{
	D3D12_CPU_DESCRIPTOR_HANDLE ret;
	ret.ptr = m_start + m_size * m_allocator.allocate();
	return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//解放
inline void descriptor_heap::deallocate(const D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
	const size_t pos = (handle.ptr - m_start) / m_size;
	m_allocator.deallocate(pos);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//sv_descriptor_heap
///////////////////////////////////////////////////////////////////////////////////////////////////

//コンストラクタ
inline sv_descriptor_heap::sv_descriptor_heap(render_device& device, const D3D12_DESCRIPTOR_HEAP_TYPE type, const uint num_bind, const uint num_bindless) : m_size(device->GetDescriptorHandleIncrementSize(type)), m_allocator(num_bind), m_allocator_bindless(num_bindless)
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = num_bind + num_bindless;
	desc.Type = type;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	check_hresult(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mp_impl)));
	m_cpu_start_bindless = mp_impl->GetCPUDescriptorHandleForHeapStart().ptr;
	m_gpu_start_bindless = mp_impl->GetGPUDescriptorHandleForHeapStart().ptr;
	m_cpu_start = m_cpu_start_bindless + m_size * num_bindless;
	m_gpu_start = m_gpu_start_bindless + m_size * num_bindless;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//割り当て
inline descriptor_handle sv_descriptor_heap::allocate(const uint num)
{
	descriptor_handle ret;
	const size_t index = m_allocator.allocate(num) * m_size;
	ret.cpu_start.ptr = m_cpu_start + index;
	ret.gpu_start.ptr = m_gpu_start + index;
	return ret;
}

inline sv_descriptor_heap::descriptor_handle_index sv_descriptor_heap::allocate_bindless()
{
	descriptor_handle_index ret;
	const size_t index = m_allocator_bindless.allocate();
	ret.cpu_start.ptr = m_cpu_start_bindless + index * m_size;
	ret.gpu_start.ptr = m_gpu_start_bindless + index * m_size;
	ret.index = uint(index);
	return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//解放
inline void sv_descriptor_heap::deallocate()
{
	m_allocator.dellocate();
}

inline void sv_descriptor_heap::deallocate_bindless(const uint index)
{
	m_allocator_bindless.deallocate(index);
}

inline void sv_descriptor_heap::deallocate_bindless(const D3D12_CPU_DESCRIPTOR_HANDLE cpu_start)
{
	m_allocator_bindless.deallocate((cpu_start.ptr - m_cpu_start) / m_size);
}

inline void sv_descriptor_heap::deallocate_bindless(const D3D12_GPU_DESCRIPTOR_HANDLE gpu_start)
{
	m_allocator_bindless.deallocate((gpu_start.ptr - m_gpu_start) / m_size);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//command_queue
///////////////////////////////////////////////////////////////////////////////////////////////////

//コンストラクタ
inline command_queue::command_queue(render_device& device, const D3D12_COMMAND_LIST_TYPE type)
{
	assert(type != D3D12_COMMAND_LIST_TYPE_NONE);
	assert(type != D3D12_COMMAND_LIST_TYPE_BUNDLE);

	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type = type;
	desc.NodeMask = 1;
	check_hresult(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&mp_impl)));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//command_list
///////////////////////////////////////////////////////////////////////////////////////////////////

//コマンドリスト
inline command_list::command_list(render_device& device, const D3D12_COMMAND_LIST_TYPE type, ID3D12CommandAllocator& allocator)
{
	assert(type != D3D12_COMMAND_LIST_TYPE_NONE);
	assert(type != D3D12_COMMAND_LIST_TYPE_BUNDLE);
	check_hresult(device->CreateCommandList(1, type, &allocator, nullptr, IID_PPV_ARGS(&mp_impl)));
	check_hresult(mp_impl->Close());
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//command_allocator
///////////////////////////////////////////////////////////////////////////////////////////////////

//コンストラクタ
inline command_allocator::command_allocator(render_device& device, const D3D12_COMMAND_LIST_TYPE type)
{
	assert(type != D3D12_COMMAND_LIST_TYPE_NONE);
	assert(type != D3D12_COMMAND_LIST_TYPE_BUNDLE);
	check_hresult(device->CreateCommandAllocator(type, IID_PPV_ARGS(&mp_impl)));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//fence
///////////////////////////////////////////////////////////////////////////////////////////////////

//コンストラクタ
inline fence::fence(render_device& device)
{
	check_hresult(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mp_impl)));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//render_device
///////////////////////////////////////////////////////////////////////////////////////////////////

//コンストラクタ
inline render_device::render_device(const HWND hwnd) : 
	iface::render_device(command_buffer_size), m_hwnd(hwnd),
	m_rtv_descriptor_heap(*this, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, rtv_descriptor_size), 
	m_dsv_descriptor_heap(*this, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, dsv_descriptor_size), 
	m_sampler_descriptor_heap(*this, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, sampler_descriptor_size), 
	m_cbv_srv_uav_descriptor_heap(*this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, cbv_srv_uav_descriptor_size), 
	m_sv_sampler_descriptor_heap(*this, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, sampler_descriptor_size, bindless_sampler_size), 
	m_sv_cbv_srv_uav_descriptor_heap(*this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, cbv_srv_uav_descriptor_size, bindless_cbv_srv_uav_size),
	m_copy_command_allocator(*this, D3D12_COMMAND_LIST_TYPE_COPY), 
	m_copy_command_list(*this, D3D12_COMMAND_LIST_TYPE_COPY, m_copy_command_allocator), 
	m_copy_command_queue(*this, D3D12_COMMAND_LIST_TYPE_COPY), 
	m_copy_fence(*this), 
	m_graphics_command_allocator{command_allocator(*this, D3D12_COMMAND_LIST_TYPE_DIRECT), command_allocator(*this, D3D12_COMMAND_LIST_TYPE_DIRECT)},
	m_graphics_command_list(*this, D3D12_COMMAND_LIST_TYPE_DIRECT, m_graphics_command_allocator[0]), 
	m_graphics_command_queue(*this, D3D12_COMMAND_LIST_TYPE_DIRECT), 
	m_graphics_fence(*this),
	m_upload_buffer(*this, upload_buffer_size, nullptr), 
	m_upload_buffer_allocator(upload_buffer_size / upload_buffer_alignment),
	m_bindless_sampler(bindless_sampler_size),
	m_bindless_cbv_srv_uav(bindless_cbv_srv_uav_size)
{
	auto create_command_signature = [&](D3D12_INDIRECT_ARGUMENT_TYPE type, uint stride)
	{
		D3D12_INDIRECT_ARGUMENT_DESC indirect_argument_desc = {};
		indirect_argument_desc.Type = type;

		D3D12_COMMAND_SIGNATURE_DESC command_signature_desc = {};
		command_signature_desc.NumArgumentDescs = 1;
		command_signature_desc.pArgumentDescs = &indirect_argument_desc;
		command_signature_desc.ByteStride = stride;

		ComPtr<ID3D12CommandSignature> p_cs;
		mp_impl->CreateCommandSignature(&command_signature_desc, nullptr, IID_PPV_ARGS(&p_cs));
		return p_cs;
	};
	mp_draw_indirect_cs = create_command_signature(D3D12_INDIRECT_ARGUMENT_TYPE_DRAW, sizeof(D3D12_DRAW_ARGUMENTS));
	mp_dispatch_indirect_cs = create_command_signature(D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH, sizeof(D3D12_DISPATCH_ARGUMENTS));
	mp_draw_indexed_indirect_cs = create_command_signature(D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED, sizeof(D3D12_DRAW_INDEXED_ARGUMENTS));

	m_copy_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	m_graphics_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	m_graphics_fence_value = m_graphics_fence->GetCompletedValue();
	m_graphics_fence_prev_value = m_graphics_fence_value;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//デストラクタ
inline render_device::~render_device()
{
	CloseHandle(m_copy_event);
	CloseHandle(m_graphics_event);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//実行
inline void render_device::present_impl(render::swapchain& swapchain_base)
{
	auto& swapchain = static_cast<d3d12::swapchain&>(swapchain_base);
	const uint back_buffer_index = swapchain.back_buffer_index();
	m_command_manager.construct<command::present>(swapchain.back_buffer(back_buffer_index));

	m_end_barrier_table.clear();
	m_begin_barrier_table.clear();

	const auto& command_ptrs = m_command_manager.sort_and_get();
	m_begin_barrier_table.resize(command_ptrs.size() + 1);

	//ステートトラッキング
	uint last_blas_build_position = 0;
	uint last_end_barrier_position = 0;
	for(uint i = 1; i <= command_ptrs.size(); i++)
	{
		auto blas_build_barrier = [&]()
		{
			if(last_blas_build_position != 0)
			{
				if(last_end_barrier_position <= last_blas_build_position)
					last_end_barrier_position = i;
				m_end_barrier_table.push_back(m_command_manager.allocate<end_barrier>(nullptr, last_end_barrier_position));
			}
		};
		auto init_tlas_state_info = [&](top_level_acceleration_structure& tlas)
		{
			auto& info = tlas.m_state_infos[0];
			if(info.last_used_frame < frame_count())
			{
				info.last_build_pos = 0;
				info.last_barrier_pos = 0;
				info.last_used_frame = frame_count();
			}
		};
		auto tlas_build_barrier = [&](top_level_acceleration_structure& tlas)
		{
			init_tlas_state_info(tlas);
			auto& info = tlas.m_state_infos[0];
			if(info.last_build_pos > info.last_barrier_pos)
			{
				if(last_end_barrier_position <= info.last_build_pos)
					last_end_barrier_position = i;
				m_end_barrier_table.push_back(m_command_manager.allocate<end_barrier>(&tlas, last_end_barrier_position));
				info.last_barrier_pos = i;
			}
		};
		auto set_tlas_build_pos = [&](top_level_acceleration_structure& tlas)
		{
			init_tlas_state_info(tlas);
			tlas.m_state_infos[0].last_build_pos = i;
		};

		auto state_transition_impl = [&](resource& res, const uint subresource_index, const D3D12_RESOURCE_STATES after, bool need_uav_barrier, const bool is_buffer)
		{
			auto& info = res.m_state_infos[subresource_index];
			if(info.last_used_frame < frame_count())
			{
				info.last_used_pos = 0;
				info.last_used_frame = frame_count();

#if 0 //bufferはExecuteCommandList後に必ずcommonに戻ると書いていたが戻らない？

				//bufferはExecuteCommandList後に必ずcommonに戻る
				if(res.is_buffer())
					info.state = D3D12_RESOURCE_STATE_COMMON;
				//textureは暗黙遷移だけだったときはcommonに戻る
				else 
#endif
				if(info.implicit)
					info.state = D3D12_RESOURCE_STATE_COMMON;

				//明示的遷移があってもcommonに戻っていたら暗黙遷移可能状態で始まる
				info.implicit = (info.state == D3D12_RESOURCE_STATE_COMMON);

				//最初に使う場合はuavバリアは不要
				need_uav_barrier = false;
			}

			if(info.state == after)
			{
				need_uav_barrier = need_uav_barrier && (info.state == D3D12_RESOURCE_STATE_UNORDERED_ACCESS) && (after == D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				if(need_uav_barrier)
				{
					//uavバリア
					if(last_end_barrier_position <= info.last_used_pos)
						last_end_barrier_position = i;
					auto* p_uav_barrier = m_command_manager.allocate<end_barrier>(&res, last_end_barrier_position);
					m_end_barrier_table.push_back(p_uav_barrier);
				}
				info.last_used_pos = i;
				return need_uav_barrier;
			}
				
			if(info.implicit)
			{
				bool common_to_after = (info.state == D3D12_RESOURCE_STATE_COMMON);
				if(common_to_after)
				{
					//bufferはcommonからは任意の状態に暗黙で遷移可能
					if(res.is_buffer())
						common_to_after = true;
					//textureはnon-pixel,pixel,copy_dest,copy_srcにのみ暗黙に遷移可能
					else
						common_to_after = (after & D3D12_RESOURCE_STATE_GENERIC_READ);
				}

				if(common_to_after)
					info.state = after;
				//read->readは暗黙遷移を維持
				else if((info.state & D3D12_RESOURCE_STATE_GENERIC_READ) && (after & D3D12_RESOURCE_STATE_GENERIC_READ))
					info.state = D3D12_RESOURCE_STATES(info.state | after);
				//read->write,write->readは明示的な遷移
				else
					info.implicit = false;
			}
			if(not(info.implicit))
			{
				const auto before = info.state;
				info.state = after;

				if(last_end_barrier_position <= info.last_used_pos)
					last_end_barrier_position = i;

				//beginバリア(i==1のときと,beginとendが同一タイミングのときは通常のバリア)
				const bool end_only = not((i == 1) || (last_end_barrier_position == info.last_used_pos + 1)); 
				if(end_only)
				{
					auto& p_front = m_begin_barrier_table[info.last_used_pos + 1];
					auto* p_begin_barrier = m_command_manager.allocate<begin_barrier>(res, subresource_index, before, after, p_front);
					p_front = p_begin_barrier;
				}

				//endバリア
				auto* p_end_barrier = m_command_manager.allocate<end_barrier>(res, subresource_index, before, after, last_end_barrier_position, not(end_only));
				m_end_barrier_table.push_back(p_end_barrier);
			}
			info.last_used_pos = i;
			return false;
		};

		auto state_transition = [&](resource& res, uint mip_slice, uint mip_count, uint array_slice, uint array_count, DXGI_FORMAT format, D3D12_RESOURCE_STATES after, bool need_uav_barrier)
		{
			//auto get_name = [&](auto& res)
			//{
			//	wchar_t name[128];
			//	uint size = sizeof(name);
			//	res->GetPrivateData(WKPDID_D3DDebugObjectNameW, &size, name);
			//	return utf16_to_utf8(wstring(name, size / 2)); //÷2が正しい対応ではないと思うけど...
			//};
			//std::cout << get_name(res) << std::endl;

			const auto& desc = res->GetDesc();
			const auto& infos = res.m_state_infos;

			uint plane_slice = 0;
			uint plane_count = 1;
			switch(format)
			{
			case DXGI_FORMAT_D24_UNORM_S8_UINT:
			case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
				plane_count = 2;
				break;
			case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
			case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
				break;
			case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
			case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
				plane_slice = 1;
				break;
			}

			//TODO: 全サブリソースまとめてバリア

			for(uint i = 0; i < plane_count; i++)
			{
				for(uint j = 0; j < array_count; j++)
				{
					for(uint k = 0; k < mip_count; k++)
					{
						//uavバリアはサブリソース指定できないので発行は1回で十分
						if(state_transition_impl(res, subresource_index(res, mip_slice + k, array_slice + j, plane_slice + i), after, need_uav_barrier, res.is_buffer()))
							need_uav_barrier = false;
					}
				}
			}
		};

		const auto& command_base = *command_ptrs[i - 1];
		switch(command_base.type)
		{
		case command::type_clear_depth_stencil_view:
		{
			auto& command = static_cast<const command::clear_depth_stencil_view&>(command_base);
			auto& view = *static_cast<d3d12::depth_stencil_view*>(command.p_dsv);
			auto& desc = view.desc();
			state_transition(view.resource(), desc.mip_slice, 1, desc.first_array_index, desc.array_size, view.format(), D3D12_RESOURCE_STATE_DEPTH_WRITE, false);
			break;
		}
		case command::type_clear_render_target_view:
		{
			auto& command = static_cast<const command::clear_render_target_view&>(command_base);
			auto& view = *static_cast<d3d12::render_target_view*>(command.p_rtv);
			auto& desc = view.desc();
			state_transition(view.resource(), desc.mip_slice, 1, desc.first_array_index, desc.array_size, view.format(), D3D12_RESOURCE_STATE_RENDER_TARGET, false);
			break;
		}
		case command::type_clear_unordered_access_view_float:
		{
			auto& command = static_cast<const command::clear_unordered_access_view_float&>(command_base);
			auto& view = *static_cast<d3d12::unordered_access_view*>(command.p_uav);
			if(view.resource().is_buffer())
			{
				auto& desc = static_cast<d3d12::buffer_unordered_access_view&>(view).desc();
				state_transition(view.resource(), 0, 1, 0, 1, DXGI_FORMAT_UNKNOWN, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, command.uav_barrier);
			}
			else
			{
				auto& desc = static_cast<d3d12::texture_unordered_access_view&>(view).desc();
				state_transition(view.resource(), desc.mip_slice, 1, desc.first_array_index, desc.array_size, DXGI_FORMAT_UNKNOWN, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, command.uav_barrier);
			}
			break;
		}
		case command::type_clear_unordered_access_view_uint:
		{
			auto& command = static_cast<const command::clear_unordered_access_view_uint&>(command_base);
			auto& view = *static_cast<d3d12::unordered_access_view*>(command.p_uav);
			if(view.resource().is_buffer())
			{
				auto& desc = static_cast<d3d12::buffer_unordered_access_view&>(view).desc();
				state_transition(view.resource(), 0, 1, 0, 1, DXGI_FORMAT_UNKNOWN, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, command.uav_barrier);
			}
			else
			{
				auto& desc = static_cast<d3d12::texture_unordered_access_view&>(view).desc();
				state_transition(view.resource(), desc.mip_slice, 1, desc.first_array_index, desc.array_size, DXGI_FORMAT_UNKNOWN, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, command.uav_barrier);
			}
			break;
		}
		case command::type_draw:
		case command::type_draw_indexed:
		case command::type_draw_indirect:
		case command::type_draw_indexed_indirect:
		case command::type_dispatch:
		case command::type_dispatch_indirect:
		{
			bool uav_barrier;
			buffer* p_buffer = nullptr;
			bind_info *p_bind_info = nullptr;
			const target_state* p_target_state = nullptr;
			switch(command_base.type)
			{
			case command::type_draw:
				uav_barrier = static_cast<const command::draw&>(command_base).uav_barrier;
				p_bind_info = static_cast<bind_info*>(static_cast<const command::draw&>(command_base).p_optional);
				p_target_state = static_cast<const d3d12::target_state*>(static_cast<const command::draw&>(command_base).p_target_state);
				break;
			case command::type_draw_indexed:
				uav_barrier = static_cast<const command::draw_indexed&>(command_base).uav_barrier;
				p_bind_info = static_cast<bind_info*>(static_cast<const command::draw_indexed&>(command_base).p_optional);
				p_target_state = static_cast<const d3d12::target_state*>(static_cast<const command::draw_indexed&>(command_base).p_target_state);
				break;
			case command::type_draw_indirect:
				uav_barrier = static_cast<const command::draw_indirect&>(command_base).uav_barrier;
				p_bind_info = static_cast<bind_info*>(static_cast<const command::draw_indirect&>(command_base).p_optional);
				p_buffer = static_cast<d3d12::buffer*>(static_cast<const command::draw_indirect&>(command_base).p_buf);
				p_target_state = static_cast<const d3d12::target_state*>(static_cast<const command::draw_indirect&>(command_base).p_target_state);
				break;
			case command::type_draw_indexed_indirect:
				uav_barrier = static_cast<const command::draw_indexed_indirect&>(command_base).uav_barrier;
				p_bind_info = static_cast<bind_info*>(static_cast<const command::draw_indexed_indirect&>(command_base).p_optional);
				p_buffer = static_cast<d3d12::buffer*>(static_cast<const command::draw_indexed_indirect&>(command_base).p_buf);
				p_target_state = static_cast<const d3d12::target_state*>(static_cast<const command::draw_indexed_indirect&>(command_base).p_target_state);
				break;
			case command::type_dispatch:
				uav_barrier = static_cast<const command::dispatch&>(command_base).uav_barrier;
				p_bind_info = static_cast<bind_info*>(static_cast<const command::dispatch&>(command_base).p_optional);
				break;
			case command::type_dispatch_indirect:
				uav_barrier = static_cast<const command::dispatch_indirect&>(command_base).uav_barrier;
				p_bind_info = static_cast<bind_info*>(static_cast<const command::dispatch_indirect&>(command_base).p_optional);
				p_buffer = static_cast<d3d12::buffer*>(static_cast<const command::dispatch_indirect&>(command_base).p_buf);
				break;
			}

			if(p_target_state)
			{
				for(uint i = 0; i < p_target_state->num_rtvs(); i++)
				{
					auto& rtv = p_target_state->rtv(i);
					state_transition(rtv.resource(), rtv.desc().mip_slice, 1, rtv.desc().first_array_index, 1, rtv.format(), D3D12_RESOURCE_STATE_RENDER_TARGET, uav_barrier);
				}
				if(p_target_state->has_dsv())
				{
					auto& dsv = p_target_state->dsv();
					auto after_state = D3D12_RESOURCE_STATE_DEPTH_WRITE;
					if((dsv.desc().flags == dsv_flag_readonly_depth) || (dsv.desc().flags == dsv_flag_readonly_stencil))
						after_state = D3D12_RESOURCE_STATE_DEPTH_READ;
					state_transition(dsv.resource(), dsv.desc().mip_slice, 1, dsv.desc().first_array_index, 1, dsv.format(), after_state, uav_barrier);
				}
			}

			if(p_buffer != nullptr)
				state_transition(*p_buffer, 0, 1, 0, 1, DXGI_FORMAT_UNKNOWN, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, uav_barrier);

			if(p_bind_info->root_tlas_srv_index != uint8_t(-1))
				tlas_build_barrier(*p_bind_info->p_tlas);

			for(uint i = 0; i < p_bind_info->num_views; i++)
			{
				if(p_bind_info->views[i].is_srv())
				{
					D3D12_RESOURCE_STATES after_state = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
					if(p_bind_info->views[i].for_ps())
						after_state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
					
					auto& view = p_bind_info->views[i].get<shader_resource_view>();
					if(view.resource().is_buffer())
					{
						auto& desc = static_cast<const d3d12::buffer_shader_resource_view&>(view).desc();
						state_transition(view.resource(), 0, 1, 0, 1, DXGI_FORMAT_UNKNOWN, after_state, uav_barrier);
					}
					else
					{
						auto& view = p_bind_info->views[i].get<texture_shader_resource_view>();
						auto& desc = p_bind_info->views[i].get<texture_shader_resource_view>().desc();
						state_transition(view.resource(), desc.most_detailed_mip, desc.mip_levels, desc.first_array_index, desc.array_size, view.format(), after_state, uav_barrier);
					}
				}
				else
				{
					auto& view = p_bind_info->views[i].get<unordered_access_view>();
					if(view.resource().is_buffer())
					{
						auto& desc = p_bind_info->views[i].get<buffer_unordered_access_view>().desc();
						state_transition(view.resource(), 0, 1, 0, 1, DXGI_FORMAT_UNKNOWN, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, uav_barrier);
					}
					else
					{
						auto& view = p_bind_info->views[i].get<texture_unordered_access_view>();
						auto& desc = p_bind_info->views[i].get<texture_unordered_access_view>().desc();
						state_transition(view.resource(), desc.mip_slice, 1, desc.first_array_index, desc.array_size, DXGI_FORMAT_UNKNOWN, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, uav_barrier);
					}
				}
			}
			break;
		}
		case command::type_build_top_level_acceleration_structure:
		{
			//BLASビルド完了を待つ必要がある
			blas_build_barrier();

			//InstanceDescsはupdate_bufferでCopyDestになるのでNonPixelShaderResourceにする必要がある
			auto& command = static_cast<const command::build_top_level_acceleration_structure&>(command_base);
			state_transition(static_cast<d3d12::buffer&>(command.p_tlas->instance_descs()), 0, 1, 0, 1, DXGI_FORMAT_UNKNOWN, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, false);
			set_tlas_build_pos(static_cast<d3d12::top_level_acceleration_structure&>(*command.p_tlas));
			break;
		}
		case command::type_build_bottom_level_acceleration_structure:
		{
			auto& command = static_cast<const command::build_bottom_level_acceleration_structure&>(command_base);
			auto& blas = *static_cast<d3d12::bottom_level_acceleration_structure*>(command.p_blas);
			last_blas_build_position = i;
			break;
		}
		case command::type_refit_bottom_level_acceleration_structure:
		{
			last_blas_build_position = i;
			break;
		}
		case command::type_compact_bottom_level_acceleration_structure:
		{
			auto& command = static_cast<const command::compact_bottom_level_acceleration_structure&>(command_base);
			auto& blas = *static_cast<d3d12::bottom_level_acceleration_structure*>(command.p_blas);
			switch(blas.compaction_state())
			{
			case raytracing_compaction_state_ready: //readbackへコピー
				state_transition(blas.compacted_size(), 0, 1, 0, 1, DXGI_FORMAT_UNKNOWN, D3D12_RESOURCE_STATE_COPY_SOURCE, false);
				break;
			case raytracing_compaction_state_progress0: //readbackへのコピー完了待ち
				break;
			case raytracing_compaction_state_progress1: //コンパクション実行
				break;
			case raytracing_compaction_state_progress2: //コンパクション結果に置き換え
				break;
			default:
				assert(0);
			}
			break;
		}
		case command::type_update_buffer:
		{
			auto& command = static_cast<const command::update_buffer&>(command_base);
			state_transition(*static_cast<d3d12::buffer*>(command.p_buf), 0, 1, 0, 1, DXGI_FORMAT_UNKNOWN, D3D12_RESOURCE_STATE_COPY_DEST, false);
			break;
		}
		case command::type_copy_buffer:
		{
			auto& command = static_cast<const command::copy_buffer&>(command_base);
			if(!command.dst_is_readback)
				state_transition(*static_cast<d3d12::buffer*>(command.p_dst), 0, 1, 0, 1, DXGI_FORMAT_UNKNOWN, D3D12_RESOURCE_STATE_COPY_DEST, false);
			if(!command.src_is_upload)
				state_transition(*static_cast<d3d12::buffer*>(command.p_src), 0, 1, 0, 1, DXGI_FORMAT_UNKNOWN, D3D12_RESOURCE_STATE_COPY_SOURCE, false);
			break;
		}
		case command::type_copy_texture:
		{
			auto& command = static_cast<const command::copy_texture&>(command_base);
			auto p_dst = static_cast<d3d12::texture*>(command.p_dst);
			auto p_src = static_cast<d3d12::texture*>(command.p_src);
			state_transition(*p_dst, 0, p_dst->mip_levels(), 0, p_dst->array_size(), convert((*p_dst)->GetDesc().Format), D3D12_RESOURCE_STATE_COPY_DEST, false);
			state_transition(*p_src, 0, p_src->mip_levels(), 0, p_src->array_size(), convert((*p_src)->GetDesc().Format), D3D12_RESOURCE_STATE_COPY_SOURCE, false);
			break;
		}
		case command::type_copy_resource:
		{
			//TODO: テクスチャ対応
			auto& command = static_cast<const command::copy_resource&>(command_base);
			state_transition(*static_cast<d3d12::resource*const>(command.p_dst), 0, 1, 0, 1, DXGI_FORMAT_UNKNOWN, D3D12_RESOURCE_STATE_COPY_DEST, false);
			state_transition(*static_cast<d3d12::resource*const>(command.p_src), 0, 1, 0, 1, DXGI_FORMAT_UNKNOWN, D3D12_RESOURCE_STATE_COPY_SOURCE, false);
			break;
		}
		case command::type_present:
		{
			auto& command = static_cast<const command::present&>(command_base);
			state_transition(*static_cast<d3d12::texture*>(command.p_tex), 0, 1, 0, 1, DXGI_FORMAT_UNKNOWN, D3D12_RESOURCE_STATE_PRESENT, false);
			break;
		}
		case command::type_call_function:
		{
			auto& command = static_cast<const command::call_function&>(command_base);
			auto* p_target_state = static_cast<const d3d12::target_state*>(command.p_target_state);
			if(p_target_state)
			{
				for(uint i = 0; i < p_target_state->num_rtvs(); i++)
				{
					auto& rtv = p_target_state->rtv(i);
					state_transition(rtv.resource(), rtv.desc().mip_slice, 1, rtv.desc().first_array_index, 1, rtv.format(), D3D12_RESOURCE_STATE_RENDER_TARGET, true);
				}
				if(p_target_state->has_dsv())
				{
					auto& dsv = p_target_state->dsv();
					auto after_state = D3D12_RESOURCE_STATE_DEPTH_WRITE;
					if((dsv.desc().flags == dsv_flag_readonly_depth) || (dsv.desc().flags == dsv_flag_readonly_stencil))
						after_state = D3D12_RESOURCE_STATE_DEPTH_READ;
					state_transition(dsv.resource(), dsv.desc().mip_slice, 1, dsv.desc().first_array_index, 1, dsv.format(), after_state, true);
				}
			}
			break;
		}
		default:
			assert(0);
		}
	}

	m_graphics_command_allocator[frame_count() % 2]->Reset();
	m_graphics_command_list->Reset(m_graphics_command_allocator[frame_count() % 2].get_impl(), nullptr);

	ID3D12DescriptorHeap *heaps[] = 
	{
		m_sv_cbv_srv_uav_descriptor_heap.get_impl(), 
		m_sv_sampler_descriptor_heap.get_impl()
	};
	m_graphics_command_list->SetDescriptorHeaps(2, heaps);
	
	float blend_factor[4] = { 1, 1, 1, 1 };
	m_graphics_command_list->OMSetBlendFactor(blend_factor);
	m_graphics_command_list->OMSetDepthBounds(0, 1);

	//命令作成
	for(uint i = 1, j = 0; i <= command_ptrs.size(); i++)
	{
		auto get_name = [&](auto& res)
		{
			wchar_t name[128];
			uint size = sizeof(name);
			res->GetPrivateData(WKPDID_D3DDebugObjectNameW, &size, name);
			return utf16_to_utf8(wstring(name, size / 2)); //÷2が正しい対応ではないと思うけど...
		};

		//バリア
		uint num_barriers = 0;
		D3D12_RESOURCE_BARRIER barriers[128];
		{
			//beginバリア
			auto* p_barrier = m_begin_barrier_table[i];
			for(; p_barrier != nullptr; p_barrier = p_barrier->p_next)
			{
				barriers[num_barriers].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				barriers[num_barriers].Transition.pResource = p_barrier->p_resource->get_impl();
				barriers[num_barriers].Transition.StateAfter = p_barrier->after;
				barriers[num_barriers].Transition.StateBefore = p_barrier->before;
				barriers[num_barriers].Transition.Subresource = p_barrier->subresource;
				barriers[num_barriers].Flags = D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY;

//#define RESORUCE_BARRIER_DEBUG
#ifdef RESORUCE_BARRIER_DEBUG
				std::cout << "[" << i << "] begin " << get_name(*p_barrier->p_resource) << "(" << p_barrier->subresource << "): " << state_string(p_barrier->before) << " -> " << state_string(p_barrier->after) << std::endl;
#endif

				if(++num_barriers >= _countof(barriers))
				{
					m_graphics_command_list->ResourceBarrier(num_barriers, barriers);
					num_barriers = 0;
				}
			}
		}
		for(; (j < m_end_barrier_table.size()) && (m_end_barrier_table[j]->time <= i); j++)
		{
			//endバリア(i==1のときは通常のバリア)
			auto& barrier = *m_end_barrier_table[j];
			if(barrier.before != barrier.after)
			{
				barriers[num_barriers].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				barriers[num_barriers].Transition.pResource = barrier.p_resource->get_impl();
				barriers[num_barriers].Transition.StateAfter = barrier.after;
				barriers[num_barriers].Transition.StateBefore = barrier.before;
				barriers[num_barriers].Transition.Subresource = barrier.subresource;
				barriers[num_barriers].Flags = barrier.not_end ? D3D12_RESOURCE_BARRIER_FLAG_NONE : D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;

#ifdef RESORUCE_BARRIER_DEBUG
				if(barrier.not_end)
					std::cout << "[" << i << "] default " << get_name(*barrier.p_resource) << "(" << barrier.subresource << "): " << state_string(barrier.before) << " -> " << state_string(barrier.after) << std::endl;
				else
					std::cout << "[" << i << "] end " << get_name(*barrier.p_resource) << "(" << barrier.subresource << "): " << state_string(barrier.before) << " -> " << state_string(barrier.after) << std::endl;
#endif
			}
			//uavバリア
			else
			{
				barriers[num_barriers].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
				barriers[num_barriers].UAV.pResource = barrier.p_resource ? barrier.p_resource->get_impl() : nullptr;
				barriers[num_barriers].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

#ifdef RESORUCE_BARRIER_DEBUG
				std::cout << "[" << i << "] uav " << (barrier.p_resource ? get_name(*barrier.p_resource) : "global") <<  std::endl;
#endif
			}
			
			if(++num_barriers >= _countof(barriers))
			{
				m_graphics_command_list->ResourceBarrier(num_barriers, barriers);
				num_barriers = 0;
			}
		}

		//std::cout << "[" << i << "] " << command_ptrs[i - 1]->type << std::endl;

		struct pix_scoped_event{
			pix_scoped_event(const char *event_name, ID3D12GraphicsCommandList *p_command_list, const uint num_barriers, const D3D12_RESOURCE_BARRIER *barriers) : p_command_list(p_command_list){
				PIXBeginEvent(p_command_list, 0, event_name); if(num_barriers){ p_command_list->ResourceBarrier(num_barriers, barriers); }
			}
			~pix_scoped_event(){
				PIXEndEvent(p_command_list);
			}
			ID3D12GraphicsCommandList *p_command_list;
		};
#define PIX_SCOPED_EVENT(name) pix_scoped_event pix_scoped_event(name, m_graphics_command_list.get_impl(), num_barriers, barriers);

		const auto& command_base = *command_ptrs[i - 1];
		switch(command_base.type)
		{
		case command::type_clear_depth_stencil_view:
		{
			PIX_SCOPED_EVENT("clear_depth_stencil_view");
			auto& command = static_cast<const command::clear_depth_stencil_view&>(command_base);
			auto& view = static_cast<const d3d12::depth_stencil_view&>(*command.p_dsv);
			m_graphics_command_list->ClearDepthStencilView(view, D3D12_CLEAR_FLAGS(command.flags), command.depth, command.stencil, 0, nullptr);
			break;
		}
		case command::type_clear_render_target_view:
		{
			PIX_SCOPED_EVENT("clear_render_target_view");
			auto& command = static_cast<const command::clear_render_target_view&>(command_base);
			auto& view = static_cast<const d3d12::render_target_view&>(*command.p_rtv);
			m_graphics_command_list->ClearRenderTargetView(view, command.color, 0, nullptr);
			break;
		}
		case command::type_clear_unordered_access_view_float:
		{
			PIX_SCOPED_EVENT("clear_unordered_access_view_float");
			auto& command = static_cast<const command::clear_unordered_access_view_float&>(command_base);
			auto& view = static_cast<const d3d12::unordered_access_view&>(*command.p_uav);
			auto handle = m_sv_cbv_srv_uav_descriptor_heap.allocate(1);
			mp_impl->CopyDescriptorsSimple(1, handle.cpu_start, view, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			m_graphics_command_list->ClearUnorderedAccessViewFloat(handle.gpu_start, view, view.resource().get_impl(), command.value, 0, nullptr);
			break;
		}
		case command::type_clear_unordered_access_view_uint:
		{
			PIX_SCOPED_EVENT("clear_unordered_access_view_uint");
			auto& command = static_cast<const command::clear_unordered_access_view_uint&>(command_base);
			auto& view = static_cast<const d3d12::unordered_access_view&>(*command.p_uav);
			auto handle = m_sv_cbv_srv_uav_descriptor_heap.allocate(1);
			mp_impl->CopyDescriptorsSimple(1, handle.cpu_start, view, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			m_graphics_command_list->ClearUnorderedAccessViewUint(handle.gpu_start, view, view.resource().get_impl(), command.value, 0, nullptr);
			break;
		}
		case command::type_draw:
		case command::type_draw_indirect:
		case command::type_draw_indexed:
		case command::type_draw_indexed_indirect:
		{
			uint stencil;
			uint constant;
			bind_info *p_bind_info; 
			const d3d12::target_state* p_target_state;
			const d3d12::geometry_state* p_geometry_state;
			const d3d12::graphics_pipeline_state* p_pipeline_state;

			auto init = [&](auto& command)
			{
				stencil = command.stencil;
				constant = command.constant;
				p_bind_info = static_cast<render_device::bind_info*>(command.p_optional);
				p_target_state = static_cast<const d3d12::target_state*>(command.p_target_state);
				p_geometry_state = static_cast<const d3d12::geometry_state*>(command.p_geometry_state);
				p_pipeline_state = static_cast<const d3d12::graphics_pipeline_state*>(command.p_pipeline_state);
			};
			switch(command_base.type)
			{
			case command::type_draw: init(static_cast<const command::draw&>(command_base)); break;
			case command::type_draw_indirect: init(static_cast<const command::draw_indirect&>(command_base)); break;
			case command::type_draw_indexed: init(static_cast<const command::draw_indexed&>(command_base)); break;
			case command::type_draw_indexed_indirect: init(static_cast<const command::draw_indexed_indirect&>(command_base)); break;
			}

#ifdef RESORUCE_BARRIER_DEBUG
			std::cout << "[" << i << "] " << p_pipeline_state->name() << std::endl;
#endif
			auto& record = p_pipeline_state->get(*this, *p_target_state, *p_geometry_state);
			auto resource_num = [&](const uint i){ auto& count = record.pipeline_resource_count.shader_resource_count_table[i]; return count.cbv + count.srv + count.uav + count.sampler; };

			m_graphics_command_list->SetGraphicsRootSignature(record.p_root_signature.Get());
			if(p_bind_info->root_constant_index != uint8_t(-1))
				m_graphics_command_list->SetGraphicsRoot32BitConstant(p_bind_info->root_constant_index, constant, 0);
			if(p_bind_info->root_tlas_srv_index != uint8_t(-1))
				m_graphics_command_list->SetGraphicsRootShaderResourceView(p_bind_info->root_tlas_srv_index, (*p_bind_info->p_tlas)->GetGPUVirtualAddress());

			uint slot = 0;
			auto set = [&](const uint shader_index)
			{
				const auto& count = record.pipeline_resource_count.shader_resource_count_table[shader_index];
				if(count.cbv + count.srv + count.uav > 0)
					m_graphics_command_list->SetGraphicsRootDescriptorTable(slot++, p_bind_info->cbv_srv_uav_heap_start);
				if(count.sampler > 0)
					m_graphics_command_list->SetGraphicsRootDescriptorTable(slot++, p_bind_info->sampler_heap_start);
			};
			if(p_pipeline_state->p_vs()){ set(0); }
			if(p_pipeline_state->p_ps()){ set(1); }

			if(p_geometry_state != nullptr)
			{
				if(p_geometry_state->has_index_buffer())
				{
					D3D12_INDEX_BUFFER_VIEW view = {};
					auto& index_buffer = p_geometry_state->index_buffer();
					view.Format = (index_buffer.stride() == 2) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
					view.SizeInBytes = index_buffer.size();
					view.BufferLocation = index_buffer->GetGPUVirtualAddress();
					m_graphics_command_list->IASetIndexBuffer(&view);
				}
				D3D12_VERTEX_BUFFER_VIEW vb_views[8];
				for(uint i = 0; i < p_geometry_state->num_vbs(); i++)
				{
					auto& vertex_buffer = p_geometry_state->vertex_buffer(i);
					vb_views[i].SizeInBytes = vertex_buffer.size() - p_geometry_state->offset(i);
					vb_views[i].StrideInBytes = p_geometry_state->stride(i);
					vb_views[i].BufferLocation = vertex_buffer->GetGPUVirtualAddress() + p_geometry_state->offset(i);
				}
				m_graphics_command_list->IASetVertexBuffers(0, p_geometry_state->num_vbs(), vb_views);
			}
			m_graphics_command_list->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY(p_pipeline_state->input_layout().topology));

			D3D12_RECT scissor[8];
			D3D12_VIEWPORT viewport[8];
			D3D12_CPU_DESCRIPTOR_HANDLE rt_descriptors[8];
			for(uint i = 0; i < p_target_state->num_rtvs(); i++)
			{
				rt_descriptors[i] = p_target_state->rtv(i);
				
				const auto& texture = static_cast<d3d12::texture&>(p_target_state->rtv(i).resource());
				scissor[i].left = scissor[i].top = 0;
				scissor[i].right = texture.width();
				scissor[i].bottom = texture.height();
				viewport[i].TopLeftX = viewport[i].TopLeftY = 0;
				viewport[i].Width = float(texture.width());
				viewport[i].Height = float(texture.height());
				viewport[i].MinDepth = 0;
				viewport[i].MaxDepth = 1;
			}
			m_graphics_command_list->RSSetViewports(p_target_state->num_rtvs(), viewport);
			m_graphics_command_list->RSSetScissorRects(p_target_state->num_rtvs(), scissor);
			m_graphics_command_list->OMSetRenderTargets(p_target_state->num_rtvs(), rt_descriptors, FALSE, p_target_state->has_dsv() ? p_target_state->dsv() : nullptr);
			m_graphics_command_list->OMSetStencilRef(stencil);

			m_graphics_command_list->SetPipelineState(record.p_pipeline_state.Get());
			switch(command_base.type)
			{
			case command::type_draw:
			{
				auto& command = static_cast<const command::draw&>(command_base);
				PIX_SCOPED_EVENT(command.p_pipeline_state->name().c_str());
				m_graphics_command_list->DrawInstanced(command.vertex_count, command.instance_count, command.start_vertex_location, 0);
				break;
			}
			case command::type_draw_indexed:
			{
				auto& command = static_cast<const command::draw_indexed&>(command_base);
				PIX_SCOPED_EVENT(command.p_pipeline_state->name().c_str());
				m_graphics_command_list->DrawIndexedInstanced(command.index_count, command.instance_count, command.start_index_location, command.base_vertex_location, 0);
				break;
			}
			case command::type_draw_indirect:
			{
				auto& command = static_cast<const command::draw_indirect&>(command_base);
				PIX_SCOPED_EVENT(command.p_pipeline_state->name().c_str());
				m_graphics_command_list->ExecuteIndirect(mp_draw_indirect_cs.Get(), 1, static_cast<d3d12::buffer*>(command.p_buf)->get_impl(), command.offset, nullptr, 0);
				break;
			}
			case command::type_draw_indexed_indirect:
			{
				auto& command = static_cast<const command::draw_indexed_indirect&>(command_base);
				PIX_SCOPED_EVENT(command.p_pipeline_state->name().c_str());
				m_graphics_command_list->ExecuteIndirect(mp_draw_indexed_indirect_cs.Get(), 1, static_cast<d3d12::buffer*>(command.p_buf)->get_impl(), command.offset, nullptr, 0);
				break;
			}}
			break;
		}
		case command::type_dispatch:
		{
			auto& command = static_cast<const command::dispatch&>(command_base);
			auto& pipeline_state = static_cast<const d3d12::compute_pipeline_state&>(*command.p_pipeline_state);
			auto* p_bind_info = static_cast<render_device::bind_info*>(command.p_optional);
			auto& resource_count = pipeline_state.resource_count();

#ifdef RESORUCE_BARRIER_DEBUG
			std::cout << "[" << i << "] " << pipeline_state.name() << std::endl;
#endif
			PIX_SCOPED_EVENT(pipeline_state.name().c_str());
			m_graphics_command_list->SetComputeRootSignature(pipeline_state.p_rs().Get());
			if(p_bind_info->root_constant_index != uint8_t(-1))
				m_graphics_command_list->SetComputeRoot32BitConstant(p_bind_info->root_constant_index, command.constant, 0);
			if(p_bind_info->root_tlas_srv_index != uint8_t(-1))
				m_graphics_command_list->SetComputeRootShaderResourceView(p_bind_info->root_tlas_srv_index, (*p_bind_info->p_tlas)->GetGPUVirtualAddress());

			uint slot = 0;
			if(resource_count.cbv + resource_count.srv + resource_count.uav > 0)
				m_graphics_command_list->SetComputeRootDescriptorTable(slot++, p_bind_info->cbv_srv_uav_heap_start);
			if(resource_count.sampler > 0)
				m_graphics_command_list->SetComputeRootDescriptorTable(slot++, p_bind_info->sampler_heap_start);

			m_graphics_command_list->SetPipelineState(pipeline_state.get_impl());
			m_graphics_command_list->Dispatch(command.x, command.y, command.z);
			break;
		}
		case command::type_dispatch_indirect:
		{
			auto& command = static_cast<const command::dispatch_indirect&>(command_base);
			auto& pipeline_state = static_cast<const d3d12::compute_pipeline_state&>(*command.p_pipeline_state);
			auto* p_bind_info = static_cast<render_device::bind_info*>(command.p_optional);
			auto& resource_count = pipeline_state.resource_count();

			PIX_SCOPED_EVENT(pipeline_state.name().c_str());
			m_graphics_command_list->SetComputeRootSignature(pipeline_state.p_rs().Get());
			if(p_bind_info->root_constant_index != uint8_t(-1))
				m_graphics_command_list->SetComputeRoot32BitConstant(p_bind_info->root_constant_index, command.constant, 0);
			if(p_bind_info->root_tlas_srv_index != uint8_t(-1))
				m_graphics_command_list->SetComputeRootShaderResourceView(p_bind_info->root_tlas_srv_index, (*p_bind_info->p_tlas)->GetGPUVirtualAddress());

			uint slot = 0;
			if(resource_count.cbv + resource_count.srv + resource_count.uav > 0)
				m_graphics_command_list->SetComputeRootDescriptorTable(slot++, p_bind_info->cbv_srv_uav_heap_start);
			if(resource_count.sampler > 0)
				m_graphics_command_list->SetComputeRootDescriptorTable(slot++, p_bind_info->sampler_heap_start);

			m_graphics_command_list->SetPipelineState(pipeline_state.get_impl());
			m_graphics_command_list->ExecuteIndirect(mp_dispatch_indirect_cs.Get(), 1, static_cast<d3d12::buffer*>(command.p_buf)->get_impl(), command.offset, nullptr, 0);
			break;
		}
		case command::type_build_top_level_acceleration_structure:
		{
			PIX_SCOPED_EVENT("build_top_level_acceleration_structure");
			auto& command = static_cast<const command::build_top_level_acceleration_structure&>(command_base);
			auto& tlas = *static_cast<d3d12::top_level_acceleration_structure*>(command.p_tlas);

			D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC desc = {};
			D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = tlas.build_inputs(command.num_instances);
			desc.Inputs = inputs;
			desc.DestAccelerationStructureData = tlas->GetGPUVirtualAddress();
			desc.ScratchAccelerationStructureData = tlas.scratch()->GetGPUVirtualAddress();

			m_graphics_command_list->BuildRaytracingAccelerationStructure(&desc, 0, nullptr);

			//後でTLAS使うタイミングでUAVバリアが必要
			break;
		}
		case command::type_build_bottom_level_acceleration_structure:
		case command::type_refit_bottom_level_acceleration_structure:
		{
			auto& blas = *static_cast<d3d12::bottom_level_acceleration_structure*>((command_base.type == command::type_build_bottom_level_acceleration_structure) ? 
				static_cast<const command::build_bottom_level_acceleration_structure&>(command_base).p_blas :
				static_cast<const command::refit_bottom_level_acceleration_structure&>(command_base).p_blas);
			auto* p_gs = static_cast<const d3d12::geometry_state*>((command_base.type == command::type_build_bottom_level_acceleration_structure) ? 
				static_cast<const command::build_bottom_level_acceleration_structure&>(command_base).p_gs :
				static_cast<const command::refit_bottom_level_acceleration_structure&>(command_base).p_gs);
			
			if(p_gs != nullptr)
				blas.update_geometry(*p_gs);

			D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC desc = {};
			D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = blas.build_inputs();
			desc.Inputs = inputs;
			desc.DestAccelerationStructureData = blas->GetGPUVirtualAddress();
			desc.ScratchAccelerationStructureData = blas.scratch()->GetGPUVirtualAddress();

			D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC info = {};
			D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC* p_info = nullptr;
			if(blas.compaction_state() == raytracing_compaction_state_not_ready)
			{
				blas.set_compaction_state(*this, raytracing_compaction_state_ready);
				info.InfoType = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE;
				info.DestBuffer = blas.compacted_size()->GetGPUVirtualAddress();
				p_info = &info;
			}

			if(command_base.type == command::type_refit_bottom_level_acceleration_structure)
			{
				desc.Inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
				desc.SourceAccelerationStructureData = blas->GetGPUVirtualAddress();
			}

			if(blas.compaction_state() != raytracing_compaction_state_not_support) //スタティックはビルド後スクラッチは不要
				blas.release_scratch();
			
			PIX_SCOPED_EVENT(command_base.type == command::type_build_bottom_level_acceleration_structure ? "build_bottom_level_acceleration_structure" : "refit_bottom_level_acceleration_structure");
			m_graphics_command_list->BuildRaytracingAccelerationStructure(&desc, p_info ? 1 : 0, p_info);
			break;
		}
		case command::type_compact_bottom_level_acceleration_structure:
		{
			PIX_SCOPED_EVENT("compact_bottom_level_acceleration_structure");
			auto& command = static_cast<const command::compact_bottom_level_acceleration_structure&>(command_base);
			auto& blas = *static_cast<d3d12::bottom_level_acceleration_structure*>(command.p_blas);
			switch(blas.compaction_state())
			{
			case raytracing_compaction_state_ready: //readbackへコピー
				blas.set_compaction_state(*this, raytracing_compaction_state_progress0);
				m_graphics_command_list->CopyBufferRegion(blas.compacted_size_readback().get_impl(), 0, blas.compacted_size().get_impl(), 0, sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE_DESC));
				break;
			case raytracing_compaction_state_progress0: //readbackへのコピー完了待ち
				blas.set_compaction_state(*this, raytracing_compaction_state_progress1);
				break;
			case raytracing_compaction_state_progress1: //コンパクション実行
				blas.set_compaction_state(*this, raytracing_compaction_state_progress2);
				m_graphics_command_list->CopyRaytracingAccelerationStructure(blas.compacted_result()->GetGPUVirtualAddress(), blas->GetGPUVirtualAddress(), D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_COMPACT);
				break;
			case raytracing_compaction_state_progress2: //コンパクション結果に置き換え
				blas.set_compaction_state(*this, raytracing_compaction_state_completed);
				break;
			default:
				assert(0);
			}
			break;
		}
		case command::type_update_buffer:
		{
			PIX_SCOPED_EVENT("update_buffer");
			auto& command = static_cast<const command::update_buffer&>(command_base);
			auto& dst = *static_cast<d3d12::buffer*>(command.p_buf);
			auto src_offset = static_cast<const uint8_t*>(command.p_update) - m_upload_buffer.data<uint8_t>();
			m_graphics_command_list->CopyBufferRegion(dst.get_impl(), command.offset, m_upload_buffer.get_impl(), src_offset, command.size); 
			break;
		}
		case command::type_copy_buffer:
		{
			PIX_SCOPED_EVENT("copy_buffer");
			auto& command = static_cast<const command::copy_buffer&>(command_base);
			auto* dst = command.dst_is_readback ? static_cast<d3d12::readback_buffer*>(command.p_dst)->get_impl() : static_cast<d3d12::buffer*>(command.p_dst)->get_impl();
			auto* src = command.src_is_upload ? static_cast<d3d12::upload_buffer*>(command.p_src)->get_impl() : static_cast<d3d12::buffer*>(command.p_src)->get_impl();
			m_graphics_command_list->CopyBufferRegion(dst, command.dst_offset, src, command.src_offset, command.size);
			break;
		}
		case command::type_copy_texture:
		{
			PIX_SCOPED_EVENT("copy_texture");
			auto& command = static_cast<const command::copy_texture&>(command_base);
			auto p_dst = static_cast<d3d12::texture*>(command.p_dst);
			auto p_src = static_cast<d3d12::texture*>(command.p_src);

			D3D12_TEXTURE_COPY_LOCATION dst_loc;
			D3D12_TEXTURE_COPY_LOCATION src_loc;
			dst_loc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			src_loc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			dst_loc.pResource = p_dst->get_impl();
			src_loc.pResource = p_src->get_impl();

			const uint plane_count = d3d12::plane_count((*p_dst)->GetDesc().Format);
			for(uint plane_slice = 0; plane_slice < plane_count; plane_slice++)
			{
				for(uint array_slice = 0; array_slice < p_dst->array_size(); array_slice++)
				{
					for(uint mip_slice = 0; mip_slice < p_dst->mip_levels(); mip_slice++)
					{
						dst_loc.SubresourceIndex = subresource_index(*p_dst, mip_slice, array_slice, plane_slice);
						src_loc.SubresourceIndex = subresource_index(*p_src, mip_slice, array_slice, plane_slice);
						m_graphics_command_list->CopyTextureRegion(&dst_loc, 0, 0, 0, &src_loc, nullptr);
					}
				}
			}
			break;
		}
		case command::type_copy_resource:
		{
			PIX_SCOPED_EVENT("copy_buffer");
			auto& command = static_cast<const command::copy_resource&>(command_base);
			auto& dst = *static_cast<d3d12::resource*>(command.p_dst);
			auto& src = *static_cast<d3d12::resource*>(command.p_src);
			m_graphics_command_list->CopyResource(dst.get_impl(), src.get_impl());
			break;
		}
		case command::type_present:
		{
			//presentが最後の命令
			{ PIX_SCOPED_EVENT("present"); }
			m_graphics_command_list->Close();
			break;
		}
		case command::type_call_function:
		{
			PIX_SCOPED_EVENT("call_function");
			auto& command = static_cast<const command::call_function&>(command_base);
			auto* p_target_state = static_cast<const d3d12::target_state*>(command.p_target_state);
			if(p_target_state)
			{
				D3D12_RECT scissor[8];
				D3D12_VIEWPORT viewport[8];
				D3D12_CPU_DESCRIPTOR_HANDLE rt_descriptors[8];
				for(uint i = 0; i < p_target_state->num_rtvs(); i++)
				{
					rt_descriptors[i] = p_target_state->rtv(i);
				
					const auto& texture = static_cast<d3d12::texture&>(p_target_state->rtv(i).resource());
					scissor[i].left = scissor[i].top = 0;
					scissor[i].right = texture.width();
					scissor[i].bottom = texture.height();
					viewport[i].TopLeftX = viewport[i].TopLeftY = 0;
					viewport[i].Width = float(texture.width());
					viewport[i].Height = float(texture.height());
					viewport[i].MinDepth = 0;
					viewport[i].MaxDepth = 1;
				}
				m_graphics_command_list->RSSetViewports(p_target_state->num_rtvs(), viewport);
				m_graphics_command_list->RSSetScissorRects(p_target_state->num_rtvs(), scissor);
				m_graphics_command_list->OMSetRenderTargets(p_target_state->num_rtvs(), rt_descriptors, FALSE, p_target_state->has_dsv() ? p_target_state->dsv() : nullptr);
			}
			command.func();
			break;
		}
		default:
			assert(0);
		}
	}

	//static auto prev = std::chrono::high_resolution_clock::now();
	//auto cur = std::chrono::high_resolution_clock::now();
	//BOOL full = false;
	//swapchain->GetFullscreenState(&full, nullptr);
	//std::cout << full << " " << std::chrono::duration_cast<std::chrono::nanoseconds>(cur- prev).count() / 1000.0 / 1000.0 << std::endl;
	//prev = cur;

	//実行&Present
	ID3D12CommandList *command_list_ptrs[] = {m_graphics_command_list.get_impl() };
	m_graphics_command_queue->ExecuteCommandLists(1, command_list_ptrs);
	swapchain->Present(1, 0);

	//同期ポイント
	m_graphics_fence_value++;
	m_graphics_command_queue->Signal(m_graphics_fence.get_impl(), m_graphics_fence_value);

	//前の実行完了を待機
	m_graphics_fence->SetEventOnCompletion(m_graphics_fence_prev_value, m_graphics_event);
	m_graphics_fence_prev_value = m_graphics_fence_value;
	WaitForSingleObject(m_graphics_event, INFINITE);

	//前フレームのメモリを解放
	m_upload_buffer_allocator.dellocate();
	m_sv_sampler_descriptor_heap.deallocate();
	m_sv_cbv_srv_uav_descriptor_heap.deallocate();
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//GPU処理完了を待機
inline void render_device::wait_idle()
{
	m_graphics_fence->SetEventOnCompletion(m_graphics_fence_value, m_graphics_event);
	WaitForSingleObject(m_graphics_event, INFINITE);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//シェーダコンパイラを作成
inline shader_compiler_ptr render_device::create_shader_compiler()
{
	return std::make_shared<shader_compiler>();
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//スワップチェインを作成
inline swapchain_ptr render_device::create_swapchain(const uint2 size)
{
	return new swapchain(m_graphics_command_queue, m_hwnd, size);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//テクスチャを作成
inline texture_ptr render_device::create_texture1d(const texture_format format, const uint width, const uint mip_levels, const resource_flags flags, const void* data)
{
	return new texture1d(*this, format, width, 1, 1, mip_levels, flags, data, D3D12_RESOURCE_DIMENSION_TEXTURE1D);
}

inline texture_ptr render_device::create_texture2d(const texture_format format, const uint width, const uint height, const uint mip_levels, const resource_flags flags, const void* data)
{
	return new texture2d(*this, format, width, height, 1, mip_levels, flags, data, D3D12_RESOURCE_DIMENSION_TEXTURE2D);
}

inline texture_ptr render_device::create_texture3d(const texture_format format, const uint width, const uint height, const uint depth, const uint mip_levels, const resource_flags flags, const void* data)
{
	return new texture3d(*this, format, width, height, depth, mip_levels, flags, data, D3D12_RESOURCE_DIMENSION_TEXTURE3D);
}

inline texture_ptr render_device::create_texture_cube(const texture_format format, const uint width, const uint height, const uint mip_levels, const resource_flags flags, const void* data)
{
	return new texture_cube(*this, format, width, height, 6, mip_levels, flags, data, D3D12_RESOURCE_DIMENSION_TEXTURE2D);
}

inline texture_ptr render_device::create_texture1d_array(const texture_format format, const uint width, const uint depth, const uint mip_levels, const resource_flags flags, const void* data)
{
	return new texture1d_array(*this, format, width, 1, depth, mip_levels, flags, data, D3D12_RESOURCE_DIMENSION_TEXTURE1D);
}

inline texture_ptr render_device::create_texture2d_array(const texture_format format, const uint width, const uint height, const uint depth, const uint mip_levels, const resource_flags flags, const void* data)
{
	return new texture2d_array(*this, format, width, height, depth, mip_levels, flags, data, D3D12_RESOURCE_DIMENSION_TEXTURE2D);
}

inline texture_ptr render_device::create_texture_cube_array(const texture_format format, const uint width, const uint height, const uint depth, const uint mip_levels, const resource_flags flags, const void* data)
{
	throw; //要テスト
	//return new texture_cube_array(*this, format, width, height, 6 * depth, mip_levels, flags, data, D3D12_RESOURCE_DIMENSION_TEXTURE2D);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//バッファを作成
inline buffer_ptr render_device::create_structured_buffer(const uint stride, const uint num_elements, const resource_flags flags, const void* data)
{
	return new structured_buffer(*this, stride, num_elements, flags, data);
}

inline buffer_ptr render_device::create_byteaddress_buffer(const uint size, const resource_flags flags, const void* data)
{
	return new byteaddress_buffer(*this, 4, ceil_div(size, 4), flags, data);
}

inline constant_buffer_ptr render_device::create_temporary_cbuffer(const uint size, const void* data)
{
	const uint alignment = 256;
	const uint offset = roundup(uint(static_cast<uint8_t*>(get_update_buffer_pointer(size + alignment - 1)) - m_upload_buffer.data<uint8_t>()), alignment);
	return new temporary_constant_buffer(*this, size, data, m_upload_buffer.data<uint8_t>() + offset, create_constant_buffer_view(m_upload_buffer->GetGPUVirtualAddress() + offset, roundup(size, alignment)));
}

inline constant_buffer_ptr render_device::create_constant_buffer(const uint size, const void* data)
{
	const uint alignment = 256;
	return new static_constant_buffer(*this, roundup(size, alignment), data);
}

inline upload_buffer_ptr render_device::create_upload_buffer(const uint size)
{
	return new upload_buffer(*this, size, nullptr);
}

inline readback_buffer_ptr render_device::create_readback_buffer(const uint size)
{
	return new readback_buffer(*this, size);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//サンプラを作成
inline sampler_ptr render_device::create_sampler(const sampler_desc& desc)
{
	D3D12_SAMPLER_DESC sampler_desc = {};
	sampler_desc.Filter = D3D12_FILTER(desc.filter);
	sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE(desc.address_u);
	sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE(desc.address_v);
	sampler_desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE(desc.address_w);
	sampler_desc.BorderColor[0] = desc.border_color[0];
	sampler_desc.BorderColor[1] = desc.border_color[1];
	sampler_desc.BorderColor[2] = desc.border_color[2];
	sampler_desc.BorderColor[3] = desc.border_color[3];
	sampler_desc.MaxAnisotropy = desc.max_anisotropy;
	sampler_desc.MaxLOD = desc.max_lod;
	sampler_desc.MinLOD = desc.min_lod;
	sampler_desc.MipLODBias = desc.lod_bias;

	D3D12_CPU_DESCRIPTOR_HANDLE handle = m_sampler_descriptor_heap.allocate();
	mp_impl->CreateSampler(&sampler_desc, handle);
	return new sampler(cpu_descriptor(handle, m_sampler_descriptor_heap));
}
	
///////////////////////////////////////////////////////////////////////////////////////////////////

//TLASを作成
inline top_level_acceleration_structure_ptr render_device::create_top_level_acceleration_structure(const uint num_instances)
{
	return new top_level_acceleration_structure(*this, num_instances);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//BLASを作成
inline bottom_level_acceleration_structure_ptr render_device::create_bottom_level_acceleration_structure(const render::geometry_state& gs, const raytracing_geometry_desc* descs, const uint num_descs, const bool allow_update)
{
	return new bottom_level_acceleration_structure(*this, static_cast<const geometry_state&>(gs), descs, num_descs, allow_update);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//Viewを作成
inline render_target_view_ptr render_device::create_render_target_view(render::texture& tex_base, const texture_rtv_desc& desc)
{
	auto& tex = static_cast<texture&>(tex_base);
	const D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = tex.create_rtv_desc(desc);
	D3D12_CPU_DESCRIPTOR_HANDLE handle = m_rtv_descriptor_heap.allocate();
	mp_impl->CreateRenderTargetView(tex.get_impl(), &rtv_desc, handle);
	return new render_target_view(rtv_desc.Format, cpu_descriptor(handle, m_rtv_descriptor_heap), tex, desc);
}

inline depth_stencil_view_ptr render_device::create_depth_stencil_view(render::texture& tex_base, const texture_dsv_desc& desc)
{
	auto& tex = static_cast<texture&>(tex_base);
	const D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc = tex.create_dsv_desc(desc);
	D3D12_CPU_DESCRIPTOR_HANDLE handle = m_dsv_descriptor_heap.allocate();
	mp_impl->CreateDepthStencilView(tex.get_impl(), &dsv_desc, handle);
	return new depth_stencil_view(dsv_desc.Format, cpu_descriptor(handle, m_dsv_descriptor_heap), tex, desc);
}

inline shader_resource_view_ptr render_device::create_shader_resource_view(render::texture& tex_base, const texture_srv_desc& desc)
{
	auto& tex = static_cast<texture&>(tex_base);
	const D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = tex.create_srv_desc(desc);
	D3D12_CPU_DESCRIPTOR_HANDLE handle = m_cbv_srv_uav_descriptor_heap.allocate();
	mp_impl->CreateShaderResourceView(tex.get_impl(), &srv_desc, handle);
	return new texture_shader_resource_view(srv_desc.Format, cpu_descriptor(handle, m_cbv_srv_uav_descriptor_heap), tex, desc);
}

inline unordered_access_view_ptr render_device::create_unordered_access_view(render::texture& tex_base, const texture_uav_desc& desc)
{
	auto& tex = static_cast<texture&>(tex_base);
	const D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = tex.create_uav_desc(desc);
	D3D12_CPU_DESCRIPTOR_HANDLE handle = m_cbv_srv_uav_descriptor_heap.allocate();
	mp_impl->CreateUnorderedAccessView(tex.get_impl(), nullptr, &uav_desc, handle);
	return new texture_unordered_access_view(cpu_descriptor(handle, m_cbv_srv_uav_descriptor_heap), tex, desc);
}

inline shader_resource_view_ptr render_device::create_shader_resource_view(render::buffer& buf_base, const buffer_srv_desc& desc)
{
	auto& buf = static_cast<buffer&>(buf_base);
	const D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = buf.create_srv_desc(desc);
	D3D12_CPU_DESCRIPTOR_HANDLE handle = m_cbv_srv_uav_descriptor_heap.allocate();
	mp_impl->CreateShaderResourceView(buf.get_impl(), &srv_desc, handle);
	return new buffer_shader_resource_view(cpu_descriptor(handle, m_cbv_srv_uav_descriptor_heap), buf, desc);
}

inline unordered_access_view_ptr render_device::create_unordered_access_view(render::buffer& buf_base, const buffer_uav_desc& desc)
{
	auto& buf = static_cast<buffer&>(buf_base);
	const D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = buf.create_uav_desc(desc);
	D3D12_CPU_DESCRIPTOR_HANDLE handle = m_cbv_srv_uav_descriptor_heap.allocate();
	mp_impl->CreateUnorderedAccessView(buf.get_impl(), nullptr, &uav_desc, handle);
	return new buffer_unordered_access_view(cpu_descriptor(handle, m_cbv_srv_uav_descriptor_heap), buf, desc);
}

inline cpu_descriptor render_device::create_constant_buffer_view(const D3D12_GPU_VIRTUAL_ADDRESS gpu_address, const uint size)
{
	D3D12_CONSTANT_BUFFER_VIEW_DESC desc;
	desc.BufferLocation = gpu_address;
	desc.SizeInBytes = size;

	D3D12_CPU_DESCRIPTOR_HANDLE handle = m_cbv_srv_uav_descriptor_heap.allocate();
	mp_impl->CreateConstantBufferView(&desc, handle);
	return cpu_descriptor(handle, m_cbv_srv_uav_descriptor_heap);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//パイプラインステートを作成
inline pipeline_state_ptr render_device::create_compute_pipeline_state(const string& name, shader_ptr p_cs)
{
	return new compute_pipeline_state(*this, name, std::move(p_cs));
}

inline pipeline_state_ptr render_device::create_graphics_pipeline_state(const string& name, shader_ptr p_vs, shader_ptr p_ps, const input_layout_desc& il, const rasterizer_desc& rs, const depth_stencil_desc& dss, const blend_desc& bs)
{
	return new graphics_pipeline_state(name, std::move(p_vs), std::move(p_ps), il, rs, dss, bs);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//ターゲットステートを作成
inline target_state_ptr render_device::create_target_state(const uint num_rtvs, render::render_target_view* rtv_ptrs[], render::depth_stencil_view* p_dsv)
{
	return new target_state(num_rtvs, rtv_ptrs, p_dsv);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//ジオメトリステートを作成
inline geometry_state_ptr render_device::create_geometry_state(const uint num_vbs, render::buffer* vb_ptrs[8], uint offsets[8], uint strides[8], render::buffer* p_ib)
{
	return new geometry_state(num_vbs, vb_ptrs, offsets, strides, p_ib);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//バインドレスリソースとして登録
inline void render_device::register_bindless(render::sampler& sampler)
{
	const auto alloc = m_sv_sampler_descriptor_heap.allocate_bindless();
	mp_impl->CopyDescriptorsSimple(1, alloc.cpu_start, static_cast<const d3d12::sampler&>(sampler), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	m_bindless_sampler[alloc.index] = &sampler;
	set_bindless_handle(sampler, alloc.index);
}

inline void render_device::register_bindless(render::constant_buffer& cbv)
{
	const auto alloc = m_sv_cbv_srv_uav_descriptor_heap.allocate_bindless();
	mp_impl->CopyDescriptorsSimple(1, alloc.cpu_start, static_cast<const d3d12::constant_buffer&>(cbv), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_bindless_cbv_srv_uav[alloc.index] = &cbv;
	set_bindless_handle(cbv, alloc.index);
}

inline void render_device::register_bindless(render::shader_resource_view& srv)
{
	const auto alloc = m_sv_cbv_srv_uav_descriptor_heap.allocate_bindless();
	mp_impl->CopyDescriptorsSimple(1, alloc.cpu_start, static_cast<d3d12::shader_resource_view&>(srv), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_bindless_cbv_srv_uav[alloc.index] = &srv;
	set_bindless_handle(srv, alloc.index);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//バインドレスリソースの登録解除
inline void render_device::unregister_bindless(render::sampler& sampler)
{
	m_sv_sampler_descriptor_heap.deallocate_bindless(sampler.bindless_handle());
	m_bindless_sampler[sampler.bindless_handle()] = nullptr;
	set_bindless_handle(sampler, invalid_bindless_handle);
}

inline void render_device::unregister_bindless(render::constant_buffer& cbv)
{
	m_sv_cbv_srv_uav_descriptor_heap.deallocate_bindless(cbv.bindless_handle());
	m_bindless_cbv_srv_uav[cbv.bindless_handle()] = nullptr;
	set_bindless_handle(cbv, invalid_bindless_handle);
}

inline void render_device::unregister_bindless(render::shader_resource_view& srv)
{
	m_sv_cbv_srv_uav_descriptor_heap.deallocate_bindless(srv.bindless_handle());
	m_bindless_cbv_srv_uav[srv.bindless_handle()] = nullptr;
	set_bindless_handle(srv, invalid_bindless_handle);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//名前を設定
inline void render_device::set_name(render::buffer& buf, const wchar_t* name)
{
	static_cast<d3d12::buffer&>(buf)->SetName(name);
}

inline void render_device::set_name(render::upload_buffer& buf, const wchar_t* name)
{
	static_cast<d3d12::upload_buffer&>(buf)->SetName(name);
}

inline void render_device::set_name(render::constant_buffer& buf, const wchar_t* name)
{
	static_cast<d3d12::static_constant_buffer&>(buf)->SetName(name);
}

inline void render_device::set_name(render::readback_buffer& buf, const wchar_t* name)
{
	static_cast<d3d12::readback_buffer&>(buf)->SetName(name);
}

inline void render_device::set_name(render::texture& tex, const wchar_t* name)
{
	static_cast<d3d12::texture&>(tex)->SetName(name);
}

inline void render_device::set_name(render::top_level_acceleration_structure& tlas, const wchar_t* name)
{
	static_cast<d3d12::top_level_acceleration_structure&>(tlas)->SetName(name);
}

inline void render_device::set_name(render::bottom_level_acceleration_structure& blas, const wchar_t* name)
{
	static_cast<d3d12::bottom_level_acceleration_structure&>(blas)->SetName(name);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//リソースをコピー
inline void render_device::copy_resource(resource& dst, resource& src)
{
	m_copy_command_allocator->Reset();
	m_copy_command_list->Reset(m_copy_command_allocator.get_impl(), nullptr);
	m_copy_command_list->CopyResource(dst.get_impl(), src.get_impl());
	m_copy_command_list->Close();

	ID3D12CommandList* copy_command_list = m_copy_command_list.get_impl();
	m_copy_command_queue->ExecuteCommandLists(1, &copy_command_list);

	const size_t fence_value = m_copy_fence->GetCompletedValue() + 1;
	m_copy_command_queue->Signal(m_copy_fence.get_impl(), fence_value);
	m_copy_fence->SetEventOnCompletion(fence_value, m_copy_event);
	WaitForSingleObject(m_copy_event, INFINITE);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//バッファをコピー
inline void render_device::copy_buffer(buffer& dst, const uint dst_offset, upload_buffer& src, const uint src_offset, const uint size)
{
	m_copy_command_allocator->Reset();
	m_copy_command_list->Reset(m_copy_command_allocator.get_impl(), nullptr);
	m_copy_command_list->CopyBufferRegion(dst.get_impl(), dst_offset, src.get_impl(), src_offset, size);
	m_copy_command_list->Close();

	ID3D12CommandList* copy_command_list = m_copy_command_list.get_impl();
	m_copy_command_queue->ExecuteCommandLists(1, &copy_command_list);

	const size_t fence_value = m_copy_fence->GetCompletedValue() + 1;
	m_copy_command_queue->Signal(m_copy_fence.get_impl(), fence_value);
	m_copy_fence->SetEventOnCompletion(fence_value, m_copy_event);
	WaitForSingleObject(m_copy_event, INFINITE);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//テクスチャをコピー
inline void render_device::copy_texture(texture& dst, upload_buffer& src, const D3D12_PLACED_SUBRESOURCE_FOOTPRINT* footprints, const uint num_subresources)
{
	D3D12_TEXTURE_COPY_LOCATION dst_loc;
	dst_loc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	dst_loc.pResource = dst.get_impl();

	D3D12_TEXTURE_COPY_LOCATION src_loc;
	src_loc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	src_loc.pResource = src.get_impl();

	m_copy_command_allocator->Reset();
	m_copy_command_list->Reset(m_copy_command_allocator.get_impl(), nullptr);
	for(uint i = 0; i < num_subresources; i++)
	{
		dst_loc.SubresourceIndex = i;
		src_loc.PlacedFootprint = footprints[i];
		m_copy_command_list->CopyTextureRegion(&dst_loc, 0, 0, 0, &src_loc, nullptr);
	}
	m_copy_command_list->Close();

	ID3D12CommandList* copy_command_list = m_copy_command_list.get_impl();
	m_copy_command_queue->ExecuteCommandLists(1, &copy_command_list);

	const size_t fence_value = m_copy_fence->GetCompletedValue() + 1;
	m_copy_command_queue->Signal(m_copy_fence.get_impl(), fence_value);
	m_copy_fence->SetEventOnCompletion(fence_value, m_copy_event);
	WaitForSingleObject(m_copy_event, INFINITE);
}
	
///////////////////////////////////////////////////////////////////////////////////////////////////

//バッファ更新用のポインタを返す
inline void* render_device::get_update_buffer_pointer(const uint size)
{
	const size_t pos = m_upload_buffer_allocator.allocate(ceil_div(size, upload_buffer_alignment));
	return m_upload_buffer.data<uint8_t>() + (upload_buffer_alignment * pos);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//実行可能かのチェックと実行に必要なデータを作成
inline void* render_device::validate(const render::pipeline_state& ps_base, const unordered_map<string, bind_resource>& resources)
{
	auto& pipeline_state = static_cast<const d3d12::compute_pipeline_state&>(ps_base);
	auto& resource_count = pipeline_state.resource_count();

	const d3d12::shader* p_shader = static_cast<d3d12::shader*>(pipeline_state.p_cs().get());
	return validate_impl(&p_shader, &resource_count, 1, resources);
}

inline void* render_device::validate(const render::pipeline_state& ps_base, const render::target_state& ts_base, const render::geometry_state& gs_base, const unordered_map<string, bind_resource>& resources)
{
	auto& target_state = static_cast<const d3d12::target_state&>(ts_base);
	auto& geometry_state = static_cast<const d3d12::geometry_state&>(gs_base);
	auto& pipeline_state = static_cast<const d3d12::graphics_pipeline_state&>(ps_base);
	auto& record = pipeline_state.get(*this, target_state, geometry_state);
	auto& shader_resource_count_table = record.pipeline_resource_count.shader_resource_count_table;

	const d3d12::shader* shader_ptrs[] = {
		static_cast<d3d12::shader*>(pipeline_state.p_vs().get()),
		static_cast<d3d12::shader*>(pipeline_state.p_ps().get()),
	};
	return validate_impl(shader_ptrs, shader_resource_count_table, 2, resources);
}

inline void* render_device::validate_impl(const shader** shader_ptrs, const shader_resource_count* resource_count_table, const uint num_shaders, const unordered_map<string, bind_resource>& resources)
{
	uint num_cbvs = 0;
	uint num_srvs = 0;
	uint num_uavs = 0;
	uint num_samplers = 0;
	for(uint i = 0; i < num_shaders; i++)
	{
		num_cbvs += resource_count_table[i].cbv;
		num_srvs += resource_count_table[i].srv;
		num_uavs += resource_count_table[i].uav;
		num_samplers += resource_count_table[i].sampler;
	}

	const auto sampler_descriptor_size = m_sv_sampler_descriptor_heap.descriptor_size();
	const auto cbv_srv_uav_descriptor_size = m_sv_cbv_srv_uav_descriptor_heap.descriptor_size();
	const auto sampler_descriptors = m_sv_sampler_descriptor_heap.allocate(num_samplers);
	const auto cbv_srv_uav_descriptors = m_sv_cbv_srv_uav_descriptor_heap.allocate(num_cbvs + num_srvs + num_uavs);
	
	auto sampler_heap_start = sampler_descriptors.cpu_start.ptr;
	auto cbv_srv_uav_heap_start = cbv_srv_uav_descriptors.cpu_start.ptr;

	auto* p_bind_info = static_cast<bind_info*>(m_command_manager.allocate(sizeof(bind_info) + sizeof(bind_info::view) * (num_srvs + num_uavs)));
	p_bind_info->sampler_heap_start = sampler_descriptors.gpu_start;
	p_bind_info->cbv_srv_uav_heap_start = cbv_srv_uav_descriptors.gpu_start;
	p_bind_info->root_constant_index = uint8_t(-1);
	p_bind_info->root_tlas_srv_index = uint8_t(-1);
	p_bind_info->num_views = 0;

	bool use_root_constant = false;
	bool use_root_tlas_srv = false;

	uint num_parameters = 0;
	for(uint i = 0; i < num_shaders; i++)
	{
		if(resource_count_table[i].cbv + resource_count_table[i].srv + resource_count_table[i].uav > 0)
			num_parameters++;
		if(resource_count_table[i].sampler > 0)
			num_parameters++;

		uint offset[] = 
		{
			0u, 
			uint(resource_count_table[i].cbv), 
			uint(resource_count_table[i].cbv + resource_count_table[i].srv), 
			0u,
		};

		auto& shader = *shader_ptrs[i];
		auto& reflection = shader.reflection();

		D3D12_SHADER_DESC shader_desc;
		reflection.GetDesc(&shader_desc);
		const bool is_pixel_shader = ((shader_desc.Version & 0xffff0000) >> 16 == D3D12_SHVER_PIXEL_SHADER);

		for(uint j = 0; j < shader_desc.BoundResources; j++)
		{
			D3D12_SHADER_INPUT_BIND_DESC input_bind_desc;
			reflection.GetResourceBindingDesc(j, &input_bind_desc);
		
			auto not_found_error = [&]()
			{
				std::cout << input_bind_desc.Name << " is not found." << std::endl;
			};
			auto type_error = [&](const char* type, const bind_resource& res)
			{
				std::cout << input_bind_desc.Name << " is " << type << ", but ";
				if(res.is_sampler()){ std::cout << "sampler"; }
				else if(res.is_cbv()){ std::cout << "cbv"; }
				else if(res.is_srv()){ std::cout << "srv"; }
				else if(res.is_uav()){ std::cout << "uav"; }
				else if(res.is_tlas()) {std::cout << "tlas"; }
				std::cout << " is bound." << std::endl;
			};

			if(input_bind_desc.Space != 0)
			{
				switch(input_bind_desc.Type)
				{
				case D3D_SIT_CBUFFER:
				{
					use_root_constant = true;
					break;
				}
				case D3D_SIT_RTACCELERATIONSTRUCTURE:
				{
					if(not(use_root_tlas_srv))
					{
						auto it = resources.find(input_bind_desc.Name);
						if(it == resources.end()){ return not_found_error(), nullptr; }
						if(not(it->second.is_tlas())){ return type_error("tlas", it->second), nullptr; }
						p_bind_info->p_tlas = &static_cast<d3d12::top_level_acceleration_structure&>(it->second.tlas());
					}
					use_root_tlas_srv = true;
					break;
				}
				case D3D_SIT_SAMPLER:
					break;
				default:
					assert(0);
				}
				continue;
			}

			switch(input_bind_desc.Type)
			{
			case D3D_SIT_CBUFFER:
			{
				auto it = resources.find(input_bind_desc.Name);
				if(it == resources.end()){ return not_found_error(), nullptr; } 
				if(not(it->second.is_cbv())){ return type_error("cbv", it->second), nullptr; }
				mp_impl->CopyDescriptorsSimple(1, D3D12_CPU_DESCRIPTOR_HANDLE{cbv_srv_uav_heap_start + cbv_srv_uav_descriptor_size * offset[0]++}, static_cast<const d3d12::constant_buffer&>(it->second.cbv()), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				break;
			}
			case D3D_SIT_TEXTURE:
			case D3D_SIT_STRUCTURED:
			case D3D_SIT_BYTEADDRESS:
			{
				auto it = resources.find(input_bind_desc.Name);
				if(it == resources.end()){ return not_found_error(), nullptr; } 
				if(not(it->second.is_srv())){ return type_error("srv", it->second), nullptr; }
				mp_impl->CopyDescriptorsSimple(1, D3D12_CPU_DESCRIPTOR_HANDLE{cbv_srv_uav_heap_start + cbv_srv_uav_descriptor_size * offset[1]++}, static_cast<const d3d12::shader_resource_view&>(it->second.srv()), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				p_bind_info->views[p_bind_info->num_views++].set(&it->second.srv(), true, is_pixel_shader);
				break;
			}
			case D3D_SIT_UAV_RWTYPED:
			case D3D_SIT_UAV_RWSTRUCTURED:
			case D3D_SIT_UAV_RWBYTEADDRESS:
			{
				auto it = resources.find(input_bind_desc.Name);
				if(it == resources.end()){ return not_found_error(), nullptr; } 
				if(not(it->second.is_uav())){ return type_error("uav", it->second), nullptr; }
				auto& uav = static_cast<const d3d12::unordered_access_view&>(it->second.uav());
				
				mp_impl->CopyDescriptorsSimple(1, D3D12_CPU_DESCRIPTOR_HANDLE{cbv_srv_uav_heap_start + cbv_srv_uav_descriptor_size * offset[2]++}, static_cast<const d3d12::unordered_access_view&>(it->second.uav()), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				p_bind_info->views[p_bind_info->num_views++].set(&it->second.uav(), false, is_pixel_shader);
				break;
			}
			case D3D_SIT_SAMPLER:
			{
				auto it = resources.find(input_bind_desc.Name);
				if(it == resources.end()){ return not_found_error(), nullptr; } 
				if(not(it->second.is_sampler())){ return type_error("sampler", it->second), nullptr; }
				mp_impl->CopyDescriptorsSimple(1, D3D12_CPU_DESCRIPTOR_HANDLE{sampler_heap_start + sampler_descriptor_size * offset[3]++}, static_cast<const d3d12::sampler&>(it->second.sampler()), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
				break;
			}
			default:
				assert(0);
			}
		}

		//次のシェーダの書き込み先を設定
		sampler_heap_start += sampler_descriptor_size * resource_count_table[i].sampler;
		cbv_srv_uav_heap_start += cbv_srv_uav_descriptor_size * (resource_count_table[i].cbv + resource_count_table[i].srv + resource_count_table[i].uav);
	}

	if(use_root_constant)
		p_bind_info->root_constant_index = num_parameters++;
	if(use_root_tlas_srv)
		p_bind_info->root_tlas_srv_index = num_parameters++;

	return p_bind_info;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//ルートシグネチャを作成
inline pair<ComPtr<ID3D12RootSignature>, pipeline_resource_count> render_device::create_root_signature(const shader** shader_ptrs, const uint num_shaders)
{
	const uint max_num = 6;
	assert(num_shaders < max_num);

	D3D12_DESCRIPTOR_RANGE sampler_ranges[max_num] = {};
	D3D12_DESCRIPTOR_RANGE cbv_srv_uav_ranges[3 * max_num] = {};
	D3D12_ROOT_PARAMETER root_parameters[max_num + 2] = {};
	D3D12_ROOT_PARAMETER& root_constant_parameter = root_parameters[max_num + 0];
	D3D12_ROOT_PARAMETER& root_tlas_srv_parameter = root_parameters[max_num + 1];
	D3D12_ROOT_SIGNATURE_DESC root_signature_desc ={};
	root_signature_desc.pParameters = root_parameters;
	root_signature_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
	root_signature_desc.NumParameters = 0;

	D3D12_DESCRIPTOR_RANGE *p_sampler_range = sampler_ranges;
	D3D12_DESCRIPTOR_RANGE *p_cbv_srv_uav_range = cbv_srv_uav_ranges;

	pipeline_resource_count pipeline_resource_count;
	for(uint i = 0; i < num_shaders; i++)
	{
		auto& shader = *shader_ptrs[i];
		auto& reflection = shader.reflection();
		auto& resource_count = pipeline_resource_count.shader_resource_count_table[i];
		auto& root_parameter = root_parameters[root_signature_desc.NumParameters];

		const auto flags = reflection.GetRequiresFlags();
		if(flags & D3D_SHADER_REQUIRES_RESOURCE_DESCRIPTOR_HEAP_INDEXING)
			root_signature_desc.Flags = D3D12_ROOT_SIGNATURE_FLAGS(root_signature_desc.Flags | D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED);

		D3D12_SHADER_DESC shader_desc;
		reflection.GetDesc(&shader_desc);

		D3D12_SHADER_VISIBILITY visibility;
		switch((shader_desc.Version & 0xffff0000) >> 16)
		{
		case D3D12_SHVER_PIXEL_SHADER:
			visibility = D3D12_SHADER_VISIBILITY_PIXEL;
			break;
		case D3D12_SHVER_VERTEX_SHADER:
			visibility = D3D12_SHADER_VISIBILITY_VERTEX;
			root_signature_desc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
			break;
		case D3D12_SHVER_GEOMETRY_SHADER:
			visibility = D3D12_SHADER_VISIBILITY_GEOMETRY;
			break;
		case D3D12_SHVER_COMPUTE_SHADER:
			visibility = D3D12_SHADER_VISIBILITY_ALL;
			break;
		case D3D12_SHVER_HULL_SHADER:
			visibility = D3D12_SHADER_VISIBILITY_HULL;
			break;
		case D3D12_SHVER_DOMAIN_SHADER:
			visibility = D3D12_SHADER_VISIBILITY_DOMAIN;
			break;
		case D3D12_SHVER_MESH_SHADER:
			visibility = D3D12_SHADER_VISIBILITY_MESH;
			break;
		case D3D12_SHVER_AMPLIFICATION_SHADER:
			visibility = D3D12_SHADER_VISIBILITY_AMPLIFICATION;
			break;
		default:
			assert(0);
		}

		for(uint j = 0; j < shader_desc.BoundResources; j++)
		{
			D3D12_SHADER_INPUT_BIND_DESC input_bind_desc;
			reflection.GetResourceBindingDesc(j, &input_bind_desc);
		
			//DescriptorTableを使うやつだけspace=0にするルール
			//space!=0はRootConstant,StaticSampler,BVH

			if(input_bind_desc.Space != 0)
			{
				switch(input_bind_desc.Type)
				{
				case D3D_SIT_CBUFFER:
				{
					if(root_constant_parameter.ParameterType != 0)
						root_constant_parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY(root_constant_parameter.ShaderVisibility | visibility);
					else
					{
						root_constant_parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
						root_constant_parameter.Constants.Num32BitValues = 1;
						root_constant_parameter.Constants.RegisterSpace = input_bind_desc.Space;
						root_constant_parameter.Constants.ShaderRegister = input_bind_desc.BindPoint;
						root_constant_parameter.ShaderVisibility = visibility;
						assert(input_bind_desc.Space == root_constant_bind_space);
						assert(input_bind_desc.BindPoint == root_constant_bind_point);
					}
					break;
				}
				case D3D_SIT_SAMPLER:
				{
					if(root_signature_desc.NumStaticSamplers == 0)
					{
						auto& static_samplers = get_static_samplers(static_sampler_bind_point, static_sampler_bind_point);
						root_signature_desc.NumStaticSamplers += uint(static_samplers.size());
						root_signature_desc.pStaticSamplers = static_samplers.data();
					}
					break;
				}
				case D3D_SIT_RTACCELERATIONSTRUCTURE:
				{
					if(root_tlas_srv_parameter.ParameterType != 0)
						root_tlas_srv_parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY(root_tlas_srv_parameter.ShaderVisibility | visibility);
					else
					{
						root_tlas_srv_parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
						root_tlas_srv_parameter.Descriptor.RegisterSpace = input_bind_desc.Space;
						root_tlas_srv_parameter.Descriptor.ShaderRegister = input_bind_desc.BindPoint;
						root_tlas_srv_parameter.ShaderVisibility = visibility;
						assert(input_bind_desc.Space == root_tlas_srv_bind_space);
						assert(input_bind_desc.BindPoint == root_tlas_srv_bind_point);
					}
					break;
				}
				default:
					throw;
				}
				continue;
			}

			switch(input_bind_desc.Type)
			{
			case D3D_SIT_CBUFFER:
				resource_count.cbv++;
				break;
			case D3D_SIT_TEXTURE:
			case D3D_SIT_STRUCTURED:
			case D3D_SIT_BYTEADDRESS:
				resource_count.srv++;
				break;
			case D3D_SIT_UAV_RWTYPED:
			case D3D_SIT_UAV_RWSTRUCTURED:
			case D3D_SIT_UAV_RWBYTEADDRESS:
				resource_count.uav++;
				break;
			case D3D_SIT_SAMPLER:
				resource_count.sampler++;
				break;
			default:
				throw;
			}
		}

		root_parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		root_parameter.DescriptorTable.pDescriptorRanges = p_cbv_srv_uav_range;
		root_parameter.ShaderVisibility = visibility;

		auto set = [&](const uint i, const D3D12_DESCRIPTOR_RANGE_TYPE type, uint num_descriptors)
		{
			p_cbv_srv_uav_range[i].RangeType = type;
			p_cbv_srv_uav_range[i].NumDescriptors = num_descriptors;
		};
		if(resource_count.cbv > 0){ set(root_parameter.DescriptorTable.NumDescriptorRanges++, D3D12_DESCRIPTOR_RANGE_TYPE_CBV, resource_count.cbv); }
		if(resource_count.srv > 0){ set(root_parameter.DescriptorTable.NumDescriptorRanges++, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, resource_count.srv); }
		if(resource_count.uav > 0){ set(root_parameter.DescriptorTable.NumDescriptorRanges++, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, resource_count.uav); }
		if(root_parameter.DescriptorTable.NumDescriptorRanges > 0)
		{
			p_cbv_srv_uav_range += root_parameter.DescriptorTable.NumDescriptorRanges;
			root_signature_desc.NumParameters++;
		}

		if(resource_count.sampler > 0)
		{
			auto& root_parameter = root_parameters[root_signature_desc.NumParameters++];
			root_parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			root_parameter.DescriptorTable.pDescriptorRanges = p_sampler_range;
			root_parameter.DescriptorTable.NumDescriptorRanges = 1;
			root_parameter.ShaderVisibility = visibility;
			p_sampler_range->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
			p_sampler_range->NumDescriptors = resource_count.sampler;
			p_sampler_range++;
		}
	}

	if(root_constant_parameter.ParameterType != 0)
		root_parameters[root_signature_desc.NumParameters++] = root_constant_parameter;
	if(root_tlas_srv_parameter.ParameterType != 0)
		root_parameters[root_signature_desc.NumParameters++] = root_tlas_srv_parameter;

	//全シェーダで共通のヒープ開始位置を使うのでオフセットを指定
	for(uint i = 0, offset = 0; sampler_ranges + i < p_sampler_range; i++)
		sampler_ranges[i].OffsetInDescriptorsFromTableStart = std::exchange(offset, offset + sampler_ranges[i].NumDescriptors);
	for(uint i = 0, offset = 0; cbv_srv_uav_ranges + i < p_cbv_srv_uav_range; i++)
		cbv_srv_uav_ranges[i].OffsetInDescriptorsFromTableStart = std::exchange(offset, offset + cbv_srv_uav_ranges[i].NumDescriptors);

	ComPtr<ID3DBlob> p_root_signature_blob, p_error_blob;
	D3D12SerializeRootSignature(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1, &p_root_signature_blob, &p_error_blob);
	if(p_error_blob != nullptr)
	{
		std::cout << static_cast<const char*>(p_error_blob->GetBufferPointer()) << std::endl;
		assert(0);
	}

	ComPtr<ID3D12RootSignature> p_root_signature;
	auto hr = mp_impl->CreateRootSignature(0, p_root_signature_blob->GetBufferPointer(), p_root_signature_blob->GetBufferSize(), IID_PPV_ARGS(&p_root_signature));
	return std::make_pair(std::move(p_root_signature), pipeline_resource_count);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace d3d12

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

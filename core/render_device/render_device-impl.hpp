
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//delay_delete
///////////////////////////////////////////////////////////////////////////////////////////////////

//コンストラクタ
inline delay_delete::delay_delete() : m_ref_count(1), m_immediate_delete()
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//デストラクタ
inline delay_delete::~delay_delete()
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//参照カウント増分
inline int intrusive_ptr_add_ref(delay_delete *p)
{
	return ++p->m_ref_count;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//参照カウント減分
inline int intrusive_ptr_release(delay_delete *p)
{
	const int ret = --p->m_ref_count;
	if(ret == 0)
	{
		if(p->m_immediate_delete)
			delete p;
		else
			g_delay_deleter.delay_delete(p);
	}
	return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//delay_deleter
///////////////////////////////////////////////////////////////////////////////////////////////////

//コンストラクタ
inline delay_deleter::delay_deleter(const uint protect_count) : m_counter(), m_protect_count(protect_count)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//破棄
inline void delay_deleter::delay_delete()
{
	while(m_records.size() && (m_records.front().counter + m_protect_count <= m_counter))
	{
		delete m_records.front().ptr;
		m_records.pop();
	}
	m_counter++;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

inline void delay_deleter::delay_delete(render::delay_delete *ptr)
{
	m_records.push({ m_counter, ptr });
}

///////////////////////////////////////////////////////////////////////////////////////////////////

inline void delay_deleter::force_delete()
{
	while(m_records.size())
	{
		delete m_records.front().ptr;
		m_records.pop();
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//target_state
///////////////////////////////////////////////////////////////////////////////////////////////////

//コンストラクタ
inline target_state::target_state(const uint num_rtvs, render_target_view* rtv_ptrs[], depth_stencil_view* p_dsv) : m_num_rtvs(num_rtvs), mp_dsv(p_dsv)
{
	for(uint i = 0; i < num_rtvs; i++){ m_rtv_ptrs[i] = rtv_ptrs[i]; }
	for(uint i = num_rtvs; i < 8; i++){ m_rtv_ptrs[i] = nullptr; }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//target_state
///////////////////////////////////////////////////////////////////////////////////////////////////

//コンストラクタ
inline geometry_state::geometry_state(const uint num_vbs, buffer *vb_ptrs[8], uint offsets[8], uint strides[8], buffer *p_ib) : m_num_vbs(num_vbs), mp_ib(p_ib)
{
	for(uint i = 0; i < num_vbs; i++){ m_vb_ptrs[i] = vb_ptrs[i]; }
	for(uint i = 0; i < num_vbs; i++){ m_offsets[i] = offsets[i]; }
	for(uint i = 0; i < num_vbs; i++){ m_strides[i] = strides[i]; }
	for(uint i = num_vbs; i < 8; i++){ m_vb_ptrs[i] = nullptr; }
	for(uint i = num_vbs; i < 8; i++){ m_offsets[i] = 0; }
	for(uint i = num_vbs; i < 8; i++){ m_strides[i] = 0; }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//texture_srv_desc
///////////////////////////////////////////////////////////////////////////////////////////////////

//コンストラクタ
inline texture_srv_desc::texture_srv_desc(const texture& tex, const srv_flag flag)
{
	this->mip_levels = tex.mip_levels();
	this->most_detailed_mip = 0;
	this->array_size = tex.array_size();
	this->first_array_index = 0;
	this->flag = flag;
}

inline texture_srv_desc::texture_srv_desc(const texture& tex, const uint most_detailed_mip, const uint mip_levels, const srv_flag flag)
{
	this->mip_levels = std::min(mip_levels, tex.mip_levels() - most_detailed_mip);
	this->most_detailed_mip = most_detailed_mip;
	this->array_size = tex.array_size();
	this->first_array_index = 0;
	this->flag = flag;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//texture_uav_desc
///////////////////////////////////////////////////////////////////////////////////////////////////

//コンストラクタ
inline texture_uav_desc::texture_uav_desc(const texture& tex, const uint mip_slice)
{
	this->mip_slice = mip_slice;
	this->array_size = tex.array_size();
	this->first_array_index = 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//texture_rtv_desc
///////////////////////////////////////////////////////////////////////////////////////////////////

//コンストラクタ
inline texture_rtv_desc::texture_rtv_desc(const texture& tex, const uint mip_slice)
{
	this->mip_slice = mip_slice;
	this->array_size = tex.array_size();
	this->first_array_index = 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//texture_dsv_desc
///////////////////////////////////////////////////////////////////////////////////////////////////

//コンストラクタ
inline texture_dsv_desc::texture_dsv_desc(const texture& tex, const uint mip_slice, const dsv_flags flags)
{
	this->mip_slice = mip_slice;
	this->array_size = tex.array_size();
	this->first_array_index = 0;
	this->flags = flags;
}

inline texture_dsv_desc::texture_dsv_desc(const texture& tex, const dsv_flags flags, const uint mip_slice)
{
	this->mip_slice = mip_slice;
	this->array_size = tex.array_size();
	this->first_array_index = 0;
	this->flags = flags;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//buffer_srv_desc
///////////////////////////////////////////////////////////////////////////////////////////////////

//コンストラクタ
inline buffer_srv_desc::buffer_srv_desc(const buffer& buf, const uint first_element, const uint num_elements, const bool force_byteaddress)
{
	if(this->force_byteaddress = force_byteaddress)
	{
		const uint first_byte = buf.stride() * first_element;
		const uint num_bytes = buf.stride() * std::min(num_elements, buf.num_elements() - first_element);
		//assert(first_byte % 4 == 0);
		//assert(num_bytes % 4 == 0);
		this->first_element = ceil_div(first_byte, 4);
		this->num_elements = ceil_div(num_bytes, 4);
	}
	else
	{
		this->first_element = first_element;
		this->num_elements = std::min(num_elements, buf.num_elements() - first_element);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//buffer_uav_desc
///////////////////////////////////////////////////////////////////////////////////////////////////

//コンストラクタ
inline buffer_uav_desc::buffer_uav_desc(const buffer& buf, const uint first_element, const uint num_elements)
{
	this->first_element = first_element;
	this->num_elements = std::min(num_elements, buf.num_elements() - first_element);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//viewport
///////////////////////////////////////////////////////////////////////////////////////////////////

//コンストラクタ
inline viewport::viewport(const uint2& left_top, const uint2& size) : left_top(left_top), size(size)
{
}

inline viewport::viewport(const uint left, const uint top, const uint width, const uint height) : left_top(left, top), size(width, height)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace iface{

///////////////////////////////////////////////////////////////////////////////////////////////////
//render_device
///////////////////////////////////////////////////////////////////////////////////////////////////

//コンストラクタ
inline render_device::render_device(const uint command_buffer_size) : m_command_manager(command_buffer_size)
{
	m_frame_count = 1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//実行
inline void render_device::present(swapchain &swapchain)
{
	present_impl(swapchain);
	m_frame_count++;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//バインドレスハンドルを設定
inline void render_device::set_bindless_handle(bindless_resource &res, const uint handle)
{
	res.m_bindless_handle = handle;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace iface

///////////////////////////////////////////////////////////////////////////////////////////////////
//関数定義
///////////////////////////////////////////////////////////////////////////////////////////////////

//1テクセルのバイト数を返す
inline uint texel_size(const texture_format format)
{
	switch(format)
	{
	case texture_format_r32g32b32a32_float:
	case texture_format_r32g32b32a32_uint:
	case texture_format_r32g32b32a32_sint:
		return 16;
	case texture_format_r32g32b32_float:
	case texture_format_r32g32b32_uint:
	case texture_format_r32g32b32_sint:
		return 12;
	case texture_format_r16g16b16a16_float:
	case texture_format_r16g16b16a16_unorm:
	case texture_format_r16g16b16a16_uint:
	case texture_format_r16g16b16a16_snorm:
	case texture_format_r16g16b16a16_sint:
	case texture_format_r32g32_float:
	case texture_format_r32g32_uint:
	case texture_format_r32g32_sint:
		return 8;
	case texture_format_r10g10b10a2_unorm:
	case texture_format_r10g10b10a2_uint:
	case texture_format_r11g11b10_float:
	case texture_format_r8g8b8a8_unorm:
	case texture_format_r8g8b8a8_unorm_srgb:
	case texture_format_r8g8b8a8_uint:
	case texture_format_r8g8b8a8_snorm:
	case texture_format_r8g8b8a8_sint:
	case texture_format_r16g16_float:
	case texture_format_r16g16_unorm:
	case texture_format_r16g16_uint:
	case texture_format_r16g16_snorm:
	case texture_format_r16g16_sint:
	case texture_format_d32_float:
	case texture_format_r32_float:
	case texture_format_r32_uint:
	case texture_format_r32_sint:
	case texture_format_r9g9b9e5_sharedexp:
		return 4;
	case texture_format_r8g8_unorm:
	case texture_format_r8g8_uint:
	case texture_format_r8g8_snorm:
	case texture_format_r8g8_sint:
	case texture_format_r16_float:
	case texture_format_d16_unorm:
	case texture_format_r16_unorm:
	case texture_format_r16_uint:
	case texture_format_r16_snorm:
	case texture_format_r16_sint:
		return 2;
	case texture_format_r8_unorm:
	case texture_format_r8_uint:
	case texture_format_r8_snorm:
	case texture_format_r8_sint:
	case texture_format_a8_unorm:
		return 1;
	case texture_format_d32_float_s8x24_uint:
	case texture_format_d24_unorm_s8_uint:
	default:
		throw;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

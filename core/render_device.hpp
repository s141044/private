
#pragma once

#ifndef NN_RENDER_CORE_RENDER_DEVICE_HPP
#define NN_RENDER_CORE_RENDER_DEVICE_HPP

#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX

#include"utility.hpp"
#include"command.hpp"
#include"allocator.hpp"
#include"intrusive_ptr.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef SHADER_DIR
#define SHADER_DIR L"./shader"
#endif
static inline const wstring shader_src_dir = SHADER_DIR L"/src/";
static inline const wstring shader_bin_dir = SHADER_DIR L"/bin/";
static inline const wstring shader_pdb_dir = SHADER_DIR L"/pdb/";
static inline const wstring shader_file_ext[] = { L".hlsl", L".hlsli", };

///////////////////////////////////////////////////////////////////////////////////////////////////

static inline const uint invalid_bindless_handle = 0xffffffff;

///////////////////////////////////////////////////////////////////////////////////////////////////
//delay_delete
/*/////////////////////////////////////////////////////////////////////////////////////////////////
intrusive_ptr�ƃZ�b�g�Ŏg��.
/////////////////////////////////////////////////////////////////////////////////////////////////*/

class delay_delete : public immovable<delay_delete>
{
public:

	//�R���X�g���N�^
	delay_delete();

	//�f�X�g���N�^
	virtual ~delay_delete();

	//�����폜��L����
	void enable_immediate_delete(){ m_immediate_delete = true; }

	//�Q�ƃJ�E���g����
	friend int intrusive_ptr_add_ref(delay_delete *p);
	friend int intrusive_ptr_release(delay_delete *p);

private:

	std::atomic_int	m_ref_count;
	bool			m_immediate_delete;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//delay_deleter
///////////////////////////////////////////////////////////////////////////////////////////////////

class delay_deleter
{
public:

	//�R���X�g���N�^
	delay_deleter(const uint protect_count);

	//�j��
	void delay_delete();
	void delay_delete(render::delay_delete *ptr);
	void force_delete();

private:

	struct record{
		uint					counter;
		render::delay_delete*	ptr;
	};

	queue<record>		m_records;
	uint				m_counter;
	uint				m_protect_count;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

static inline delay_deleter g_delay_deleter(3);

///////////////////////////////////////////////////////////////////////////////////////////////////
//resource_flags
///////////////////////////////////////////////////////////////////////////////////////////////////

enum resource_flags
{
	resource_flag_none									= 0x0,
	resource_flag_allow_depth_stencil					= 0x1,
	resource_flag_allow_render_target					= 0x2,
	resource_flag_allow_unordered_access				= 0x4,
	resource_flag_allow_shader_resource					= 0x8,
	resource_flag_allow_indirect_argument				= 0x10,
	resource_flag_raytracing_acceleration_structure		= 0x20,
	resource_flag_scratch								= 0x40,
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//buffer
/*/////////////////////////////////////////////////////////////////////////////////////////////////
byteaddress_buffer��stride��4.
/////////////////////////////////////////////////////////////////////////////////////////////////*/

class buffer : public delay_delete
{
public:

	//�R���X�g���N�^
	buffer(const uint stride, const uint num_elements) : m_stride(stride), m_num_elements(num_elements){}

	//�f�X�g���N�^
	virtual ~buffer(){}

	//�T�C�Y��Ԃ�
	uint size() const { return m_stride * m_num_elements; }

	//�X�g���C�h/�v�f����Ԃ�
	uint stride() const { return m_stride; }
	uint num_elements() const { return m_num_elements; }

private:

	uint m_stride;
	uint m_num_elements;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//bindless_resource
///////////////////////////////////////////////////////////////////////////////////////////////////

class bindless_resource
{
public:

	//�f�X�g���N�^
	virtual ~bindless_resource(){ assert(m_bindless_handle == invalid_bindless_handle); }

	//�o�C���h���X�n���h����Ԃ�
	uint bindless_handle() const { return m_bindless_handle; }

private:

	friend class iface::render_device;
	uint m_bindless_handle = invalid_bindless_handle;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//constant_buffer
/*/////////////////////////////////////////////////////////////////////////////////////////////////
������/��������̂�1�x�����f�[�^�̍X�V���\
/////////////////////////////////////////////////////////////////////////////////////////////////*/

class constant_buffer : public delay_delete, public bindless_resource
{
public:	

	//�R���X�g���N�^
	constant_buffer(const uint size) : m_size(size){}

	//�f�X�g���N�^
	virtual ~constant_buffer(){}

	//�T�C�Y��Ԃ�
	uint size() const { return m_size; }

	//�X�V�p�A�h���X��Ԃ�
	template<class T = void> T *data(){ return static_cast<T*>(m_data); }

protected:

	void *m_data;

private:

	uint m_size;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//readback_buffer
///////////////////////////////////////////////////////////////////////////////////////////////////

class readback_buffer : public delay_delete
{
public:

	//�R���X�g���N�^
	readback_buffer(const uint size) : m_size(size){}

	//�f�X�g���N�^
	virtual ~readback_buffer(){}

	//�T�C�Y��Ԃ�
	uint size() const { return m_size; }

	//�ǂݎ��p�A�h���X��Ԃ�
	template<class T = void> const T *data() const { return static_cast<const T*>(m_data); }

protected:

	void *m_data;

private:

	uint m_size;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//upload_buffer
///////////////////////////////////////////////////////////////////////////////////////////////////

class upload_buffer : public delay_delete
{
public:	

	//�R���X�g���N�^
	upload_buffer(const uint size) : m_size(size){}

	//�f�X�g���N�^
	virtual ~upload_buffer(){}

	//�T�C�Y��Ԃ�
	uint size() const { return m_size; }

	//�X�V�p�A�h���X��Ԃ�
	template<class T = void> T *data(){ return static_cast<T*>(m_data); }

protected:

	void *m_data;

private:

	uint m_size;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//texture_format
///////////////////////////////////////////////////////////////////////////////////////////////////

enum texture_format : uint8_t
{
	texture_format_r32g32b32a32_float = 2,
	texture_format_r32g32b32a32_uint = 3,
	texture_format_r32g32b32a32_sint = 4,
	texture_format_r32g32b32_float = 6,
	texture_format_r32g32b32_uint = 7,
	texture_format_r32g32b32_sint = 8,
	texture_format_r16g16b16a16_float = 10,
	texture_format_r16g16b16a16_unorm = 11,
	texture_format_r16g16b16a16_uint = 12,
	texture_format_r16g16b16a16_snorm = 13,
	texture_format_r16g16b16a16_sint = 14,
	texture_format_r32g32_float = 16,
	texture_format_r32g32_uint = 17,
	texture_format_r32g32_sint = 18,
	texture_format_d32_float_s8x24_uint = 20,
	texture_format_r10g10b10a2_unorm = 24,
	texture_format_r10g10b10a2_uint = 25,
	texture_format_r11g11b10_float = 26,
	texture_format_r8g8b8a8_unorm = 28,
	texture_format_r8g8b8a8_unorm_srgb = 29,
	texture_format_r8g8b8a8_uint = 30,
	texture_format_r8g8b8a8_snorm = 31,
	texture_format_r8g8b8a8_sint = 32,
	texture_format_r16g16_float = 34,
	texture_format_r16g16_unorm = 35,
	texture_format_r16g16_uint = 36,
	texture_format_r16g16_snorm = 37,
	texture_format_r16g16_sint = 38,
	texture_format_d32_float = 40,
	texture_format_r32_float = 41,
	texture_format_r32_uint = 42,
	texture_format_r32_sint = 43,
	texture_format_d24_unorm_s8_uint = 45,
	texture_format_r8g8_unorm = 49,
	texture_format_r8g8_uint = 50,
	texture_format_r8g8_snorm = 51,
	texture_format_r8g8_sint = 52,
	texture_format_r16_float = 54,
	texture_format_d16_unorm = 55,
	texture_format_r16_unorm = 56,
	texture_format_r16_uint = 57,
	texture_format_r16_snorm = 58,
	texture_format_r16_sint = 59,
	texture_format_r8_unorm = 61,
	texture_format_r8_uint = 62,
	texture_format_r8_snorm = 63,
	texture_format_r8_sint = 64,
	texture_format_a8_unorm = 65,
	texture_format_r9g9b9e5_sharedexp = 67,
	texture_format_count,
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//texture
///////////////////////////////////////////////////////////////////////////////////////////////////

class texture : public delay_delete
{
public:

	//�R���X�g���N�^
	texture(const texture_format format, const uint width, const uint height, const uint depth, const uint mip_levels, const bool is_3d) : m_format(format), m_width(width), m_height(height), m_depth(depth), m_is_3d(is_3d), m_mip_levels(mip_levels){}

	//�f�X�g���N�^
	virtual ~texture(){}

	//�e�N�X�`������Ԃ�
	texture_format format() const { return m_format; }
	uint width() const { return m_width; }
	uint height() const { return m_height; }
	uint depth() const { return m_is_3d ? m_depth : 1; }
	uint array_size() const { return m_is_3d ? 1 : m_depth; }
	uint mip_levels() const { return m_mip_levels; }

private:

	texture_format	m_format;
	uint			m_width;
	uint			m_height;
	uint			m_depth : 31;
	uint			m_is_3d : 1;
	uint			m_mip_levels;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//raytracing_instance_flags
///////////////////////////////////////////////////////////////////////////////////////////////////

enum raytracing_instance_flags : uint
{
	raytracing_instance_flag_none					= 0x0,
	raytracing_instance_flag_triangle_cull_disable	= 0x1,
	raytracing_instance_flag_triangle_front_ccw		= 0x2,
	raytracing_instance_flag_force_opaque			= 0x4,
	raytracing_instance_flag_force_non_opaque		= 0x8,
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//raytracing_instance_mask
///////////////////////////////////////////////////////////////////////////////////////////////////

enum raytracing_instance_mask : uint
{
	raytracing_instance_mask_default	= 0x1,
	raytracing_instance_mask_subsurface	= 0x2,
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//raytracing_instance_desc
/*/////////////////////////////////////////////////////////////////////////////////////////////////
transform�͍s�D��.�]�u�͕s�v.
/////////////////////////////////////////////////////////////////////////////////////////////////*/

struct raytracing_instance_desc
{
	float		transform[3][4];
	uint		id		: 24;
	uint		mask	: 8;
	uint		unused	: 24;
	uint		flags	: 8;
	uint64_t	blas_address;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//raytracing_geometry_desc
/*/////////////////////////////////////////////////////////////////////////////////////////////////
vertex_count��(index�̍ő�l+1)�ȏ�̒l
/////////////////////////////////////////////////////////////////////////////////////////////////*/

struct raytracing_geometry_desc
{
	uint		vertex_count;
	uint		index_count;
	uint64_t	start_index_location;
	uint64_t	base_vertex_location;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//raytracing_compaction_state
/*/////////////////////////////////////////////////////////////////////////////////////////////////
ready�ɂȂ��Ă����completed�ɂȂ�܂�comapction���߂��o��������.
state-progress0�Ői���x��������.
/////////////////////////////////////////////////////////////////////////////////////////////////*/

enum raytracing_compaction_state
{
	raytracing_compaction_state_not_support, 
	raytracing_compaction_state_completed, 
	raytracing_compaction_state_not_ready, 
	raytracing_compaction_state_ready, 
	raytracing_compaction_state_progress0, 
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//top_level_acceleration_structure
/*/////////////////////////////////////////////////////////////////////////////////////////////////
update_buffer��instance_descs�̃o�b�t�@���X�V���Ă���
/////////////////////////////////////////////////////////////////////////////////////////////////*/

class top_level_acceleration_structure : public delay_delete
{
public:

	//�f�X�g���N�^
	virtual ~top_level_acceleration_structure(){}

	//�C���X�^���X����Ԃ�
	buffer& instance_descs() const { return *mp_instance_descs; }

protected:

	buffer* mp_instance_descs;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//bottom_level_acceleration_structure
/*/////////////////////////////////////////////////////////////////////////////////////////////////
Float3�̒��_�f�[�^.U16�̃C���f�b�N�X�f�[�^.
/////////////////////////////////////////////////////////////////////////////////////////////////*/

class bottom_level_acceleration_structure : public delay_delete
{
public:

	//�R���X�g���N�^
	bottom_level_acceleration_structure() : m_compaction_state(raytracing_compaction_state_not_support), m_gpu_virtual_address(-1){}

	//�f�X�g���N�^
	virtual ~bottom_level_acceleration_structure(){}

	//�R���p�N�V�����̏�Ԃ�Ԃ�
	raytracing_compaction_state compaction_state() const { return m_compaction_state; }

	//���z�A�h���X��Ԃ�
	uint64_t gpu_virtual_address() const { return m_gpu_virtual_address; }

protected:

	uint64_t					m_gpu_virtual_address;
	raytracing_compaction_state	m_compaction_state;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//address_mode
///////////////////////////////////////////////////////////////////////////////////////////////////

enum address_mode
{
	address_mode_wrap	= 1,
	address_mode_mirror	= 2,
	address_mode_clamp	= 3,
	address_mode_border	= 4,
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//filter_type
///////////////////////////////////////////////////////////////////////////////////////////////////

enum filter_type
{
	filter_type_min_mag_mip_point				= 0x0,
	filter_type_min_mag_point_mip_linear		= 0x1,
	filter_type_min_point_mag_linear_mip_point	= 0x4,
	filter_type_min_point_mag_mip_linear		= 0x5,
	filter_type_min_linear_mag_mip_point		= 0x10,
	filter_type_min_linear_mag_point_mip_linear	= 0x11,
	filter_type_min_mag_linear_mip_point		= 0x14,
	filter_type_min_mag_mip_linear				= 0x15,
	filter_type_min_mag_anisotropic_mip_point	= 0x54,
	filter_type_anisotropic						= 0x55,
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//sampler_desc
///////////////////////////////////////////////////////////////////////////////////////////////////

struct sampler_desc
{
	sampler_desc()
	{
		filter = filter_type_min_mag_mip_linear;
		address_u = address_mode_wrap;
		address_v = address_mode_wrap;
		address_w = address_mode_wrap;
		max_anisotropy = 16;
		max_lod = FLT_MAX;
		min_lod = 0;
		lod_bias = 0;
		border_color = 0;
	}

	filter_type		filter;
	address_mode	address_u;
	address_mode	address_v;
	address_mode	address_w;
	uint			max_anisotropy;
	float			max_lod;
	float			min_lod;
	float			lod_bias;
	float4			border_color;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//sampler
///////////////////////////////////////////////////////////////////////////////////////////////////

class sampler : public delay_delete, public bindless_resource
{
public:

	//�f�X�g���N�^
	virtual ~sampler(){}
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//shader_resource_view
///////////////////////////////////////////////////////////////////////////////////////////////////

class shader_resource_view : public delay_delete, public bindless_resource
{
public:

	//�f�X�g���N�^
	virtual ~shader_resource_view(){}
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//unordered_access_view
///////////////////////////////////////////////////////////////////////////////////////////////////

class unordered_access_view : public delay_delete
{
public:

	//�f�X�g���N�^
	virtual ~unordered_access_view(){}
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//render_target_view
///////////////////////////////////////////////////////////////////////////////////////////////////

class render_target_view : public delay_delete
{
public:

	//�f�X�g���N�^
	virtual ~render_target_view(){}
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//depth_stencil_view
///////////////////////////////////////////////////////////////////////////////////////////////////

class depth_stencil_view : public delay_delete
{
public:

	//�f�X�g���N�^
	virtual ~depth_stencil_view(){}
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//target_state
///////////////////////////////////////////////////////////////////////////////////////////////////

class target_state : public delay_delete
{
public:

	//�R���X�g���N�^
	target_state(const uint num_rtvs, render_target_view* rtv_ptrs[], depth_stencil_view* p_dsv);

	//�f�X�g���N�^
	virtual ~target_state(){}

	//RTV����Ԃ�
	uint num_rtvs() const { return m_num_rtvs; }

	//DSV������
	bool has_dsv() const { return mp_dsv != nullptr; }

	//RTV/DSV��Ԃ�
	render_target_view& rtv(const uint i) const { return *m_rtv_ptrs[i]; }
	depth_stencil_view& dsv() const { return *mp_dsv; }

private:

	uint				m_num_rtvs;
	render_target_view*	m_rtv_ptrs[8];
	depth_stencil_view*	mp_dsv;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//geometry_state
/*/////////////////////////////////////////////////////////////////////////////////////////////////
���_�o�b�t�@�͋��ʂ̂��̂��g�p���Ă��悢���C�C���f�b�N�X�o�b�t�@�͓Ɨ��������̂��g�p����D
�C���f�b�N�X�o�b�t�@��stride�ŃC���f�b�N�X�̃T�C�Y��\��.
/////////////////////////////////////////////////////////////////////////////////////////////////*/

class geometry_state : public delay_delete
{
public:

	//�R���X�g���N�^
	geometry_state(const uint num_vbs, buffer *vb_ptrs[8], uint offsets[8], uint strides[8], buffer *p_ib);

	//�f�X�g���N�^
	virtual ~geometry_state(){}

	//���_�o�b�t�@����Ԃ�
	uint num_vbs() const { return m_num_vbs; }

	//�C���f�b�N�X�o�b�t�@������
	bool has_index_buffer() const { return (mp_ib != nullptr); }

	//���_/�C���f�b�N�X�o�b�t�@��Ԃ�
	buffer& vertex_buffer(const uint i) const { return *m_vb_ptrs[i]; }
	buffer& index_buffer() const { return *mp_ib; }

	//���_�f�[�^�̃I�t�Z�b�g/�X�g���C�h��Ԃ�
	uint offset(const uint i) const { return m_offsets[i]; }
	uint stride(const uint i) const { return m_strides[i]; }

private:

	uint	m_num_vbs;
	uint	m_offsets[8];
	uint	m_strides[8];
	buffer*	m_vb_ptrs[8];
	buffer*	mp_ib;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//pipeline_state
///////////////////////////////////////////////////////////////////////////////////////////////////

class pipeline_state : public delay_delete
{
public:

	//�R���X�g���N�^
	pipeline_state(const string &name) : m_name(name){}

	//�f�X�g���N�^
	virtual ~pipeline_state(){}

	//���O��Ԃ�
	const string& name() const { return m_name; }

private:

	string m_name;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//shader
///////////////////////////////////////////////////////////////////////////////////////////////////

class shader : public delay_delete
{
public:

	//�f�X�g���N�^
	virtual ~shader(){}
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//blend_op
///////////////////////////////////////////////////////////////////////////////////////////////////

enum blend_op : uint8_t
{
	blend_op_add			= 1, 
	blend_op_subtract		= 2, 
	blend_op_rev_subtract	= 3, 
	blend_op_min			= 4,
	blend_op_max			= 5,
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//blend_factor
/*/////////////////////////////////////////////////////////////////////////////////////////////////
���ɂ�����͕̂K�v�ɂȂ�����ǉ�
/////////////////////////////////////////////////////////////////////////////////////////////////*/

enum blend_factor : uint8_t
{
	blend_factor_zero			= 1, 
	blend_factor_one			= 2,
	blend_factor_src_color		= 3, 
	blend_factor_inv_src_color	= 4,
	blend_factor_src_alpha		= 5,
	blend_factor_inv_src_alpha	= 6,
	blend_factor_dst_alpha		= 7,
	blend_factor_inv_dst_alpha	= 8,
	blend_factor_dst_color		= 9, 
	blend_factor_inv_dst_color	= 10,
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//render_target_write_mask
///////////////////////////////////////////////////////////////////////////////////////////////////

enum render_target_write_mask : uint8_t
{
	render_target_write_mask_none	= 0b0000, 
	render_target_write_mask_red	= 0b0001, 
	render_target_write_mask_green	= 0b0010, 
	render_target_write_mask_blue	= 0b0100, 
	render_target_write_mask_alpha	= 0b1000, 
	render_target_write_mask_all	= 0b1111, 
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//render_target_blend_desc
///////////////////////////////////////////////////////////////////////////////////////////////////

struct render_target_blend_desc
{
	bool						enable = false;
	blend_op					op = blend_op_add;
	blend_factor				src = blend_factor_one;
	blend_factor				dst = blend_factor_zero;
	blend_op					op_alpha = blend_op_add;
	blend_factor				src_alpha = blend_factor_one;
	blend_factor				dst_alpha = blend_factor_zero;
	render_target_write_mask	write_mask = render_target_write_mask_all;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//blend_desc
/*/////////////////////////////////////////////////////////////////////////////////////////////////
output = src*srcBlendFactor op dst*dstBlendFacotr
/////////////////////////////////////////////////////////////////////////////////////////////////*/

struct blend_desc
{
	bool						independent_blend = false;
	render_target_blend_desc	render_target[8];
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//comparison_func
///////////////////////////////////////////////////////////////////////////////////////////////////

enum comparison_func : uint8_t
{
	comparison_func_never			= 1, 
	comparison_func_less			= 2, 
	comparison_func_equal			= 3, 
	comparison_func_not_equal		= 6, 
	comparison_func_less_equal		= 4, 
	comparison_func_greater			= 5, 
	comparison_func_greater_equal	= 7, 
	comparison_func_always			= 8, 
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//stencil_op
///////////////////////////////////////////////////////////////////////////////////////////////////

enum stencil_op : uint8_t
{
	stencil_op_keep		= 1, 
	stencil_op_zero		= 2, 
	stencil_op_invert	= 6, 
	stencil_op_replace	= 3, 
	stencil_op_inc_sat	= 4, 
	stencil_op_dec_sat	= 5, 
	stencil_op_inc		= 7, 
	stencil_op_dec		= 8, 
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//stencil_op_desc
///////////////////////////////////////////////////////////////////////////////////////////////////

struct stencil_op_desc
{
	stencil_op		stencil_fail_op	= stencil_op_keep;
	stencil_op		depth_fail_op	= stencil_op_keep;
	stencil_op		pass_op			= stencil_op_keep;
	comparison_func	func			= comparison_func_always;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//depth_stencil_desc
///////////////////////////////////////////////////////////////////////////////////////////////////

struct depth_stencil_desc
{
	bool				depth_enabale		= true;
	bool				depth_write			= true;
	comparison_func		depth_func			= comparison_func_less;
	bool				stencil_enable		= false;
	uint8_t				stencil_read_mask	= 0xff;
	uint8_t				stencil_write_mask	= 0xff;
	stencil_op_desc		front;
	stencil_op_desc		back;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//cull_mode
///////////////////////////////////////////////////////////////////////////////////////////////////

enum cull_mode : uint8_t
{
	cull_mode_none	= 1, 
	cull_mode_front	= 2, 
	cull_mode_back	= 3, 
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//rasterizer_desc
///////////////////////////////////////////////////////////////////////////////////////////////////

struct rasterizer_desc
{
	bool		fill					= true;
	cull_mode	cull_mode				= cull_mode_back;
	int			depth_bias				= 0;
	float		depth_bias_clamp		= 0;
	float		slope_scaled_depth_bias	= 0;
	bool		conservative			= false;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//input_element_desc
/*/////////////////////////////////////////////////////////////////////////////////////////////////
offset�͒��_�f�[�^�̐擪����Y���v�f������ʒu�܂ł̃I�t�Z�b�g
/////////////////////////////////////////////////////////////////////////////////////////////////*/

struct input_element_desc
{
	std::string		semantic_name;
	uint8_t			semantic_index = 0;
	texture_format	format;
	uint8_t			slot;
	uint8_t			offset = 0;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//topology_type
///////////////////////////////////////////////////////////////////////////////////////////////////

enum topology_type
{
	topology_type_undefined = 0,
	topology_type_point_list = 1,
	topology_type_line_list = 2,
	topology_type_line_strip = 3,
	topology_type_triangle_list = 4,
	topology_type_triangle_strip = 5,
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//input_layout_desc
///////////////////////////////////////////////////////////////////////////////////////////////////

struct input_layout_desc
{
	topology_type		topology;
	uint				num_elements;
	input_element_desc	element_descs[8];
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//shader_compiler
///////////////////////////////////////////////////////////////////////////////////////////////////

class shader_compiler
{
public:

	//�f�X�g���N�^
	virtual ~shader_compiler(){}

	//�R���p�C��
	virtual shader_ptr compile(const wstring& filename, const wstring& entry_point, const wstring& target, const wstring& options, unordered_set<wstring>& dependent) = 0;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//srv_flag
///////////////////////////////////////////////////////////////////////////////////////////////////

enum srv_flag
{
	srv_flag_none			= 0,
	srv_flag_depth			= 1,
	srv_flag_stencil		= 2,
	srv_flag_cube_as_array	= 4,
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//texture_srv_desc
///////////////////////////////////////////////////////////////////////////////////////////////////

struct texture_srv_desc
{
	texture_srv_desc(const texture& tex, const srv_flag flag = srv_flag_none);
	texture_srv_desc(const texture& tex, const uint most_detailed_mip, const uint mip_levels = 1, const srv_flag flag = srv_flag_none);

	uint array_size;
	uint first_array_index;
	uint mip_levels;
	uint most_detailed_mip;
	srv_flag flag;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//texture_uav_desc
///////////////////////////////////////////////////////////////////////////////////////////////////

struct texture_uav_desc
{
	texture_uav_desc(const texture &tex, const uint mip_slice = 0);

	uint mip_slice;
	uint array_size;
	uint first_array_index;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//texture_rtv_desc
/*/////////////////////////////////////////////////////////////////////////////////////////////////
array�̂ǂ̃X���C�X�ɏo�͂��邩�����߂���@������(SV_RenderTargetArrayIndex)
/////////////////////////////////////////////////////////////////////////////////////////////////*/

struct texture_rtv_desc
{
	texture_rtv_desc(const texture& tex, const uint mip_slice = 0);

	uint mip_slice;
	uint array_size;
	uint first_array_index;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//dsv_flags
///////////////////////////////////////////////////////////////////////////////////////////////////

enum dsv_flags
{
	dsv_flag_none,
	dsv_flag_readonly_depth,
	dsv_flag_readonly_stencil,
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//texture_dsv_desc
/*/////////////////////////////////////////////////////////////////////////////////////////////////
array�̂ǂ̃X���C�X�ɏo�͂��邩�����߂���@������(SV_RenderTargetArrayIndex)
/////////////////////////////////////////////////////////////////////////////////////////////////*/

struct texture_dsv_desc
{
	texture_dsv_desc(const texture& tex, const uint mip_slice, const dsv_flags flags = dsv_flag_none);
	texture_dsv_desc(const texture& tex, const dsv_flags flags = dsv_flag_none, const uint mip_slice = 0);

	uint mip_slice;
	uint array_size;
	uint first_array_index;
	dsv_flags flags;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//buffer_srv_desc
/*/////////////////////////////////////////////////////////////////////////////////////////////////
bindless�Ŏg���Ƃ���byteaddress�Ƃ���
/////////////////////////////////////////////////////////////////////////////////////////////////*/

struct buffer_srv_desc
{
	buffer_srv_desc(const buffer& buf, const uint first_element = 0, const uint num_elements = -1, const bool force_byteaddress = false);

	uint num_elements;
	uint first_element;
	bool force_byteaddress;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//buffer_uav_desc
///////////////////////////////////////////////////////////////////////////////////////////////////

struct buffer_uav_desc
{
	buffer_uav_desc(const buffer& buf, const uint first_element = 0, const uint num_elements = -1);

	uint num_elements;
	uint first_element;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//swapchain
/*/////////////////////////////////////////////////////////////////////////////////////////////////
resize������target���擾���Ȃ���. �o�b�N�o�b�t�@�̃C���f�b�N�X���擾���Ȃ���.
/////////////////////////////////////////////////////////////////////////////////////////////////*/

class swapchain : public delay_delete
{
public:

	//�f�X�g���N�^
	virtual ~swapchain(){}

	//�����_�[�^�[�Q�b�g�e�N�X�`����Ԃ�
	texture& back_buffer(const uint i) const { return *m_back_buffer_ptrs[i]; }

	//�o�b�N�o�b�t�@�̃C���f�b�N�X��Ԃ�
	virtual uint back_buffer_index() const = 0;

	//���T�C�Y
	virtual void resize(const uint2 size) = 0;
	
	//�t���X�N���[���̐؂�ւ�
	virtual void toggle_fullscreen(const bool fullscreen) = 0;

	//�o�b�t�@����Ԃ�
	static constexpr uint buffer_count(){ return _countof(m_back_buffer_ptrs); }

protected:

	texture_ptr	m_back_buffer_ptrs[2];
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//bind_resource
///////////////////////////////////////////////////////////////////////////////////////////////////

class bind_resource
{
public:

	//�R���X�g���N�^
	bind_resource() : m_type(-1), m_timestamp(-1), mp_resource(){}
	bind_resource(sampler& sampler, const uint timestamp) : m_type(0), m_timestamp(timestamp), mp_resource(&sampler){}
	bind_resource(constant_buffer& cbv, const uint timestamp) : m_type(1), m_timestamp(timestamp), mp_resource(&cbv){}
	bind_resource(shader_resource_view& srv, const uint timestamp) : m_type(2), m_timestamp(timestamp), mp_resource(&srv){}
	bind_resource(unordered_access_view& uav, const uint timestamp) : m_type(3), m_timestamp(timestamp), mp_resource(&uav){}
	bind_resource(top_level_acceleration_structure& tlas, const uint timestamp) : m_type(4), m_timestamp(timestamp), mp_resource(&tlas){}

	//View��Ԃ�
	sampler& sampler() const { return assert(m_type == 0), *static_cast<render::sampler*>(mp_resource); }
	constant_buffer& cbv() const { return assert(m_type == 1), *static_cast<constant_buffer*>(mp_resource); }
	shader_resource_view& srv() const { return assert(m_type == 2), *static_cast<shader_resource_view*>(mp_resource); }
	unordered_access_view& uav() const { return assert(m_type == 3), *static_cast<unordered_access_view*>(mp_resource); }
	top_level_acceleration_structure& tlas() const { return assert(m_type == 4), *static_cast<top_level_acceleration_structure*>(mp_resource); }

	//�T���v��/cbv/srv/uav��
	bool is_sampler() const { return m_type == 0; }
	bool is_cbv() const { return m_type == 1; }
	bool is_srv() const { return m_type == 2; }
	bool is_uav() const { return m_type == 3; }
	bool is_tlas() const { return m_type == 4; }

	//�^�C���X�^���v��Ԃ�
	uint timestamp() const { return m_timestamp; }

private:

	uint	m_type;
	uint	m_timestamp;
	void*	mp_resource;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//render_device
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace iface{ class render_device
{
public:

	//�R���X�g���N�^
	render_device(const uint command_buffer_size = mebi(16));

	//�t���[������Ԃ�
	uint frame_count() const { return m_frame_count; }

	//���s
	void present(swapchain& swapchain);

	//GPU����������ҋ@
	virtual void wait_idle() = 0;

	//�V�F�[�_�R���p�C�����쐬
	virtual shader_compiler_ptr create_shader_compiler() = 0;

	//�X���b�v�`�F�C�����쐬
	virtual swapchain_ptr create_swapchain(const uint2 size) = 0;

	//�e�N�X�`�����쐬
	virtual texture_ptr create_texture1d(const texture_format format, const uint width, const uint mip_levels, const resource_flags flags, const void* data = nullptr) = 0;
	virtual texture_ptr create_texture2d(const texture_format format, const uint width, const uint height, const uint mip_levels, const resource_flags flags, const void* data = nullptr) = 0;
	virtual texture_ptr create_texture3d(const texture_format format, const uint width, const uint height, const uint depth, const uint mip_levels, const resource_flags flags, const void* data = nullptr) = 0;
	virtual texture_ptr create_texture_cube(const texture_format format, const uint width, const uint height, const uint mip_levels, const resource_flags flags, const void* data = nullptr) = 0;
	virtual texture_ptr create_texture1d_array(const texture_format format, const uint width, const uint depth, const uint mip_levels, const resource_flags flags, const void* data = nullptr) = 0;
	virtual texture_ptr create_texture2d_array(const texture_format format, const uint width, const uint height, const uint depth, const uint mip_levels, const resource_flags flags, const void* data = nullptr) = 0;
	virtual texture_ptr create_texture_cube_array(const texture_format format, const uint width, const uint height, const uint depth, const uint mip_levels, const resource_flags flags, const void* data = nullptr) = 0;

	//�o�b�t�@���쐬
	virtual buffer_ptr create_structured_buffer(const uint stride, const uint num_elements, const resource_flags flags, const void* data = nullptr) = 0;
	virtual buffer_ptr create_byteaddress_buffer(const uint size, const resource_flags flags, const void* data = nullptr) = 0;
	virtual constant_buffer_ptr create_temporary_cbuffer(const uint size, const void* data = nullptr) = 0;
	virtual constant_buffer_ptr create_constant_buffer(const uint size, const void* data = nullptr) = 0;
	virtual upload_buffer_ptr create_upload_buffer(const uint size) = 0;
	virtual readback_buffer_ptr create_readback_buffer(const uint size) = 0;

	//�T���v�����쐬
	virtual sampler_ptr create_sampler(const sampler_desc& desc) = 0;

	//TLAS/BLAS���쐬
	virtual top_level_acceleration_structure_ptr create_top_level_acceleration_structure(const uint num_instances) = 0;
	virtual bottom_level_acceleration_structure_ptr create_bottom_level_acceleration_structure(const geometry_state& gs, const raytracing_geometry_desc* descs, const uint num_descs, const bool allow_update) = 0;

	//View���쐬
	virtual render_target_view_ptr create_render_target_view(texture& tex, const texture_rtv_desc& desc) = 0;
	virtual depth_stencil_view_ptr create_depth_stencil_view(texture& tex, const texture_dsv_desc& desc) = 0;
	virtual shader_resource_view_ptr create_shader_resource_view(buffer& buf, const buffer_srv_desc& desc) = 0;
	virtual shader_resource_view_ptr create_shader_resource_view(texture& tex, const texture_srv_desc& desc) = 0;
	virtual unordered_access_view_ptr create_unordered_access_view(buffer& buf, const buffer_uav_desc& desc) = 0;
	virtual unordered_access_view_ptr create_unordered_access_view(texture& tex, const texture_uav_desc& desc) = 0;

	//�p�C�v���C���X�e�[�g���쐬
	virtual pipeline_state_ptr create_compute_pipeline_state(const string& name, shader_ptr p_cs) = 0;
	virtual pipeline_state_ptr create_graphics_pipeline_state(const string& name, shader_ptr p_vs, shader_ptr p_ps, const input_layout_desc& il, const rasterizer_desc& rs, const depth_stencil_desc& dss, const blend_desc& bs) = 0;

	//�^�[�Q�b�g�X�e�[�g���쐬
	virtual target_state_ptr create_target_state(const uint num_rtvs, render_target_view* rtv_ptrs[], depth_stencil_view* p_dsv) = 0;

	//�W�I���g���X�e�[�g���쐬
	virtual geometry_state_ptr create_geometry_state(const uint num_vbs, buffer* vb_ptrs[8], uint offsets[8], uint strides[8], buffer* p_ib) = 0;

	//�o�C���h���X���\�[�X�Ƃ��ēo�^
	virtual void register_bindless(sampler& sampler) = 0;
	virtual void register_bindless(constant_buffer& cbv) = 0;
	virtual void register_bindless(shader_resource_view& srv) = 0;

	//�o�C���h���X���\�[�X�̓o�^����
	virtual void unregister_bindless(sampler& sampler) = 0;
	virtual void unregister_bindless(constant_buffer& cbv) = 0;
	virtual void unregister_bindless(shader_resource_view& srv) = 0;

	//���O��ݒ�
	virtual void set_name(buffer& buf, const wchar_t* name) = 0;
	virtual void set_name(upload_buffer& buf, const wchar_t* name) = 0;
	virtual void set_name(constant_buffer& buf, const wchar_t* name) = 0;
	virtual void set_name(readback_buffer& buf, const wchar_t* name) = 0;
	virtual void set_name(texture& tex, const wchar_t* name) = 0;
	virtual void set_name(top_level_acceleration_structure& tlas, const wchar_t* name) = 0;
	virtual void set_name(bottom_level_acceleration_structure& blas, const wchar_t* name) = 0;

private:

	friend class render_context;

	//���s
	virtual void present_impl(swapchain& swapchain) = 0;

	//�o�b�t�@�X�V�p�̃|�C���^��Ԃ�
	virtual void* get_update_buffer_pointer(const uint size) = 0;

	//���s�\���̃`�F�b�N�Ǝ��s�ɕK�v�ȃf�[�^���쐬
	virtual void* validate(const pipeline_state& ps, const unordered_map<string, bind_resource>& resources) = 0;
	virtual void* validate(const pipeline_state& ps, const target_state& ts, const geometry_state& gs, const unordered_map<string, bind_resource>& resources) = 0;

protected:

	//�o�C���h���X�n���h����ݒ�
	void set_bindless_handle(bindless_resource& res, const uint handle);

protected:

	command::manager m_command_manager;

private:

	uint m_frame_count;
};}

///////////////////////////////////////////////////////////////////////////////////////////////////
//�֐��錾
///////////////////////////////////////////////////////////////////////////////////////////////////

//1�e�N�Z���̃o�C�g����Ԃ�
uint texel_size(const texture_format format);

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#include"render_device/render_device-impl.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

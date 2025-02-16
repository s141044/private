
#pragma once

#ifndef NN_RENDER_RENDER_TYPE_HPP
#define NN_RENDER_RENDER_TYPE_HPP

#include"core.hpp"
#include"image.hpp"
#include"utility.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//priority
///////////////////////////////////////////////////////////////////////////////////////////////////

enum priority
{
	priority_initiaize = 0, 
	priority_compute_skinning = 100,
	priority_build_bottom_level_acceleration_structure = 200,
	priority_refit_bottom_level_acceleration_structure = 300,
	priority_compact_bottom_level_acceleration_structure = 400,
	priority_build_top_level_acceleration_structure = priority_compact_bottom_level_acceleration_structure + 10,
	priority_pick = 500,

	priority_render_hdr = 600, 
	priority_postprocess = 700,
	priority_tonemapping = 800, 
	priority_render_ldr = 900, 
	
	priority_draw_picked = 900,
	priority_render_gui = 1000,

	priority_present = 0xffffffff,
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//bindless_instance_desc
/*/////////////////////////////////////////////////////////////////////////////////////////////////
procedural�̏ꍇ�͉�������H
/////////////////////////////////////////////////////////////////////////////////////////////////*/

struct bindless_instance_desc
{
	uint	bindless_geometry_handle;
	uint	bindless_material_handle;
	uint	start_index_location;
	uint	base_vertex_location;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//transform
/*/////////////////////////////////////////////////////////////////////////////////////////////////
m*v�̏�
/////////////////////////////////////////////////////////////////////////////////////////////////*/

class transform
{
public:

	//�R���X�g���N�^
	transform() : 
		m_ltow(float4(1,0,0,0),float4(0,1,0,0),float4(0,0,1,0),float4(0,0,0,1)), 
		m_wtol(float4(1,0,0,0),float4(0,1,0,0),float4(0,0,1,0),float4(0,0,0,1)){}

	//�ϊ��s���Ԃ�
	const float4x4 &ltow_matrix() const { return m_ltow; }
	const float4x4 &wtol_matrix() const { return m_wtol; }

	//�ϊ��s���ݒ�
	void set_ltow_matrix(const float4x4 &ltow){ m_ltow = ltow; m_wtol = inverse(m_ltow); }
	void set_wtol_matrix(const float4x4 &wtol){ m_wtol = wtol; m_ltow = inverse(m_wtol); }

private:

	float4x4 m_ltow;
	float4x4 m_wtol;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//drwa_type
///////////////////////////////////////////////////////////////////////////////////////////////////

enum draw_type
{
	draw_type_mask, //��0����������
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//render_entity
///////////////////////////////////////////////////////////////////////////////////////////////////

class render_entity : public transform
{
public:

	static constexpr uint invalid_instance_index = -1;

	//�f�X�g���N�^
	virtual ~render_entity();

	//�X�V
	virtual void update(render_context &context, const float dt){}

	//�`��
	virtual void draw(render_context &context, draw_type type){}

	//�C���X�^���X�C���f�b�N�X��Ԃ�
	uint bindless_instance_index() const { return m_bindless_instance_index; }
	uint raytracing_instance_index() const { return m_raytracing_instance_index; }

protected:

	//�C���X�^���X�o�^/����
	void register_instance(const uint bindless_instance_count, const uint raytracing_instance_count);
	void unregister_instance();

private:

	uint	m_bindless_instance_index = invalid_instance_index;
	uint	m_raytracing_instance_index = invalid_instance_index;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//material_type
///////////////////////////////////////////////////////////////////////////////////////////////////

enum material_type : uint
{
	material_type_standard,
	material_type_transparent,

	//
	material_type_lambert,
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//material
/*/////////////////////////////////////////////////////////////////////////////////////////////////
bindless�f�[�^�̐擪�͌Œ�
/////////////////////////////////////////////////////////////////////////////////////////////////*/

class material
{
public:

	static constexpr uint blas_group_key_max = 16;

	struct bindless_material_base
	{
		bindless_material_base(const material_type type, const uint alpha_map) : alpha_map_type((alpha_map & 0x00ffffff) | (type << 24)){}
		uint	alpha_map_type;
	};

	//�R���X�g���N�^
	material(const material_type type) : m_type(type), m_blas_group_key(0){}

	//�f�X�g���N�^
	virtual ~material(){ if((mp_srv != nullptr) && (mp_srv->bindless_handle() != invalid_bindless_handle)){ gp_render_device->unregister_bindless(*mp_srv); } }

	//�o�C���h���X�̊Ǘ�
	virtual void register_bindless() = 0;
	virtual void unregister_bindless(){ gp_render_device->unregister_bindless(*mp_srv); }

	//�}�e���A���^�C�v��Ԃ�
	material_type type() const { return m_type; }

	//�o�C���h���X�n���h����Ԃ�
	uint bindless_handle() const { return mp_srv->bindless_handle(); }

	//BLAS�O���[�s���O�̃L�[��Ԃ�
	uint blas_group_key() const { return assert(m_blas_group_key < blas_group_key_max), m_blas_group_key; }

protected:

	uint						m_blas_group_key;
	buffer_ptr					mp_buf;
	shader_resource_view_ptr	mp_srv;

private:

	material_type				m_type;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

using material_ptr = shared_ptr<material>;

///////////////////////////////////////////////////////////////////////////////////////////////////
//bindless_geometry
/*/////////////////////////////////////////////////////////////////////////////////////////////////
uint32	num_vbs;
uint32	ib_handle;
uint32	vb_handles[8];
uint32	offsets[8]; //���_�̐擪����̃I�t�Z�b�g���܂߂�
uint8	strides[8];
/////////////////////////////////////////////////////////////////////////////////////////////////*/

class bindless_geometry
{
public:

	//�R���X�g���N�^
	bindless_geometry() = default;
	bindless_geometry(const geometry_state &gs, const uint offset[8]);

	//�f�X�g���N�^
	~bindless_geometry();

	//�o�C���h���X�n���h����Ԃ�
	uint bindless_handle() const { return mp_srv->bindless_handle(); }

private:

	buffer_ptr					mp_buf;
	shader_resource_view_ptr	mp_srv;
	shader_resource_view_ptr	mp_ib_srv;
	shader_resource_view_ptr	mp_vb_srv[8];
};

///////////////////////////////////////////////////////////////////////////////////////////////////

using bindless_geometry_ptr = shared_ptr<bindless_geometry>;

///////////////////////////////////////////////////////////////////////////////////////////////////
//mesh
/*/////////////////////////////////////////////////////////////////////////////////////////////////
�EStaticMesh
0: {"name": "POSITION", "slot": 0, "offset": 0, "format": "r32g32b32_float"},
1: {"name": "NORMAL",   "slot": 1, "offset": 0, "format": "r16g16_unorm"}, //oct�̈��k
2: {"name": "TANGENT",  "slot": 1, "offset": 4, "format": "r16g16_snorm"}, //oct�̈��k & x�̕�����dot(cross(normal,tangent),binormal)
3: {"name": "TEXCOORD", "slot": 1, "offset": 8, "format": "r32g32_float"}, //half���Ɛ��x�s��...

�ESkinningMesh (�X�L�j���O�v�Z�͒��_�V�F�[�_�ōs�킸���O��Compute�V�F�[�_�Ŏ��s)
0: {"name": "POSITION",	"slot": 0, "offset": 0, "format": "r32g32b32_float", "index": 0},
1: {"name": "NORMAL",	"slot": 1, "offset": 0, "format": "r16g16_unorm"},
2: {"name": "TANGENT",	"slot": 1, "offset": 4, "format": "r16g16_snorm"},
3: {"name": "TEXCOORD",	"slot": 1, "offset": 8, "format": "r32g32_float"},
4: {"name": "POSITION", "slot": 0, "offset": 0, "format": "r32g32b32_float", "index": 1},
/////////////////////////////////////////////////////////////////////////////////////////////////*/

class mesh : public render_entity
{
public:

	struct cluster
	{
		uint index_count;
		uint vertex_count; //BLAS�쐬�ɕK�v
		uint start_index_location;
		uint base_vertex_location;
		uint material_index;
	};

	//�R���X�g���N�^
	mesh();

	//�X�V
	void update(render_context &context, const float dt) final;

	//�`��
	void draw(render_context &context, draw_type type) final;

	//�W�I���g���X�e�[�g��Ԃ�
	const geometry_state &geometry_state() const { return *m_gs_ptrs[gp_render_device->frame_count() % 2]; }
	const bindless_geometry &bindless_geometry() const { return *m_bindless_gs_ptrs[gp_render_device->frame_count() % 2]; }

	//�}�e���A����Ԃ�
	const vector<material_ptr> &material_ptrs() const { return m_material_ptrs; }

	//�N���X�^��Ԃ�
	const vector<cluster> &clusters() const { return m_clusters; }

protected:

	//�W�I���g���̍X�V
	virtual void update_geometry(render_context&){}

	//���C�g���p�f�[�^�̍X�V
	void update_raytracing(render_context &context);
	
	//�@��/�ڐ�/UV�����k
	static uint encode_normal(const float3 &normal);
	static uint encode_tangent(const float3 &tangent, const float3 &binormal, const float3 &normal);
	//static uint encode_uv(const float2 &uv);

protected:

	geometry_state_ptr								mp_orig_gs;
	geometry_state_ptr								m_gs_ptrs[2];
	vector<cluster>									m_clusters;
	vector<material_ptr>							m_material_ptrs;

private:

	shader_file_holder								m_shaders;
	vector<bottom_level_acceleration_structure_ptr> m_blas_ptrs;
	bindless_geometry_ptr							m_bindless_gs_ptrs[2];
	bool											m_compaction_completed = false;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

using mesh_ptr = shared_ptr<mesh>;

///////////////////////////////////////////////////////////////////////////////////////////////////
//raytracing_manager
/*/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////*/

class raytracing_manager
{
public:

	//�R���X�g���N�^
	raytracing_manager(const uint max_size);

	//�C���X�^���X�o�^/����
	struct register_result{ uint bindless_instance_index, raytracing_instance_index; };
	register_result register_instance(const uint bindless_instance_count, const uint raytracing_instance_count);
	void unregister_instance(const uint bindless_instance_index, const uint raytracing_instance_index);

	//TLAS�\�z
	void build(render_context &context);

	//���\�[�X��Ԃ�
	top_level_acceleration_structure &tlas() const { return *mp_tlas; }
	shader_resource_view &bindless_instance_descs_srv() const { return *mp_bindless_isntance_descs_srv; }
	shader_resource_view &raytracing_instance_descs_srv() const { return *mp_raytracing_isntance_descs_srv; }

	//�X�V�p�A�h���X��Ԃ�
	bindless_instance_desc *update_bindless_instance(const uint i){ return mp_bindless_instance_descs_ubuf->data<bindless_instance_desc>() + i; }
	raytracing_instance_desc *update_raytracing_instance(const uint i){ return mp_raytracing_instance_descs_ubuf->data<raytracing_instance_desc>() + i; }

private:

	top_level_acceleration_structure_ptr	mp_tlas;
	buffer_ptr								mp_bindless_instance_descs_buf;
	shader_resource_view_ptr				mp_bindless_isntance_descs_srv;
	upload_buffer_ptr						mp_bindless_instance_descs_ubuf;
	shader_resource_view_ptr				mp_raytracing_isntance_descs_srv;
	upload_buffer_ptr						mp_raytracing_instance_descs_ubuf;
	default_allocator						m_bindless_instance_allocator;
	default_allocator						m_raytracing_instance_allocator;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

static inline std::unique_ptr<raytracing_manager> gp_raytracing_manager;

///////////////////////////////////////////////////////////////////////////////////////////////////
//raytracing_picker
/*/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////*/

class raytracing_picker
{
public:

	//�R���X�g���N�^
	raytracing_picker(const uint max_size);

	//�C���X�^���X�o�^/����
	void register_instance(render_entity &entity);
	void unregister_instance(render_entity &entity);

	//�s�b�N�v��
	void pick_request(render_context &context, const uint2 &pixel_pos);

	//�s�b�N���ʂ�Ԃ�
	render_entity* get();

	//�s�b�N���ʂ��N���A
	void clear();

private:

	uint						m_complete_frame;
	shader_file_holder			m_shaders;
	vector<render_entity*>		m_current;
	vector<render_entity*>		m_past;
	vector<render_entity*>		m_unregistered;
	readback_buffer_ptr			mp_readback_buf;
	buffer_ptr					mp_result_buf;
	unordered_access_view_ptr	mp_result_uav;
	render_entity*				mp_picked;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

static inline std::unique_ptr<raytracing_picker> gp_raytracing_picker;

///////////////////////////////////////////////////////////////////////////////////////////////////
//texture_type
///////////////////////////////////////////////////////////////////////////////////////////////////

enum texture_type
{
	texture_type_rgb,
	texture_type_srgb,
	texture_type_normal,
	texture_type_scalar,
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//texture_manager
/*/////////////////////////////////////////////////////////////////////////////////////////////////
TODO: �X�V�Ń~�b�v�}�b�v����
/////////////////////////////////////////////////////////////////////////////////////////////////*/

class texture_manager
{
public:

	class texture_resource;

	//�R���X�g���N�^
	texture_manager();

	//�쐬
	shared_ptr<texture_resource> create(const string &filename, const texture_type type);

	//�X�V
	bool update(render_context &context);

private:

	using dictionary = unordered_map<string, shared_ptr<texture_resource>>;

	shader_file_holder					m_shaders;
	dictionary							m_texture_ptrs;
	queue<weak_ptr<texture_resource>>	m_queue;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

static inline std::unique_ptr<texture_manager> gp_texture_manager;

///////////////////////////////////////////////////////////////////////////////////////////////////
//texture_manager::texture_resource
///////////////////////////////////////////////////////////////////////////////////////////////////

class texture_manager::texture_resource
{
public:

	//�f�X�g���N�^
	~texture_resource();

	//SRV��Ԃ�
	shader_resource_view &srv() const;

private:

	friend class texture_manager;

	//�R���X�g���N�^
	texture_resource() = default;
	texture_resource(texture_ptr p_tex, dictionary::iterator it);

private:

	texture_ptr					mp_tex;
	shader_resource_view_ptr	mp_srv;
	dictionary::iterator		m_it;
	bool						m_initialized;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

using texture_resource_ptr = shared_ptr<texture_manager::texture_resource>;

///////////////////////////////////////////////////////////////////////////////////////////////////
//camera
/*/////////////////////////////////////////////////////////////////////////////////////////////////
�J�����̎���view�s�񂩂���o����
m=(axisX -dot(axisX,p))
  (axisY -dot(axisY,p))
  (axisZ -dot(axisZ,p))
proj�s���reverse-z.������0.�߂���1.
///////////////////////////////////////////////////////////////////////////////////////////////////
�s��cpp�ł͍s�D�悾��hlsl�͗�D��Ȃ̂Ōv�Z����Ƃ��͏������t�ɂ���K�v������
/////////////////////////////////////////////////////////////////////////////////////////////////*/

class camera
{
public:

	//�R���X�g���N�^
	camera() = default;

	//�ʒu&�����_��ݒ�
	void look_at(const float3 &pos, const float3 &center);
	void look_at(const float3 &pos, const float3 &dir, const float dist);

	//�ʒu
	const float3 &position() const;
	void set_position(const float3& position);

	//�����_
	float3 center() const;

	//�����_�܂ł̋���
	float distance() const;

	//����p
	float fovy() const;
	void set_fovy(const float fovy);

	//�N���b�v����
	float near_clip() const;
	float far_clip() const;
	void set_near_clip(const float near_clip);
	void set_far_clip(const float far_dist);

	//�A�X�y�N�g��
	float aspect() const;
	void set_aspect(const float aspect);

	//�r���[/�ˉe�s���Ԃ�
	float3x4 view_mat() const;
	float4x4 proj_mat() const;

	//����Ԃ�
	const float3 &axis_x() const;
	const float3 &axis_y() const;
	const float3 &axis_z() const;

public:

	//���s�ړ�
	void translate(const float3 &t);

	//����/������U��
	void pan(const float radian);
	void tilt(const float radian);

	//�����_����̉�]
	void rotate_x(const float radian);
	void rotate_y(const float radian);

	//�����_�ɐڋ�
	void dolly_in(const float dist);

	//��p�����߂�
	void zoom_in(const float degree);

private:

	float3	m_pos;
	float3	m_axis_x;
	float3	m_axis_y;
	float3	m_axis_z;
	float	m_dist;
	float	m_fovy;
	float	m_aspect;
	float	m_near_clip;
	float	m_far_clip;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//base_application
///////////////////////////////////////////////////////////////////////////////////////////////////

class base_application : public application
{
public:

	//�R���X�g���N�^
	using application::application;

private:

	//������
	void initialize() override final;

	//�I������
	void finalize() override final;

	//�X�V
	int update(render_context& context, const float delta_time) override final;

	//���T�C�Y
	void resize(const uint2 size) override final;

protected:

	//�R���X�^���g�o�b�t�@�̍X�V
	void update_scene_info(render_context& context, const uint2 screen_size, const float jitter_scale = 1);

	//GUI�`�恕Present
	void composite_gui_and_present(render_context& context, shader_resource_view& src_srv, bool r9g9b9e5 = true);

	//LDR�^�[�Q�b�g/RTV/SRV��Ԃ�
	target_state& ldr_target(){ return *mp_ldr_target; }
	render_target_view& ldr_color_rtv(){ return *mp_ldr_color_rtv; }
	shader_resource_view& ldr_color_srv(){ return *mp_ldr_color_srv; }

private:

	//������
	virtual void initialize_impl() = 0;

	//�I������
	virtual void finalize_impl() = 0;

	//�X�V
	virtual int update_impl(render_context& context, const float delta_time) = 0;

	//���T�C�Y
	virtual void resize_impl(const uint2 size) = 0;

protected:

	camera						m_camera;
	float						m_move_speed	= 0.05f;
	float						m_zoom_speed	= 1.0f / 120.0f;
	float						m_dolly_speed	= 0.1f / 120.0f;
	float						m_pan_speed		= -0.01f / PI();
	float						m_tilt_speed	= -0.01f / PI();
	float						m_rotate_speed	= -0.01f / PI();

private:

	shader_file_holder			m_present_shaders;
	shader_file_holder			m_dev_draw_shaders;
	geometry_state_ptr			mp_dummy_geometry;
	target_state_ptr			mp_present_target[swapchain::buffer_count()];
	target_state_ptr			mp_ldr_target;
	texture_ptr					mp_ldr_color_tex;
	shader_resource_view_ptr	mp_ldr_color_srv;
	render_target_view_ptr		mp_ldr_color_rtv;
	constant_buffer_ptr			mp_camera_info;
	float4x4					m_inv_view_proj_mat;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#include"base/mesh-impl.hpp"
#include"base/camera-impl.hpp"
#include"base/render_entity-impl.hpp"
#include"base/texture_manager-impl.hpp"
#include"base/base_application-impl.hpp"
#include"base/raytracing_picker-impl.hpp"
#include"base/bindless_geometry-impl.hpp"
#include"base/raytracing_manager-impl.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

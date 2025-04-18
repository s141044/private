
#pragma once

#ifndef NN_RENDER_RENDER_BASE_HPP
#define NN_RENDER_RENDER_BASE_HPP

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
proceduralの場合は何がいる？
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
T*Rz*Ry*Rx*S*vの順
/////////////////////////////////////////////////////////////////////////////////////////////////*/

class transform
{
public:

	//コンストラクタ
	transform() : m_t(0, 0, 0), m_r(0, 0, 0), m_s(1, 1, 1), m_update_frame(){}

	//変換行列を返す
	float3x4 ltow_matrix() const;
	float3x4 wtol_matrix() const;

	//平行移動
	const float3& translation() const { return m_t; }
	void set_translation(const float3& t){ if(m_t != t){ m_update_frame = gp_render_device->frame_count(); m_t = t; }}

	//回転
	const float3& rotation() const { return m_r; }
	void set_rotation(const float3& r){ if(m_r != r){ m_update_frame = gp_render_device->frame_count(); m_r = r; }}

	//スケーリング
	const float3& scaling() const { return m_s; }
	void set_scaling(const float3& s){ if(m_s != s){ m_update_frame = gp_render_device->frame_count(); m_s = s; }}

	//更新フレームを返す
	uint update_frame() const { return m_update_frame; }

private:

	float3	m_t;
	float3	m_r;
	float3	m_s;
	uint	m_update_frame;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//drwa_type
///////////////////////////////////////////////////////////////////////////////////////////////////

enum draw_type
{
	draw_type_mask, //非0を書き込む
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//render_entity
///////////////////////////////////////////////////////////////////////////////////////////////////

class render_entity : public transform
{
public:

	static constexpr uint invalid_instance_index = -1;

	//デストラクタ
	virtual ~render_entity();

	//更新
	virtual void update(render_context& context, const float dt){}

	//描画
	virtual void draw(render_context& context, draw_type type){}

	//インスタンスインデックスを返す
	uint bindless_instance_index() const { return m_bindless_instance_index; }
	uint raytracing_instance_index() const { return m_raytracing_instance_index; }

protected:

	//インスタンス登録/解除
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
bindlessデータの先頭は固定
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

	//コンストラクタ
	material(const material_type type) : m_type(type), m_blas_group_key(0), m_has_update(false){}

	//デストラクタ
	virtual ~material(){ if((mp_srv != nullptr) && (mp_srv->bindless_handle() != invalid_bindless_handle)){ gp_render_device->unregister_bindless(*mp_srv); } }

	//バインドレスの管理
	virtual void register_bindless() = 0;
	virtual void unregister_bindless(){ gp_render_device->unregister_bindless(*mp_srv); }

	//パラメータの更新があるか
	bool has_update() const { return m_has_update; }

	//パラメータを更新
	virtual void update(render_context& context) = 0;

	//発光パワーを返す
	float emissive_power() const { return m_emissive_power; }

	//マテリアルタイプを返す
	material_type type() const { return m_type; }

	//バインドレスハンドルを返す
	uint bindless_handle() const { return mp_srv->bindless_handle(); }

	//BLASグルーピングのキーを返す
	uint blas_group_key() const { return assert(m_blas_group_key < blas_group_key_max), m_blas_group_key; }

protected:

	uint						m_blas_group_key;
	buffer_ptr					mp_buf;
	shader_resource_view_ptr	mp_srv;
	float						m_emissive_power;
	bool						m_has_update;

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
uint32	offsets[8]; //頂点の先頭からのオフセットも含める
uint8	strides[8];
/////////////////////////////////////////////////////////////////////////////////////////////////*/

class bindless_geometry
{
public:

	//コンストラクタ
	bindless_geometry() = default;
	bindless_geometry(const geometry_state& gs, const uint offset[8]);

	//デストラクタ
	~bindless_geometry();

	//バインドレスハンドルを返す
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
//emissive_blas
///////////////////////////////////////////////////////////////////////////////////////////////////

class emissive_blas
{
public:

	//コンストラクタ
	emissive_blas();

	//デストラクタ
	~emissive_blas();

	//ビルド
	bool build(render_context& context, const uint raytracing_instance_index, const uint bindless_instance_index, const uint primitive_count, const float base_power);

	//更新
	void update(render_context& context, const float base_power);

	//バインドレスハンドルを返す
	uint bindless_handle() const { return mp_blas_srv->bindless_handle(); }

private:

	struct header
	{
		float		area;
		float		base_power;
		uint16_t	primitive_count;
		uint16_t	raytracing_instance_index;
		uint		bindless_instance_index;
	};
	struct bucket
	{
		float	w;
		uint	a; 
	};

	shader_file_holder			m_shaders;
	buffer_ptr					mp_blas_buf;
	shader_resource_view_ptr	mp_blas_srv;
	unordered_access_view_ptr	mp_blas_uav;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//emissive_tlas
///////////////////////////////////////////////////////////////////////////////////////////////////

class emissive_tlas
{
public:

	//コンストラクタ
	emissive_tlas(const uint max_instance);

	//登録/解除
	uint register_blas(const emissive_blas& blas);
	void unregister_blas(uint index);

	//ビルド
	bool build(render_context& context);

	//リソースをバインド
	void bind(render_context& context);

private:

	struct header
	{
		float	power;
		float	inv_power;
		uint	blas_count;
		uint	padding;
	};
	struct bucket
	{
		float	w;
		uint	a; 
	};

	shader_file_holder			m_shaders;
	default_allocator			m_blas_allocator;
	upload_buffer_ptr			mp_blas_handle_ubuf;
	buffer_ptr					mp_blas_handle_buf;
	shader_resource_view_ptr	mp_blas_handle_srv;
	unordered_access_view_ptr	mp_blas_handle_uav;
	buffer_ptr					mp_tlas_buf;
	shader_resource_view_ptr	mp_tlas_srv;
	unordered_access_view_ptr	mp_tlas_uav;

	buffer_ptr					mp_weight_buf;
	shader_resource_view_ptr	mp_weight_srv;
	unordered_access_view_ptr	mp_weight_uav;
	buffer_ptr					mp_sum_weight_buf;
	shader_resource_view_ptr	mp_sum_weight_srv;
	unordered_access_view_ptr	mp_sum_weight_uav;
	buffer_ptr					mp_max_weight_buf;
	shader_resource_view_ptr	mp_max_weight_srv;
	unordered_access_view_ptr	mp_max_weight_uav;
	buffer_ptr					mp_light_index_buf;
	shader_resource_view_ptr	mp_light_index_srv;
	unordered_access_view_ptr	mp_light_index_uav;
	buffer_ptr					mp_heavy_index_buf;
	shader_resource_view_ptr	mp_heavy_index_srv;
	unordered_access_view_ptr	mp_heavy_index_uav;
	buffer_ptr					mp_light_sum_weight_buf;
	shader_resource_view_ptr	mp_light_sum_weight_srv;
	unordered_access_view_ptr	mp_light_sum_weight_uav;
	buffer_ptr					mp_heavy_sum_weight_buf;
	shader_resource_view_ptr	mp_heavy_sum_weight_srv;
	unordered_access_view_ptr	mp_heavy_sum_weight_uav;
	buffer_ptr					mp_scan_scratch_buf;
	shader_resource_view_ptr	mp_scan_scratch_srv;
	unordered_access_view_ptr	mp_scan_scratch_uav;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//mesh
/*/////////////////////////////////////////////////////////////////////////////////////////////////
・StaticMesh
0: {"name": "POSITION", "slot": 0, "offset": 0, "format": "r32g32b32_float"},
1: {"name": "NORMAL",   "slot": 1, "offset": 0, "format": "r16g16_unorm"}, //octの圧縮
2: {"name": "TANGENT",  "slot": 1, "offset": 4, "format": "r16g16_snorm"}, //octの圧縮 & xの符号はdot(cross(normal,tangent),binormal)
3: {"name": "TEXCOORD", "slot": 1, "offset": 8, "format": "r32g32_float"}, //halfだと精度不足...

・SkinningMesh (スキニング計算は頂点シェーダで行わず事前にComputeシェーダで実行)
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
		uint vertex_count; //BLAS作成に必要
		uint start_index_location;
		uint base_vertex_location;
		uint material_index;
	};

	//コンストラクタ
	mesh();

	//更新
	void update(render_context& context, const float dt) final;

	//描画
	void draw(render_context& context, draw_type type) final;

	//ジオメトリステートを返す
	const geometry_state& geometry_state() const { return *m_gs_ptrs[gp_render_device->frame_count() % 2]; }
	const bindless_geometry& bindless_geometry() const { return *m_bindless_gs_ptrs[gp_render_device->frame_count() % 2]; }

	//クラスタを返す
	const vector<cluster>& clusters() const { return m_clusters; }

	//マテリアルを返す
	const vector<material_ptr>& material_ptrs() const { return m_material_ptrs; }
	
	//マテリアルを設定
	void set_material(const uint i, material_ptr p_mtl){ m_material_ptrs[i] = std::move(p_mtl); }

protected:

	//ジオメトリの更新
	virtual void update_geometry(render_context&){}

	//レイトレ用データの更新
	void update_raytracing(render_context& context);
	
	//マテリアルの更新
	void update_material(render_context& context);

	//法線/接線/UVを圧縮
	static uint encode_normal(const float3& normal);
	static uint encode_tangent(const float3& tangent, const float3& binormal, const float3& normal);
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
	uint											m_update_frame = 0;
	bool											m_compaction_completed = false;
	
	struct emissive_info
	{
		uint										bindless_instance_index;
		uint										emissive_instance_index;
		unique_ptr<emissive_blas>					p_emissive_blas;
		float										power;
	};
	vector<emissive_info>							m_emissive_info;
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

	//コンストラクタ
	raytracing_manager(const uint max_size);

	//インスタンス登録/解除
	struct register_result{ uint bindless_instance_index, raytracing_instance_index; };
	register_result register_instance(const uint bindless_instance_count, const uint raytracing_instance_count);
	void unregister_instance(const uint bindless_instance_index, const uint raytracing_instance_index);

	//Emissiveインスタンス登録/解除
	uint register_emissive(const emissive_blas& blas);
	void unregister_emissive(const uint index);

	//TLAS構築
	void build(render_context& context);

	//リソースをバインド
	void bind(render_context& context);

	//更新用アドレスを返す
	bindless_instance_desc* update_bindless_instance(const uint i){ return mp_bindless_instance_descs_ubuf->data<bindless_instance_desc>() + i; }
	raytracing_instance_desc* update_raytracing_instance(const uint i){ return mp_raytracing_instance_descs_ubuf->data<raytracing_instance_desc>() + i; }

private:

	top_level_acceleration_structure_ptr	mp_tlas;
	emissive_tlas							m_emissive_tlas;
	buffer_ptr								mp_bindless_instance_descs_buf;
	shader_resource_view_ptr				mp_bindless_instance_descs_srv;
	upload_buffer_ptr						mp_bindless_instance_descs_ubuf;
	shader_resource_view_ptr				mp_raytracing_instance_descs_srv;
	upload_buffer_ptr						mp_raytracing_instance_descs_ubuf;
	default_allocator						m_bindless_instance_allocator;
	default_allocator						m_raytracing_instance_allocator;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

static inline std::unique_ptr<raytracing_manager> gp_raytracing_manager;

///////////////////////////////////////////////////////////////////////////////////////////////////
//emissive_sampler
///////////////////////////////////////////////////////////////////////////////////////////////////

class emissive_sampler
{
public:

	//コンストラクタ
	emissive_sampler();

	//サンプリング
	bool sample(render_context& context, const uint sample_count);

	//リソースをバインド
	void bind(render_context& context);

	//デバッグ描画
	void debug_draw(render_context& context, const float scale = 0.1f);

private:

	struct sample_t
	{
		float3	position;
		uint	normal;
		uint	power;
		uint	pdf_approx;
	};

	shader_file_holder			m_shaders;
	buffer_ptr					mp_emissive_sample_buf;
	shader_resource_view_ptr	mp_emissive_sample_srv;
	unordered_access_view_ptr	mp_emissive_sample_uav;

	shader_file_holder			m_debug_shaders;
	buffer_ptr					mp_debug_ib;
	buffer_ptr					mp_debug_vb;
	geometry_state_ptr			mp_debug_gs;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//raytracing_picker
/*/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////*/

class raytracing_picker
{
public:

	//コンストラクタ
	raytracing_picker(const uint max_size);

	//インスタンス登録/解除
	void register_instance(render_entity& entity);
	void unregister_instance(render_entity& entity);

	//ピック要求
	void pick_request(render_context& context, const uint2& pixel_pos);

	//ピック結果を返す
	render_entity* get();

	//ピック結果をクリア
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
	texture_type_hdr,
	texture_type_rgb,
	texture_type_srgb,
	texture_type_normal,
	texture_type_scalar,
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//texture_manager
/*/////////////////////////////////////////////////////////////////////////////////////////////////
TODO: 更新でミップマップ生成
TODO: 使われてないテクスチャの削除
/////////////////////////////////////////////////////////////////////////////////////////////////*/

class texture_manager
{
public:

	class texture_resource;

	//コンストラクタ
	texture_manager();

	//デストラクタ
	~texture_manager();

	//作成
	shared_ptr<texture_resource> create(const string& filename, const texture_type type);

	//更新
	bool update(render_context& context);

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

	//デストラクタ
	~texture_resource();

	//SRVを返す
	shader_resource_view& srv() const;

	//ファイル名を返す
	const string& filename() const;

	//テクスチャを返す
	operator texture&(){ return *mp_tex; }
	operator const texture&() const { return *mp_tex; }

private:

	friend class texture_manager;

	//コンストラクタ
	texture_resource() = default;
	texture_resource(texture_ptr p_tex, string filename, dictionary::iterator it);

private:

	texture_ptr					mp_tex;
	shader_resource_view_ptr	mp_srv;
	dictionary::iterator		m_it;
	string						m_filename;
	bool						m_initialized;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

using texture_resource_ptr = shared_ptr<texture_manager::texture_resource>;

///////////////////////////////////////////////////////////////////////////////////////////////////
//camera
/*/////////////////////////////////////////////////////////////////////////////////////////////////
カメラの軸はview行列から取り出せる
m=(axisX -dot(axisX,p))
  (axisY -dot(axisY,p))
  (axisZ -dot(axisZ,p))
proj行列はreverse-z.遠くが0.近くが1.
///////////////////////////////////////////////////////////////////////////////////////////////////
行列がcppでは行優先だがhlslは列優先なので計算するときは順序を逆にする必要がある
/////////////////////////////////////////////////////////////////////////////////////////////////*/

class camera
{
public:

	//コンストラクタ
	camera() = default;

	//位置&注視点を設定
	void look_at(const float3& pos, const float3& center);
	void look_at(const float3& pos, const float3& dir, const float dist);

	//位置
	const float3& position() const;
	void set_position(const float3& position);

	//注視点
	float3 center() const;

	//注視点までの距離
	float distance() const;

	//視野角
	float fovy() const;
	void set_fovy(const float fovy);

	//クリップ距離
	float near_clip() const;
	float far_clip() const;
	void set_near_clip(const float near_clip);
	void set_far_clip(const float far_dist);

	//アスペクト比
	float aspect() const;
	void set_aspect(const float aspect);

	//ビュー/射影行列を返す
	float3x4 view_mat() const;
	float4x4 proj_mat() const;
	float3x4 inv_view_mat() const;
	float4x4 inv_proj_mat() const;

	//軸を返す
	const float3& axis_x() const;
	const float3& axis_y() const;
	const float3& axis_z() const;

public:

	//平行移動
	void translate(const float3& t);

	//水平/垂直首振り
	void pan(const float radian);
	void tilt(const float radian);

	//注視点周りの回転
	void rotate_x(const float radian);
	void rotate_y(const float radian);

	//注視点に接近
	void dolly_in(const float dist);

	//画角を狭める
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
//debug_draw
///////////////////////////////////////////////////////////////////////////////////////////////////

class debug_draw
{
public:

	struct vertex
	{
		vertex() = default;
		vertex(const float3& p, const float3& c);
		float3	position;
		uint	color;
	};

	//コンストラクタ
	debug_draw(const uint capacity);

	//ライン/ポイント描画
	vertex* draw_lines(const uint num_lines);
	vertex* draw_points(const uint num_points);
	vertex* draw_line_strip(const uint num_lines);

	//遅延描画
	void draw(std::function<void()> func);

	//描画
	bool draw(render_context& context, target_state& target_state);

private:

	enum draw_type
	{
		draw_type_line,
		draw_type_line_strip,
		draw_type_point,
		draw_type_count,
	};
	struct draw_info
	{
		uint	vertex_count;
		uint	start_vertex_location;
	};
	
	vertex* draw_xxx(const uint vertex_count, const draw_type type);

private:

	shader_file_holder				m_shader_file;
	buffer_ptr						mp_vertex_buf;
	unordered_access_view_ptr		mp_vertex_uav;
	upload_buffer_ptr				mp_vertex_ubuf;
	geometry_state_ptr				mp_geometry_state;
	uint							m_front;
	uint							m_back;
	uint							m_capacity;
	vector<draw_info>				m_draw_infos[draw_type_count];
	queue<std::function<void()>>	m_draw_queue;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

inline unique_ptr<debug_draw> gp_debug_draw;

///////////////////////////////////////////////////////////////////////////////////////////////////
//base_application
///////////////////////////////////////////////////////////////////////////////////////////////////

class base_application : public application
{
public:

	//コンストラクタ
	using application::application;

private:

	//初期化
	void initialize() override final;

	//終了処理
	void finalize() override final;

	//更新
	int update(render_context& context, const float delta_time) override final;

	//リサイズ
	void resize(const uint2 size) override final;

protected:

	//コンスタントバッファの更新
	void update_scene_info(render_context& context, const uint2 screen_size, const float jitter_scale = 1);
	void update_system_info(render_context& context, const float delta_time);

	//GUI描画＆Present
	void composite_gui_and_present(render_context& context, shader_resource_view& src_srv, bool r9g9b9e5 = true);

	//LDRターゲット/RTV/SRVを返す
	target_state& ldr_target(){ return *mp_ldr_target; }
	render_target_view& ldr_color_rtv(){ return *mp_ldr_color_rtv; }
	shader_resource_view& ldr_color_srv(){ return *mp_ldr_color_srv; }

private:

	//初期化
	virtual void initialize_impl() = 0;

	//終了処理
	virtual void finalize_impl() = 0;

	//更新
	virtual int update_impl(render_context& context, const float delta_time) = 0;

	//リサイズ
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
#include"base/transform-impl.hpp"
#include"base/debug_draw-impl.hpp"
#include"base/emissive_blas-impl.hpp"
#include"base/emissive_tlas-impl.hpp"
#include"base/render_entity-impl.hpp"
#include"base/texture_manager-impl.hpp"
#include"base/emissive_sampler-impl.hpp"
#include"base/base_application-impl.hpp"
#include"base/raytracing_picker-impl.hpp"
#include"base/bindless_geometry-impl.hpp"
#include"base/raytracing_manager-impl.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

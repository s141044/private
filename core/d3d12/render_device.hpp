
#pragma once

#ifndef NN_RENDER_CORE_D3D12_RENDER_DEVICE_HPP
#define NN_RENDER_CORE_D3D12_RENDER_DEVICE_HPP

#include"../render_device.hpp"

#include<d3d12.h>
#include<d3d12sdklayers.h>
#include<dxgi1_6.h>
#include<Windows.h>
#include<pix3.h>

#include<wrl.h>

#include<dxcapi.h>
#include<d3d12shader.h>
#include<d3dcompiler.h>

///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxcompiler.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "runtimeobject.lib")
#pragma comment(lib, "WinPixEventRuntime.lib")

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace d3d12{

///////////////////////////////////////////////////////////////////////////////////////////////////
//�O���錾
///////////////////////////////////////////////////////////////////////////////////////////////////

using Microsoft::WRL::ComPtr;

///////////////////////////////////////////////////////////////////////////////////////////////////

using ID3D12Device = ID3D12Device14;
using ID3D12GraphicsCommandList = ID3D12GraphicsCommandList4;
using IDXGIFactory = IDXGIFactory7;
using IDXGISwapChain = IDXGISwapChain4;
using IDXGIAdapter = IDXGIAdapter4;
using IDxcCompiler = IDxcCompiler3;

///////////////////////////////////////////////////////////////////////////////////////////////////

class render_device;
class geometry_state;

///////////////////////////////////////////////////////////////////////////////////////////////////
//com_wrapper
///////////////////////////////////////////////////////////////////////////////////////////////////

template<class T> class com_wrapper
{
public:

	T* operator->() const { return mp_impl.Get(); }
	operator T&() const { return *mp_impl.Get(); }
	T* get_impl() const { return mp_impl.Get(); }

protected:

	ComPtr<T> mp_impl;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//descriptor_heap
///////////////////////////////////////////////////////////////////////////////////////////////////

class descriptor_heap : public com_wrapper<ID3D12DescriptorHeap>
{
public:

	//�R���X�g���N�^
	descriptor_heap(render_device& device, const D3D12_DESCRIPTOR_HEAP_TYPE type, const uint num);

	//���蓖��/���
	D3D12_CPU_DESCRIPTOR_HANDLE allocate();
	void deallocate(const D3D12_CPU_DESCRIPTOR_HANDLE handle);

private:

	uint							m_size;
	size_t							m_start;
	lockfree_fixed_size_allocator	m_allocator;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//descriptor_handle
///////////////////////////////////////////////////////////////////////////////////////////////////

struct descriptor_handle
{
	D3D12_CPU_DESCRIPTOR_HANDLE cpu_start;
	D3D12_GPU_DESCRIPTOR_HANDLE gpu_start;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//sv_descriptor_heap
/*/////////////////////////////////////////////////////////////////////////////////////////////////
�擪�̓o�C���h���X�p�̃f�X�N���v�^���L�^
/////////////////////////////////////////////////////////////////////////////////////////////////*/

class sv_descriptor_heap : public com_wrapper<ID3D12DescriptorHeap>
{
public:

	//�R���X�g���N�^
	sv_descriptor_heap(render_device& device, const D3D12_DESCRIPTOR_HEAP_TYPE type, const uint num_bind, const uint num_bindless);

	//���蓖��/���
	descriptor_handle allocate(const uint num);
	void deallocate();

	//�o�C���h���X�p�̊��蓖��/���
	struct descriptor_handle_index : descriptor_handle{ uint index; };
	descriptor_handle_index allocate_bindless();
	void deallocate_bindless(const uint index);
	void deallocate_bindless(const D3D12_CPU_DESCRIPTOR_HANDLE cpu_start);
	void deallocate_bindless(const D3D12_GPU_DESCRIPTOR_HANDLE gpu_start);

	//�f�X�N���v�^�̃T�C�Y��Ԃ�
	uint descriptor_size() const { return m_size; }

private:

	uint							m_size;
	size_t							m_cpu_start;
	size_t							m_gpu_start;
	size_t							m_cpu_start_bindless;
	size_t							m_gpu_start_bindless;
	ring_allocator					m_allocator;
	lockfree_fixed_size_allocator	m_allocator_bindless;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//cpu_descriptor
///////////////////////////////////////////////////////////////////////////////////////////////////

class cpu_descriptor : public D3D12_CPU_DESCRIPTOR_HANDLE
{
public:

	//�R���X�g���N�^
	cpu_descriptor() : mp_heap(){}
	cpu_descriptor(const D3D12_CPU_DESCRIPTOR_HANDLE handle, descriptor_heap& heap) : D3D12_CPU_DESCRIPTOR_HANDLE(handle), mp_heap(&heap){}
	cpu_descriptor(const cpu_descriptor&) = delete;
	cpu_descriptor(cpu_descriptor&& d) : D3D12_CPU_DESCRIPTOR_HANDLE(d), mp_heap(d.mp_heap){ d.mp_heap = nullptr; }

	//�f�X�g���N�^
	~cpu_descriptor(){ if(mp_heap){ mp_heap->deallocate(*this); } }

	//���
	cpu_descriptor& operator=(cpu_descriptor &&d){ if(&d != this){ this->~cpu_descriptor(); new (this) cpu_descriptor(std::move(d)); } return *this; }

private:

	descriptor_heap*	mp_heap;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//resource
///////////////////////////////////////////////////////////////////////////////////////////////////

class resource : public com_wrapper<ID3D12Resource>
{
public:

	//�R���X�g���N�^
	resource() = default;

	//������
	void init(ID3D12Device& device, const D3D12_HEAP_PROPERTIES& prop, const D3D12_HEAP_FLAGS flags, const D3D12_RESOURCE_DESC& desc, const D3D12_RESOURCE_STATES state, const D3D12_CLEAR_VALUE* clear_value);
	void init(ComPtr<ID3D12Resource> p_impl, const D3D12_RESOURCE_DESC& desc, const D3D12_RESOURCE_STATES state);

	//�o�b�t�@/�e�N�X�`����
	bool is_buffer() const;
	bool is_texture() const;

private:

	friend class render_device;

	union state_info
	{
		struct
		{
			uint					last_used_frame;
			uint					last_used_pos;
			D3D12_RESOURCE_STATES	state;
			bool					implicit;
		};

		//tlas�p
		struct
		{
			uint					last_used_frame;
			uint					last_build_pos;
			uint					last_barrier_pos;
		};
	};
	vector<state_info> m_state_infos;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//buffer
///////////////////////////////////////////////////////////////////////////////////////////////////

class buffer : public render::buffer, public resource
{
public:

	//�R���X�g���N�^
	buffer(render_device& device, const uint stride, const uint num_elements, const resource_flags flags, const void* data, const D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON);

	//srv/uavDesc�𐶐�
	virtual D3D12_SHADER_RESOURCE_VIEW_DESC create_srv_desc(const buffer_srv_desc& desc) const = 0;
	virtual D3D12_UNORDERED_ACCESS_VIEW_DESC create_uav_desc(const buffer_uav_desc& desc) const = 0;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//structured_buffer
///////////////////////////////////////////////////////////////////////////////////////////////////

class structured_buffer : public buffer
{
public:

	//�R���X�g���N�^
	using buffer::buffer;

	//srv/uavDesc�𐶐�
	D3D12_SHADER_RESOURCE_VIEW_DESC create_srv_desc(const buffer_srv_desc& desc) const override;
	D3D12_UNORDERED_ACCESS_VIEW_DESC create_uav_desc(const buffer_uav_desc& desc) const override;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//byteaddress_buffer
///////////////////////////////////////////////////////////////////////////////////////////////////

class byteaddress_buffer : public buffer
{
public:

	//�R���X�g���N�^
	using buffer::buffer;

	//srv/uavDesc�𐶐�
	D3D12_SHADER_RESOURCE_VIEW_DESC create_srv_desc(const buffer_srv_desc& desc) const override;
	D3D12_UNORDERED_ACCESS_VIEW_DESC create_uav_desc(const buffer_uav_desc& desc) const override;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//constant_buffer
/*/////////////////////////////////////////////////////////////////////////////////////////////////
������/��������̂�1�x�����f�[�^�̍X�V���\
/////////////////////////////////////////////////////////////////////////////////////////////////*/

class constant_buffer : public render::constant_buffer
{
public:

	//�R���X�g���N�^
	constant_buffer(const uint size, cpu_descriptor view = cpu_descriptor());

	//View��Ԃ�
	operator D3D12_CPU_DESCRIPTOR_HANDLE() const;

protected:

	cpu_descriptor m_view;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//static_constant_buffer
/*/////////////////////////////////////////////////////////////////////////////////////////////////
�t���[���Œl���ω����Ȃ����̂Ɏg�p.�قȂ�l���K�v�ȏꍇ�͍Đ������K�v.
/////////////////////////////////////////////////////////////////////////////////////////////////*/

class static_constant_buffer : public constant_buffer, public resource
{
public:

	//�R���X�g���N�^
	static_constant_buffer(render_device& device, const uint size, const void* data);
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//temporary_constant_buffer
/*/////////////////////////////////////////////////////////////////////////////////////////////////
������1�t���[�������Ő����R�X�g�͋ɂ߂ĒႢ.
/////////////////////////////////////////////////////////////////////////////////////////////////*/

class temporary_constant_buffer : public constant_buffer
{
public:

	//�R���X�g���N�^
	temporary_constant_buffer(render_device& device, const uint size, const void* src_data, void* dst_data, cpu_descriptor view);
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//upload_buffer
///////////////////////////////////////////////////////////////////////////////////////////////////

class upload_buffer : public render::upload_buffer, public resource
{
public:

	//�R���X�g���N�^
	upload_buffer(render_device& device, const uint size, const void* data);

	//�f�X�g���N�^
	~upload_buffer();
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//readback_buffer
///////////////////////////////////////////////////////////////////////////////////////////////////

class readback_buffer : public render::readback_buffer, public resource
{
public:

	//�R���X�g���N�^
	readback_buffer(render_device& device, const uint size);

	//�f�X�g���N�^
	~readback_buffer();
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//texture
///////////////////////////////////////////////////////////////////////////////////////////////////

class texture : public render::texture, public resource
{
public:

	//�R���X�g���N�^
	texture(render_device& device, const texture_format format, const uint width, const uint height, const uint depth, const uint mip_levels, const resource_flags flags, const void* data, const D3D12_RESOURCE_DIMENSION dimension);
	texture(ComPtr<ID3D12Resource> p_target);

	//rtv/dsv/srv/uavDesc�𐶐�
	virtual D3D12_DEPTH_STENCIL_VIEW_DESC create_dsv_desc(const texture_dsv_desc& desc) const { throw; }
	virtual D3D12_RENDER_TARGET_VIEW_DESC create_rtv_desc(const texture_rtv_desc& desc) const { throw; }
	virtual D3D12_SHADER_RESOURCE_VIEW_DESC create_srv_desc(const texture_srv_desc& desc) const = 0;
	virtual D3D12_UNORDERED_ACCESS_VIEW_DESC create_uav_desc(const texture_uav_desc& desc) const = 0;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//texture1d
///////////////////////////////////////////////////////////////////////////////////////////////////

class texture1d : public texture
{
public:

	//�R���X�g���N�^
	using texture::texture;

	//srv/uavDesc�𐶐�
	D3D12_SHADER_RESOURCE_VIEW_DESC create_srv_desc(const texture_srv_desc& desc) const override;
	D3D12_UNORDERED_ACCESS_VIEW_DESC create_uav_desc(const texture_uav_desc& desc) const override;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//texture2d
///////////////////////////////////////////////////////////////////////////////////////////////////

class texture2d : public texture
{
public:

	//�R���X�g���N�^
	using texture::texture;

	//rtv/dsv/srv/uavDesc�𐶐�
	D3D12_RENDER_TARGET_VIEW_DESC create_rtv_desc(const texture_rtv_desc& desc) const override;
	D3D12_DEPTH_STENCIL_VIEW_DESC create_dsv_desc(const texture_dsv_desc& desc) const override;
	D3D12_SHADER_RESOURCE_VIEW_DESC create_srv_desc(const texture_srv_desc& desc) const override;
	D3D12_UNORDERED_ACCESS_VIEW_DESC create_uav_desc(const texture_uav_desc& desc) const override;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//texture3d
///////////////////////////////////////////////////////////////////////////////////////////////////

class texture3d : public texture
{
public:

	//�R���X�g���N�^
	using texture::texture;

	//rtv/srv/uavDesc�𐶐�
	D3D12_RENDER_TARGET_VIEW_DESC create_rtv_desc(const texture_rtv_desc& desc) const override;
	D3D12_SHADER_RESOURCE_VIEW_DESC create_srv_desc(const texture_srv_desc& desc) const override;
	D3D12_UNORDERED_ACCESS_VIEW_DESC create_uav_desc(const texture_uav_desc& desc) const override;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//texture_cube
///////////////////////////////////////////////////////////////////////////////////////////////////

class texture_cube : public texture
{
public:

	//�R���X�g���N�^
	using texture::texture;

	//rtv/dsv/srv/uavDesc�𐶐�
	D3D12_RENDER_TARGET_VIEW_DESC create_rtv_desc(const texture_rtv_desc& desc) const override;
	D3D12_DEPTH_STENCIL_VIEW_DESC create_dsv_desc(const texture_dsv_desc& desc) const override;
	D3D12_SHADER_RESOURCE_VIEW_DESC create_srv_desc(const texture_srv_desc& desc) const override;
	D3D12_UNORDERED_ACCESS_VIEW_DESC create_uav_desc(const texture_uav_desc& desc) const override;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//texture1d_array
///////////////////////////////////////////////////////////////////////////////////////////////////

class texture1d_array : public texture
{
public:

	//�R���X�g���N�^
	using texture::texture;

	//srv/uavDesc�𐶐�
	D3D12_SHADER_RESOURCE_VIEW_DESC create_srv_desc(const texture_srv_desc& desc) const override;
	D3D12_UNORDERED_ACCESS_VIEW_DESC create_uav_desc(const texture_uav_desc& desc) const override;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//texture2d_array
///////////////////////////////////////////////////////////////////////////////////////////////////

class texture2d_array : public texture
{
public:

	//�R���X�g���N�^
	using texture::texture;

	//rtv/dsv/srv/uavDesc�𐶐�
	D3D12_RENDER_TARGET_VIEW_DESC create_rtv_desc(const texture_rtv_desc& desc) const override;
	D3D12_DEPTH_STENCIL_VIEW_DESC create_dsv_desc(const texture_dsv_desc& desc) const override;
	D3D12_SHADER_RESOURCE_VIEW_DESC create_srv_desc(const texture_srv_desc& desc) const override;
	D3D12_UNORDERED_ACCESS_VIEW_DESC create_uav_desc(const texture_uav_desc& desc) const override;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//texture_cube_array
///////////////////////////////////////////////////////////////////////////////////////////////////

class texture_cube_array : public texture
{
public:

	//�R���X�g���N�^
	using texture::texture;

	//rtv/dsv/srv/uavDesc�𐶐�
	D3D12_RENDER_TARGET_VIEW_DESC create_rtv_desc(const texture_rtv_desc& desc) const override;
	D3D12_DEPTH_STENCIL_VIEW_DESC create_dsv_desc(const texture_dsv_desc& desc) const override;
	D3D12_SHADER_RESOURCE_VIEW_DESC create_srv_desc(const texture_srv_desc& desc) const override;
	D3D12_UNORDERED_ACCESS_VIEW_DESC create_uav_desc(const texture_uav_desc& desc) const override;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//top_level_acceleration_structure
/*/////////////////////////////////////////////////////////////////////////////////////////////////
D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE�ō��J�ڂ��Ȃ�
/////////////////////////////////////////////////////////////////////////////////////////////////*/

class top_level_acceleration_structure : public render::top_level_acceleration_structure, public resource
{
public:

	//�R���X�g���N�^
	top_level_acceleration_structure(render_device& device, const uint num_instances);

	//D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS��Ԃ�
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS build_inputs(const uint num_instances) const;

	//�o�b�t�@��Ԃ�
	buffer& scratch() const { return *mp_scratch; }
	buffer& instance_descs() const { return *mp_instance_descs; }

private:

	intrusive_ptr<structured_buffer>	mp_scratch;
	intrusive_ptr<structured_buffer>	mp_instance_descs;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//bottom_level_acceleration_structure
///////////////////////////////////////////////////////////////////////////////////////////////////

class bottom_level_acceleration_structure : public render::bottom_level_acceleration_structure, public resource
{
public:

	//�R���X�g���N�^
	bottom_level_acceleration_structure(render_device& device, const geometry_state& gs, const raytracing_geometry_desc* descs, const uint num_descs, const bool allow_update);

	//�W�I���g�����X�V
	void update_geometry(const geometry_state& gs);

	//D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS��Ԃ�
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS build_inputs() const;

	//�R���p�N�V����
	void set_compaction_state(render_device& device, const raytracing_compaction_state state);

	//�X�N���b�`�o�b�t�@���폜
	void release_scratch(){ return mp_scratch.reset(); }

	//�o�b�t�@��Ԃ�
	buffer& scratch() const { return *mp_scratch; }
	buffer& compacted_size() const { return *mp_compacted_size; }
	buffer& compacted_result() const { return *mp_compacted_result; }
	readback_buffer& compacted_size_readback() const { return *mp_compacted_size_readback; }

private:

	uint64_t								m_vb_address;
	uint64_t								m_ib_address;
	bool									m_allow_update;

	vector<D3D12_RAYTRACING_GEOMETRY_DESC>	m_descs;
	intrusive_ptr<structured_buffer>		mp_scratch;
	intrusive_ptr<structured_buffer>		mp_compacted_size;
	intrusive_ptr<readback_buffer>			mp_compacted_size_readback;
	intrusive_ptr<structured_buffer>		mp_compacted_result;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//sampler
///////////////////////////////////////////////////////////////////////////////////////////////////

class sampler : public render::sampler
{
public:

	//�R���X�g���N�^
	sampler(cpu_descriptor descriptor) : m_descriptor(std::move(descriptor)){}

	//View��Ԃ�
	operator D3D12_CPU_DESCRIPTOR_HANDLE() const { return m_descriptor; }

private:

	cpu_descriptor m_descriptor;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//shader_resource_view
///////////////////////////////////////////////////////////////////////////////////////////////////

class shader_resource_view : public render::shader_resource_view
{
public:

	//�R���X�g���N�^
	shader_resource_view(cpu_descriptor descriptor, resource& resource) : m_descriptor(std::move(descriptor)), mp_resource(&resource){}

	//View��Ԃ�
	operator D3D12_CPU_DESCRIPTOR_HANDLE() const { return m_descriptor; }

	//���\�[�X��Ԃ�
	resource& resource() const { return *mp_resource; }

private:

	DXGI_FORMAT			m_format;
	cpu_descriptor		m_descriptor;
	d3d12::resource*	mp_resource;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//texture_shader_resource_view
///////////////////////////////////////////////////////////////////////////////////////////////////

class texture_shader_resource_view : public shader_resource_view
{
public:

	//�R���X�g���N�^
	texture_shader_resource_view(DXGI_FORMAT format, cpu_descriptor descriptor, texture& tex, const texture_srv_desc& desc) : shader_resource_view(std::move(descriptor), tex), m_format(format), m_desc(desc){}

	//�t�H�[�}�b�g��Ԃ�
	DXGI_FORMAT format() const { return m_format; }

	//Desc��Ԃ�
	const texture_srv_desc& desc() const { return m_desc; }

private:

	DXGI_FORMAT			m_format;
	texture_srv_desc	m_desc;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//buffer_shader_resource_view
///////////////////////////////////////////////////////////////////////////////////////////////////

class buffer_shader_resource_view : public shader_resource_view
{
public:

	//�R���X�g���N�^
	buffer_shader_resource_view(cpu_descriptor descriptor, buffer& buf, const buffer_srv_desc& desc) : shader_resource_view(std::move(descriptor), buf), m_desc(desc){}

	//Desc��Ԃ�
	const buffer_srv_desc& desc() const { return m_desc; }

private:

	buffer_srv_desc m_desc;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//unordered_access_view
///////////////////////////////////////////////////////////////////////////////////////////////////

class unordered_access_view : public render::unordered_access_view
{
public:

	//�R���X�g���N�^
	unordered_access_view(cpu_descriptor descriptor, resource& resource) : m_descriptor(std::move(descriptor)), mp_resource(&resource){}

	//View��Ԃ�
	operator D3D12_CPU_DESCRIPTOR_HANDLE() const { return m_descriptor; }

	//���\�[�X��Ԃ�
	resource& resource() const { return *mp_resource; }

private:

	cpu_descriptor		m_descriptor;
	d3d12::resource*	mp_resource;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//texture_unordered_access_view
///////////////////////////////////////////////////////////////////////////////////////////////////

class texture_unordered_access_view : public unordered_access_view
{
public:

	//�R���X�g���N�^
	texture_unordered_access_view(cpu_descriptor descriptor, texture& tex, const texture_uav_desc& desc) : unordered_access_view(std::move(descriptor), tex), m_desc(desc){}

	//Desc��Ԃ�
	const texture_uav_desc& desc() const { return m_desc; }

private:

	texture_uav_desc m_desc;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//buffer_unordered_access_view
///////////////////////////////////////////////////////////////////////////////////////////////////

class buffer_unordered_access_view : public unordered_access_view
{
public:

	//�R���X�g���N�^
	buffer_unordered_access_view(cpu_descriptor descriptor, buffer& buf, const buffer_uav_desc& desc) : unordered_access_view(std::move(descriptor), buf), m_desc(desc){}

	//Desc��Ԃ�
	const buffer_uav_desc& desc() const { return m_desc; }

private:

	buffer_uav_desc m_desc;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//render_target_view
///////////////////////////////////////////////////////////////////////////////////////////////////

class render_target_view : public render::render_target_view
{
public:

	//�R���X�g���N�^
	render_target_view(DXGI_FORMAT format, cpu_descriptor descriptor, resource& resource, const texture_rtv_desc& desc) : m_format(format), m_descriptor(std::move(descriptor)), mp_resource(&resource), m_desc(desc){}

	//�t�H�[�}�b�g��Ԃ�
	DXGI_FORMAT format() const { return m_format; }

	//���\�[�X��Ԃ�
	resource& resource() const { return *mp_resource; }

	//Desc��Ԃ�
	const texture_rtv_desc& desc() const { return m_desc; }

	//Viwe��Ԃ�
	operator D3D12_CPU_DESCRIPTOR_HANDLE() const { return m_descriptor; }

private:

	DXGI_FORMAT			m_format;
	cpu_descriptor		m_descriptor;
	d3d12::resource*	mp_resource;
	texture_rtv_desc	m_desc;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//depth_stencil_view
///////////////////////////////////////////////////////////////////////////////////////////////////

class depth_stencil_view : public render::depth_stencil_view
{
public:

	//�R���X�g���N�^
	depth_stencil_view(DXGI_FORMAT format, cpu_descriptor descriptor, resource& resource, const texture_dsv_desc& desc) : m_format(format), m_descriptor(std::move(descriptor)), mp_resource(&resource), m_desc(desc){}

	//�t�H�[�}�b�g��Ԃ�
	DXGI_FORMAT format() const { return m_format; }

	//���\�[�X��Ԃ�
	resource& resource() const { return *mp_resource; }

	//Desc��Ԃ�
	const texture_dsv_desc& desc() const { return m_desc; }

	//View��Ԃ�
	operator const D3D12_CPU_DESCRIPTOR_HANDLE&() const { return m_descriptor; }
	operator const D3D12_CPU_DESCRIPTOR_HANDLE*() const { return &m_descriptor; }

private:

	DXGI_FORMAT			m_format;
	cpu_descriptor		m_descriptor;
	d3d12::resource*	mp_resource;
	texture_dsv_desc	m_desc;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//target_state
///////////////////////////////////////////////////////////////////////////////////////////////////

class target_state : public render::target_state
{
public:

	//�R���X�g���N�^
	target_state(const uint num_rtvs, render::render_target_view* rtv_ptrs[], render::depth_stencil_view* p_dsv) : render::target_state(num_rtvs, rtv_ptrs, p_dsv){}

	//RTV/DSV��Ԃ�
	render_target_view& rtv(const uint i) const { return static_cast<render_target_view&>(render::target_state::rtv(i)); }
	depth_stencil_view& dsv() const { return static_cast<depth_stencil_view&>(render::target_state::dsv()); }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//geometry_state
///////////////////////////////////////////////////////////////////////////////////////////////////

class geometry_state : public render::geometry_state
{
public:

	//�R���X�g���N�^
	geometry_state(const uint num_vbs, render::buffer* vb_ptrs[8], uint offsets[8], uint strides[8], render::buffer* p_ib) : render::geometry_state(num_vbs, vb_ptrs, offsets, strides, p_ib){}

	//���_/�C���f�b�N�X�o�b�t�@��Ԃ�
	buffer& vertex_buffer(const uint i) const { return static_cast<buffer&>(render::geometry_state::vertex_buffer(i)); }
	buffer& index_buffer() const { return static_cast<buffer&>(render::geometry_state::index_buffer()); }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//shader_resource_count
///////////////////////////////////////////////////////////////////////////////////////////////////

struct shader_resource_count
{
	uint8_t cbv, srv, uav, sampler;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//pipeline_resource_count
///////////////////////////////////////////////////////////////////////////////////////////////////

struct pipeline_resource_count
{
	shader_resource_count shader_resource_count_table[8] = {};
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//graphics_pipeline_state
/*/////////////////////////////////////////////////////////////////////////////////////////////////
rtv/dsv�t�H�[�}�b�g�Ɉˑ����邽�ߕ������Ă�K�v������.
/////////////////////////////////////////////////////////////////////////////////////////////////*/

class graphics_pipeline_state : public render::pipeline_state
{
public:

	//�R���X�g���N�^
	graphics_pipeline_state(const string& name, shader_ptr p_vs, shader_ptr p_ps, const input_layout_desc& il, const rasterizer_desc& rs, const depth_stencil_desc& dss, const blend_desc& bs);

	//�V�F�[�_��Ԃ�
	const shader_ptr& p_vs() const { return mp_vs; }
	const shader_ptr& p_ps() const { return mp_ps; }

	//�p�C�v���C���X�e�[�g��Ԃ�
	struct record
	{
		size_t						hash;
		ComPtr<ID3D12PipelineState>	p_pipeline_state;
		ComPtr<ID3D12RootSignature>	p_root_signature;
		pipeline_resource_count		pipeline_resource_count;
	};
	const record& get(render_device& device, const target_state& ts, const geometry_state* p_gs) const;

	//�p�C�v���C������Ԃ�
	const input_layout_desc& input_layout() const { return m_il; }
	const blend_desc& blend_state() const { return m_bs; }
	const rasterizer_desc& rasterizer_state() const { return m_rs; }
	const depth_stencil_desc& depth_stencil_state() const { return m_dss; }

private:

	shader_ptr				mp_vs;
	shader_ptr				mp_ps;
	input_layout_desc		m_il;
	rasterizer_desc			m_rs;
	depth_stencil_desc		m_dss;
	blend_desc				m_bs;
	mutable vector<record>	m_records;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//compute_pipeline_state
///////////////////////////////////////////////////////////////////////////////////////////////////

class compute_pipeline_state : public render::pipeline_state, public com_wrapper<ID3D12PipelineState>
{
public:

	//�R���X�g���N�^
	compute_pipeline_state(render_device& device, const string& name, shader_ptr p_cs);

	//�V�F�[�_��Ԃ�
	const shader_ptr& p_cs() const { return mp_cs; }

	//���[�g�V�O�l�`����Ԃ�
	const ComPtr<ID3D12RootSignature>& p_rs() const { return mp_rs; }

	//���\�[�X����Ԃ�
	const shader_resource_count& resource_count() const { return m_resource_count; }

private:

	shader_ptr					mp_cs;
	ComPtr<ID3D12RootSignature>	mp_rs;
	shader_resource_count		m_resource_count;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//shader
///////////////////////////////////////////////////////////////////////////////////////////////////

class shader : public render::shader
{
public:

	//�R���X�g���N�^
	shader(ComPtr<ID3DBlob> p_blob, ComPtr<ID3D12ShaderReflection> p_reflection);

	//�o�C�i����Ԃ�
	ID3DBlob& blob() const;

	//���t���N�V������Ԃ�
	ID3D12ShaderReflection& reflection() const;

private:

	ComPtr<ID3DBlob>				mp_blob;
	ComPtr<ID3D12ShaderReflection>	mp_reflection;
	//ComPtr<ID3D12LibraryReflection>	mp_reflection;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//shader_compiler
/*/////////////////////////////////////////////////////////////////////////////////////////////////
���C�u�����Ή��͂��Ă��Ȃ���.�����ς��邾��
/////////////////////////////////////////////////////////////////////////////////////////////////*/

class shader_compiler : public render::shader_compiler
{
public:

	//�R���X�g���N�^
	shader_compiler();

	//�R���p�C��
	shader_ptr compile(const wstring& filename, const wstring& entry_point, const wstring& target, const wstring& options, unordered_set<wstring>& dependent) override;

private:

	class include_handler;

	ComPtr<IDxcUtils>		mp_utils;
	ComPtr<IDxcCompiler>	mp_compiler;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//swapchain
///////////////////////////////////////////////////////////////////////////////////////////////////

class swapchain : public render::swapchain, public com_wrapper<IDXGISwapChain>
{
public:

	//�R���X�g���N�^
	swapchain(ID3D12CommandQueue& queue, const HWND hwnd, const uint2 size);

	//�o�b�N�o�b�t�@�̃C���f�b�N�X��Ԃ�
	uint back_buffer_index() const override;

	//���T�C�Y
	void resize(const uint2 size) override;

	//�t���X�N���[���̐؂�ւ�
	void toggle_fullscreen(const bool fullscreen) override;

private:

	//�o�b�N�o�b�t�@���擾
	void get_back_buffers();

private:

	bool m_hdr_mode;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//command_queue
///////////////////////////////////////////////////////////////////////////////////////////////////

class command_queue : public com_wrapper<ID3D12CommandQueue>
{
public:

	//�R���X�g���N�^
	command_queue(render_device& device, const D3D12_COMMAND_LIST_TYPE type);
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//command_list
///////////////////////////////////////////////////////////////////////////////////////////////////

class command_list : public com_wrapper<ID3D12GraphicsCommandList>
{
public:

	//�R���X�g���N�^
	command_list(render_device& device, const D3D12_COMMAND_LIST_TYPE type, ID3D12CommandAllocator& allocator);
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//command_allocator
///////////////////////////////////////////////////////////////////////////////////////////////////

class command_allocator : public com_wrapper<ID3D12CommandAllocator>
{
public:

	//�R���X�g���N�^
	command_allocator(render_device& device, const D3D12_COMMAND_LIST_TYPE type);
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//fence
///////////////////////////////////////////////////////////////////////////////////////////////////

class fence : public com_wrapper<ID3D12Fence>
{
public:

	//�R���X�g���N�^
	fence(render_device& device);
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//render_device_impl
///////////////////////////////////////////////////////////////////////////////////////////////////

class render_device_impl : public com_wrapper<ID3D12Device>
{
public:

	//�R���X�g���N�^
	render_device_impl();
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//render_device
///////////////////////////////////////////////////////////////////////////////////////////////////

class render_device : public iface::render_device, public render_device_impl
{
public:

	//�R���X�g���N�^
	render_device(const HWND hwnd);

	//�f�X�g���N�^
	~render_device();

	//���s
	void present_impl(render::swapchain& swapchain_base) override;

	//GPU����������ҋ@
	void wait_idle() override;

	//�V�F�[�_�R���p�C�����쐬
	shader_compiler_ptr create_shader_compiler() override;

	//�X���b�v�`�F�C�����쐬
	swapchain_ptr create_swapchain(const uint2 size) override;

	//�e�N�X�`�����쐬
	texture_ptr create_texture1d(const texture_format format, const uint width, const uint mip_levels, const resource_flags flags, const void* data) override;
	texture_ptr create_texture2d(const texture_format format, const uint width, const uint height, const uint mip_levels, const resource_flags flags, const void* data) override;
	texture_ptr create_texture3d(const texture_format format, const uint width, const uint height, const uint depth, const uint mip_levels, const resource_flags flags, const void* data) override;
	texture_ptr create_texture_cube(const texture_format format, const uint width, const uint height, const uint mip_levels, const resource_flags flags, const void* data) override;
	texture_ptr create_texture1d_array(const texture_format format, const uint width, const uint depth, const uint mip_levels, const resource_flags flags, const void* data) override;
	texture_ptr create_texture2d_array(const texture_format format, const uint width, const uint height, const uint depth, const uint mip_levels, const resource_flags flags, const void* data) override;
	texture_ptr create_texture_cube_array(const texture_format format, const uint width, const uint height, const uint depth, const uint mip_levels, const resource_flags flags, const void* data) override;

	//�o�b�t�@���쐬
	buffer_ptr create_structured_buffer(const uint stride, const uint num_elements, const resource_flags flags, const void* data) override;
	buffer_ptr create_byteaddress_buffer(const uint size, const resource_flags flags, const void* data) override;
	constant_buffer_ptr create_temporary_cbuffer(const uint size, const void* data) override;
	constant_buffer_ptr create_constant_buffer(const uint size, const void* data) override;
	readback_buffer_ptr create_readback_buffer(const uint size) override;
	upload_buffer_ptr create_upload_buffer(const uint size) override;

	//�T���v�����쐬
	sampler_ptr create_sampler(const sampler_desc& desc) override;

	//TLAS/BLAS���쐬
	top_level_acceleration_structure_ptr create_top_level_acceleration_structure(const uint num_instances) override;
	bottom_level_acceleration_structure_ptr create_bottom_level_acceleration_structure(const render::geometry_state& gs, const raytracing_geometry_desc* descs, const uint num_descs, const bool allow_update) override;

	//View���쐬
	render_target_view_ptr create_render_target_view(render::texture& tex, const texture_rtv_desc& desc) override;
	depth_stencil_view_ptr create_depth_stencil_view(render::texture& tex, const texture_dsv_desc& desc) override;
	shader_resource_view_ptr create_shader_resource_view(render::buffer& buf, const buffer_srv_desc& desc) override;
	shader_resource_view_ptr create_shader_resource_view(render::texture& tex, const texture_srv_desc& desc) override;
	unordered_access_view_ptr create_unordered_access_view(render::buffer& buf, const buffer_uav_desc& desc) override;
	unordered_access_view_ptr create_unordered_access_view(render::texture& tex, const texture_uav_desc& desc) override;
	cpu_descriptor create_constant_buffer_view(const D3D12_GPU_VIRTUAL_ADDRESS gpu_address, const uint size);

	//�p�C�v���C���X�e�[�g���쐬
	pipeline_state_ptr create_compute_pipeline_state(const string& name, shader_ptr p_cs) override;
	pipeline_state_ptr create_graphics_pipeline_state(const string& name, shader_ptr p_vs, shader_ptr p_ps, const input_layout_desc& il, const rasterizer_desc& rs, const depth_stencil_desc& dss, const blend_desc& bs) override;

	//�^�[�Q�b�g�X�e�[�g���쐬
	target_state_ptr create_target_state(const uint num_rtvs, render::render_target_view* rtv_ptrs[], render::depth_stencil_view* p_dsv) override;

	//�W�I���g���X�e�[�g���쐬
	geometry_state_ptr create_geometry_state(const uint num_vbs, render::buffer* vb_ptrs[8], uint offsets[8], uint strides[8], render::buffer* p_ib) override;

	//�o�C���h���X���\�[�X�Ƃ��ēo�^
	void register_bindless(render::sampler& sampler) override;
	void register_bindless(render::constant_buffer& cbv) override;
	void register_bindless(render::shader_resource_view& srv) override;

	//�o�C���h���X���\�[�X�̓o�^����
	void unregister_bindless(render::sampler& sampler) override;
	void unregister_bindless(render::constant_buffer& cbv) override;
	void unregister_bindless(render::shader_resource_view& srv) override;

	//���O��ݒ�
	void set_name(render::buffer& buf, const wchar_t* name) override;
	void set_name(render::upload_buffer& buf, const wchar_t* name) override;
	void set_name(render::constant_buffer& buf, const wchar_t* name) override;
	void set_name(render::readback_buffer& buf, const wchar_t* name) override;
	void set_name(render::texture& tex, const wchar_t* name) override;
	void set_name(render::top_level_acceleration_structure& tlas, const wchar_t* name) override;
	void set_name(render::bottom_level_acceleration_structure& blas, const wchar_t* name) override;

	//�o�b�t�@�X�V�p�̃|�C���^��Ԃ�
	void* get_update_buffer_pointer(const uint size) override;

	//���s�\���̃`�F�b�N�Ǝ��s�ɕK�v�ȃf�[�^���쐬
	void* validate(const render::pipeline_state& ps, const unordered_map<string, bind_resource>& resources) override;
	void* validate(const render::pipeline_state& ps, const render::target_state& ts, const render::geometry_state* p_gs, const unordered_map<string, bind_resource>& resources) override;
	void* validate_impl(const shader** shader_ptrs, const shader_resource_count* resource_count_table, const uint num_shaders, const unordered_map<string, bind_resource>& resources);

	//���\�[�X/�o�b�t�@/�e�N�X�`�����R�s�[
	void copy_resource(resource& dst, resource& src);
	void copy_buffer(buffer& dst, const uint dst_offset, upload_buffer& src, const uint src_offset, const uint size);
	void copy_texture(texture& dst, upload_buffer& src, const D3D12_PLACED_SUBRESOURCE_FOOTPRINT* footprints, const uint num_subresources);

	//���[�g�V�O�l�`�����쐬
	pair<ComPtr<ID3D12RootSignature>, pipeline_resource_count> create_root_signature(const shader** shaders, const uint num_shaders);

	//�R�}���h�L���[/���X�g/�f�X�N���v�^�q�[�v��Ԃ�(�O�����C�u�������g��)
	command_list& get_command_list() { return m_graphics_command_list; }
	command_queue& get_command_queue() { return m_graphics_command_queue; }
	sv_descriptor_heap& get_descriptor_heap() { return m_sv_cbv_srv_uav_descriptor_heap; }

private:
	
	static constexpr uint root_constant_bind_point = 0;
	static constexpr uint root_constant_bind_space = 1;
	static constexpr uint root_tlas_srv_bind_point = 0;
	static constexpr uint root_tlas_srv_bind_space = 1;
	static constexpr uint static_sampler_bind_point = 0;
	static constexpr uint static_sampler_bind_space = 1;

	static constexpr uint command_buffer_size				= mebi(1);
	static constexpr uint rtv_descriptor_size				= 128;
	static constexpr uint dsv_descriptor_size				= 128;
	static constexpr uint sampler_descriptor_size			= kibi(1);
	static constexpr uint cbv_srv_uav_descriptor_size		= kibi(16);
	static constexpr uint upload_buffer_size				= mebi(128);
	static constexpr uint upload_buffer_alignment			= 256;
	static constexpr uint bindless_sampler_size				= kibi(1);
	static constexpr uint bindless_cbv_srv_uav_size			= kibi(16);

	struct bind_info : immovable<bind_info>
	{
		uint8_t								root_constant_index;
		uint8_t								root_tlas_srv_index;
		top_level_acceleration_structure*	p_tlas;
		D3D12_GPU_DESCRIPTOR_HANDLE			sampler_heap_start;
		D3D12_GPU_DESCRIPTOR_HANDLE			cbv_srv_uav_heap_start;
		uint								num_views;

		//�X�e�[�g�g���b�L���O�̂��߂ɕK�v
		struct view
		{
			void set(const void* ptr, const bool is_srv, const bool for_ps){ val = reinterpret_cast<size_t>(ptr) | (uint64_t(for_ps) << 1) | uint64_t(is_srv); }
			template<class T> const T &get() const { return *reinterpret_cast<const T*>(val & ~0x3); }
			bool is_srv() const { return val & 0x1; }
			bool for_ps() const { return val & 0x2; }
			uint64_t val;
		};
		view views[];
	};

private:

	HWND							m_hwnd;

	descriptor_heap					m_rtv_descriptor_heap;
	descriptor_heap					m_dsv_descriptor_heap;
	descriptor_heap					m_sampler_descriptor_heap;
	descriptor_heap					m_cbv_srv_uav_descriptor_heap;
	sv_descriptor_heap				m_sv_sampler_descriptor_heap;
	sv_descriptor_heap				m_sv_cbv_srv_uav_descriptor_heap;

	command_allocator				m_copy_command_allocator;
	command_queue					m_copy_command_queue;
	command_list					m_copy_command_list;
	fence							m_copy_fence;
	HANDLE							m_copy_event;

	command_allocator				m_graphics_command_allocator[2];
	command_queue					m_graphics_command_queue;
	command_list					m_graphics_command_list;
	fence							m_graphics_fence;
	HANDLE							m_graphics_event;
	uint64_t						m_graphics_fence_value;
	uint64_t						m_graphics_fence_prev_value;

	upload_buffer					m_upload_buffer;
	lockfree_ring_allocator			m_upload_buffer_allocator;

	ComPtr<ID3D12CommandSignature>	mp_draw_indirect_cs;
	ComPtr<ID3D12CommandSignature>	mp_draw_indexed_indirect_cs;
	ComPtr<ID3D12CommandSignature>	mp_dispatch_indirect_cs;

	vector<bindless_resource*>		m_bindless_sampler;
	vector<bindless_resource*>		m_bindless_cbv_srv_uav;

	//std::deque<fence>				m_compute_fences;
	//std::deque<command_queue>		m_compute_command_queues;
	//std::deque<command_list>		m_compute_command_lists;
	//std::deque<command_allocator>	m_compute_command_allocators;

private:

	struct begin_barrier
	{
		begin_barrier(resource& resource, uint subresource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after, begin_barrier* p_next) : p_resource(&resource), subresource(subresource), before(before), after(after), p_next(p_next){}
		D3D12_RESOURCE_STATES	before;
		D3D12_RESOURCE_STATES	after;
		resource*				p_resource;
		uint					subresource;
		begin_barrier*			p_next;
	};
	struct end_barrier
	{
		end_barrier(resource* p_resource, uint time) : p_resource(p_resource), before(), after(), time(time){}
		end_barrier(resource& resource, uint subresource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after, uint time, bool not_end) : p_resource(&resource), subresource(subresource), before(before), after(after), time(time), not_end(not_end){}
		D3D12_RESOURCE_STATES	before;
		D3D12_RESOURCE_STATES	after;
		uint					time;
		resource*				p_resource;
		uint					subresource;
		bool					not_end;
	};
	deque<end_barrier*>		m_end_barrier_table;
	vector<begin_barrier*>	m_begin_barrier_table;

private:

	vector<bottom_level_acceleration_structure*>	m_blas_ptrs;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace d3d12

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#include"render_device/render_device-impl.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

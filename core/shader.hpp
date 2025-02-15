
#pragma once

#ifndef NN_RENDER_CORE_SHADER_HPP
#define NN_RENDER_CORE_SHADER_HPP

#include"utility.hpp"
#include"../../vector.hpp"
#include"../../utility/json.hpp"
#include"../../utility/queue.hpp"
#include"../../utility/filesystem_watcher.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//shader_file
///////////////////////////////////////////////////////////////////////////////////////////////////

class shader_file
{
public:

	//�R���X�g���N�^
	shader_file(unordered_map<string, pipeline_state_ptr> ps_ptrs = {}) : m_ps_ptrs(std::move(ps_ptrs)){}

	//�p�C�v���C���X�e�[�g��Ԃ�
	const pipeline_state_ptr& get(const string& name){ return m_ps_ptrs[name]; }

	//�L��/����/�X�V�\��
	bool is_valid() const { return m_ps_ptrs.size(); }
	bool is_invalid() const { return m_ps_ptrs.empty(); }
	bool has_update() const { return bool(mp_update.load()); }

private:

	friend class shader_manager;
	friend class shader_file_holder;

	std::atomic<shared_ptr<shader_file>>		mp_update;
	unordered_map<string, pipeline_state_ptr>	m_ps_ptrs;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//shader_file_holder
///////////////////////////////////////////////////////////////////////////////////////////////////

class shader_file_holder
{
public:

	//�R���X�g���N�^
	shader_file_holder(shared_ptr<shader_file> p_file = nullptr) : mp_file(std::move(p_file)){}

	//�p�C�v���C���X�e�[�g��Ԃ�
	const pipeline_state_ptr& get(const string& name){ return mp_file->get(name); }

	//��/�L��/����/�X�V�\��
	bool is_empty() const { return (mp_file == nullptr); }
	bool is_valid() const { return mp_file->is_valid(); }
	bool is_invalid() const { return mp_file->is_invalid(); }
	bool has_update() const { return mp_file->has_update(); }

	//�X�V
	void update();

private:

	shared_ptr<shader_file> mp_file;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//shader_manager
/*/////////////////////////////////////////////////////////////////////////////////////////////////
hlsl��,�ǂ�sdf�Ɉˑ�����Ă��邩���L�^.
hlsl�̕ύX�����m�����Ƃ���,�ˑ�����Ă�sdf�̍ăR���p�C�����s��.
sdf�̕ύX�����m�����Ƃ���,���R�ăR���p�C�����s��.
/////////////////////////////////////////////////////////////////////////////////////////////////*/

class shader_manager
{
public:

	//�R���X�g���N�^
	shader_manager(iface::render_device& device);

	//�f�X�g���N�^
	~shader_manager();

	//�V�F�[�_�t�@�C����Ԃ�
	shader_file_holder create(wstring filename);

private:

	struct record
	{
		shared_ptr<shader_file>			p_file;
		unordered_set<wstring>			dependent; //�ˑ����Ă���hlsl(�Ȃ�)
		std::filesystem::file_time_type	last_write_time;
	};
	record* get(const wstring& filename);

	//�V�F�[�_�t�@�C�����쐬
	shared_ptr<shader_file> create(iface::render_device& device, const wstring& filename, unordered_set<wstring>& dependent);

private:

	shader_compiler_ptr								mp_compiler;
	unordered_map<wstring, unordered_set<wstring>>	m_depended; //hlsl���g���Ă���sdf�̃��X�g
	unordered_map<wstring, record>					m_records;
	vector<wstring>									m_failed_files;
	std::shared_mutex								m_records_mtx;
	std::condition_variable							m_cv;
	std::thread										m_worker;
	std::thread										m_watcher;
	std::queue<wstring>								m_queue;
	std::mutex										m_queue_mtx;
	nn::util::filesystem_watcher					m_watcher_impl;
	bool											m_terminate;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#include"shader/shader_manager-impl.hpp"
#include"shader/shader_file_holder-impl.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

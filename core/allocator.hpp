
#pragma once

#ifndef NN_RENDER_CORE_ALLOCATOR_HPP
#define NN_RENDER_CORE_ALLOCATOR_HPP

#include<vector>
#include<cassert>

#include"../../memory/memory_allocator_impl.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//default_allocator
///////////////////////////////////////////////////////////////////////////////////////////////////

class default_allocator : private nn::memory_allocator_impl
{
public:

	//�R���X�g���N�^
	using memory_allocator_impl::memory_allocator_impl;

	//���蓖��
	size_t allocate(const size_t size)
	{
		assert(size > 0);
		std::lock_guard<std::mutex> lock(m_mtx);
		return memory_allocator_impl::allocate(size);
	}

	//���
	void deallocate(const size_t index)
	{
		std::lock_guard<std::mutex> lock(m_mtx);
		return memory_allocator_impl::deallocate(index);
	}

	//���蓖�ăT�C�Y��Ԃ�
	size_t allocate_size(const size_t index) const
	{
		return memory_allocator_impl::allocate_size(index);
	}

	//�����̃t���[�u���b�N�̃C���f�b�N�X��Ԃ�
	size_t last_free_block_index() const
	{
		return memory_allocator_impl::last_free_block_index();
	}

private:

	std::mutex m_mtx;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//FixedSizeAllocator
///////////////////////////////////////////////////////////////////////////////////////////////////

template<bool thread_safe> class FixedSizeAllocator
{
public:

	//�R���X�g���N�^
	FixedSizeAllocator(const size_t max_num) : m_size(max_num), m_stack(max_num)
	{
		for(size_t i = 0; i < max_num; i++){ m_stack[i] = max_num - i - 1; }
	}

	//���蓖��
	size_t allocate()
	{
		return m_stack[--m_size];
	}

	//���
	void deallocate(const size_t pos)
	{
		m_stack[m_size++] = pos;
	}

private:

	std::conditional_t<thread_safe, std::atomic_size_t, size_t>	m_size;
	std::vector<size_t>											m_stack;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

using fixed_size_allocator = FixedSizeAllocator<false>;
using lockfree_fixed_size_allocator = FixedSizeAllocator<true>;

///////////////////////////////////////////////////////////////////////////////////////////////////
//RingAllocator
///////////////////////////////////////////////////////////////////////////////////////////////////

template<bool thread_safe> class RingAllocator
{
public:

	//�R���X�g���N�^
	RingAllocator(const size_t size) : m_cur(), m_size(size)
	{
		m_beg[0] = size;
		m_beg[1] = 0;
	}

	//���蓖��
	size_t allocate(const size_t size);

	//���
	void dellocate()
	{
		m_beg[0] = m_beg[1];
		m_beg[1] = m_cur;

		//allocate���ꂸ��deallocate������(m_beg[0]==m_cur)�̏�ԂɂȂ�
		//���̏�Ԃ�full�̏�Ԃ�\���̂Ŕ�����K�v������
		if(m_beg[0] == m_beg[1])
		{
			m_beg[0] = m_size;
			m_beg[1] = m_cur = 0;
		}
	}

private:

	size_t m_beg[2];
	size_t m_size;

	std::conditional_t<thread_safe, std::atomic_size_t, size_t> m_cur;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

//���蓖��
template<> inline size_t RingAllocator<false>::allocate(const size_t size)
{
	assert(m_cur != m_beg[0]); //�t��

	if(size == 0)
		return m_cur;

	size_t end = m_beg[0];
	if(m_beg[0] < m_cur)
	{
		if(m_cur + size > m_size)
			m_cur = 0;
		else
			end = m_size;
	}
	assert(m_cur + size <= end);
	return std::exchange(m_cur, m_cur + size);
}

template<> inline size_t RingAllocator<true>::allocate(const size_t size)
{
	if(size == 0)
		return m_cur.load(std::memory_order_relaxed);

	while(true)
	{
		size_t old = m_cur;
		size_t cur = old;

		assert(cur != m_beg[0]);
		
		size_t end = m_beg[0];
		if(m_beg[0] < cur)
		{
			if(cur + size > m_size)
				cur = 0;
			else
				end = m_size;
		}
		assert(cur + size <= end);
		if(m_cur.compare_exchange_strong(old, cur + size))
			return cur;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

using ring_allocator = RingAllocator<false>;
using lockfree_ring_allocator = RingAllocator<true>;

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

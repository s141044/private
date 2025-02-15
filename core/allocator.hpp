
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

	//コンストラクタ
	using memory_allocator_impl::memory_allocator_impl;

	//割り当て
	size_t allocate(const size_t size)
	{
		assert(size > 0);
		std::lock_guard<std::mutex> lock(m_mtx);
		return memory_allocator_impl::allocate(size);
	}

	//解放
	void deallocate(const size_t index)
	{
		std::lock_guard<std::mutex> lock(m_mtx);
		return memory_allocator_impl::deallocate(index);
	}

	//割り当てサイズを返す
	size_t allocate_size(const size_t index) const
	{
		return memory_allocator_impl::allocate_size(index);
	}

	//末尾のフリーブロックのインデックスを返す
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

	//コンストラクタ
	FixedSizeAllocator(const size_t max_num) : m_size(max_num), m_stack(max_num)
	{
		for(size_t i = 0; i < max_num; i++){ m_stack[i] = max_num - i - 1; }
	}

	//割り当て
	size_t allocate()
	{
		return m_stack[--m_size];
	}

	//解放
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

	//コンストラクタ
	RingAllocator(const size_t size) : m_cur(), m_size(size)
	{
		m_beg[0] = size;
		m_beg[1] = 0;
	}

	//割り当て
	size_t allocate(const size_t size);

	//解放
	void dellocate()
	{
		m_beg[0] = m_beg[1];
		m_beg[1] = m_cur;

		//allocateされずにdeallocateされると(m_beg[0]==m_cur)の状態になり
		//この状態はfullの状態を表すので避ける必要がある
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

//割り当て
template<> inline size_t RingAllocator<false>::allocate(const size_t size)
{
	assert(m_cur != m_beg[0]); //フル

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

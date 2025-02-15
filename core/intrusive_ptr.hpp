
#pragma once

#ifndef NN_RENDER_CORE_INTRUSIVE_PTR_HPP
#define NN_RENDER_CORE_INTRUSIVE_PTR_HPP

#include<vector>
#include<cassert>

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//intrusive_ptr
/*////////////////////////////////////////////////////////////////////////////////////////////////
T�͈ȉ����Ăׂ�K�v������
int intrusive_ptr_add_ref(T*)�ŎQ�ƃJ�E���g�𑝂₷
int intrusive_ptr_release(T*)�ŎQ�ƃJ�E���g�����炵�ĎQ�ƃJ�E���g��Ԃ�
///////////////////////////////////////////////////////////////////////////////////////////////////
���ۂ�delete����̂�T���ōs��
///////////////////////////////////////////////////////////////////////////////////////////////////
U*�ŏ���������Ƃ����L���͈ڏ������
/////////////////////////////////////////////////////////////////////////////////////////////////*/

template<class T> class intrusive_ptr
{
public:

	//�R���X�g���N�^
	intrusive_ptr(std::nullptr_t = nullptr) : m_ptr()
	{
	}
	intrusive_ptr(intrusive_ptr&& other) : m_ptr(other.m_ptr)
	{
		other.m_ptr = nullptr;
	}
	intrusive_ptr(const intrusive_ptr& other) : m_ptr(other.m_ptr)
	{
		intrusive_ptr_add_ref(m_ptr);
	}
	template<class U> intrusive_ptr(intrusive_ptr<U>&& other) : m_ptr(static_cast<T*>(other.m_ptr))
	{
		other.m_ptr = nullptr;
	}
	template<class U> intrusive_ptr(const intrusive_ptr<U>& other) : m_ptr(static_cast<T*>(other.m_ptr))
	{
		intrusive_ptr_add_ref(m_ptr);
	}
	template<class U> intrusive_ptr(U* ptr) : m_ptr(static_cast<T*>(ptr))
	{
	}

	//�f�X�g���N�^
	~intrusive_ptr()
	{
		if(m_ptr)
		{
			const int count = intrusive_ptr_release(m_ptr);
			assert(count >= 0);
			m_ptr = nullptr;
		}
	}

	//���
	intrusive_ptr& operator=(std::nullptr_t)
	{
		this->~intrusive_ptr();
		return *this;
	}
	intrusive_ptr& operator=(intrusive_ptr&& other)
	{
		if(&other != this)
		{
			this->~intrusive_ptr();
			m_ptr = other.m_ptr;
			other.m_ptr = nullptr;
		}
		return *this;
	}
	intrusive_ptr& operator=(const intrusive_ptr& other)
	{
		if(&other != this)
		{
			this->~intrusive_ptr();
			if(other.m_ptr)
			{
				m_ptr = other.m_ptr;
				intrusive_ptr_add_ref(m_ptr);
			}
		}
		return *this;
	}
	template<class U> intrusive_ptr& operator=(intrusive_ptr<U>&& other)
	{
		if(&other != this)
		{
			this->~intrusive_ptr();
			m_ptr = static_cast<T*>(other.m_ptr);
			other.m_ptr = nullptr;
		}
		return *this;
	}
	template<class U> intrusive_ptr& operator=(const intrusive_ptr<U>& other)
	{
		if(&other != this)
		{
			this->~intrusive_ptr();
			if(other.m_ptr)
			{
				m_ptr = static_cast<T*>(other.m_ptr);
				intrusive_ptr_add_ref(m_ptr);
			}
		}
		return *this;
	}
	template<class U> intrusive_ptr& operator=(U* other)
	{
		if(other != m_ptr)
		{
			this->~intrusive_ptr();
			m_ptr = static_cast<T*>(other);
		}
		return *this;
	}

	//���Z�b�g
	void reset()
	{
		this->~intrusive_ptr();
	}
	template<class U> void reset(U* ptr)
	{
		operator=(intrusive_ptr(ptr));
	}

	//�A�h���X��Ԃ�
	T* get() const
	{
		return m_ptr;
	}
	T* operator->() const
	{
		return m_ptr;
	}

	//�ԐڎQ��
	T& operator*() const
	{
		return *m_ptr;
	}

	//��nullptr��
	operator bool() const
	{
		return m_ptr;
	}

	//��r
	bool operator==(std::nullptr_t) const
	{
		return m_ptr == nullptr;
	}
	bool operator!=(std::nullptr_t) const
	{
		return m_ptr != nullptr;
	}
	template<class U> bool operator==(const intrusive_ptr<U>& other) const
	{
		return m_ptr == other.m_ptr;
	}
	template<class U> bool operator!=(const intrusive_ptr<U>& other) const
	{
		return m_ptr != other.m_ptr;
	}

private:

	T* m_ptr;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif

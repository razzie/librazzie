/*
Copyright (C) 2016 - Gábor "Razzie" Görzsöny

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE
*/

#pragma once

#include <mutex>
#include <type_traits>
#include "raz/bitset.hpp"

namespace raz
{
	class IMemoryPool
	{
	public:
		virtual ~IMemoryPool() = default;
		virtual void* allocate(size_t bytes) = 0;
		virtual void  deallocate(void* ptr, size_t bytes) = 0;
		virtual size_t getFreeMemory() const = 0;
		virtual size_t getUsedMemory() const = 0;

		template<class T, class... Args>
		T* create(Args... args)
		{
			raz::Allocator<T> alloc(this);
			T* t = std::allocator_traits<raz::Allocator<T>>::allocate(alloc, 1);
			std::allocator_traits<raz::Allocator<T>>::construct(alloc, t, std::forward<Args>(args)...);
			return t;
		}

		template<class T>
		void destroy(T* t)
		{
			raz::Allocator<T> alloc(this);
			std::allocator_traits<raz::Allocator<T>>::destroy(alloc, t);
			std::allocator_traits<raz::Allocator<T>>::deallocate(alloc, t, 1);
		}
	};

	template<size_t SIZE, size_t ALIGNMENT = 128, class Mutex = std::mutex>
	class MemoryPool : public IMemoryPool
	{
		static_assert(SIZE % ALIGNMENT == 0, "Incorrect alignment");

	public:
		MemoryPool() = default;
		MemoryPool(const MemoryPool&) = delete;
		~MemoryPool() = default;

		MemoryPool& operator=(const MemoryPool&) = delete;

		virtual void* allocate(size_t bytes)
		{
			std::lock_guard<Lock> guard(m_lock);

			auto available = m_chunks.falsebits();
			auto begin_iter = available.begin();
			auto end_iter = available.end();

			if (begin_iter == end_iter)
				throw std::bad_alloc();

			const size_t chunks = ((bytes - 1) / ALIGNMENT) + 1;
			size_t current_chunk_pos = *begin_iter;
			size_t current_chunk_size = 0;

			for (auto iter = begin_iter; iter != end_iter; ++iter)
			{
				if (*iter == current_chunk_pos + current_chunk_size)
				{
					++current_chunk_size;
					if (current_chunk_size == chunks)
					{
						for (size_t i = current_chunk_pos; i < current_chunk_pos + current_chunk_size; ++i)
						{
							m_chunks.set(i);
						}
						return m_memory + (current_chunk_pos * ALIGNMENT);
					}
				}
				else
				{
					current_chunk_pos = *iter;
					current_chunk_size = 1;
				}
			}

			throw std::bad_alloc();
		}

		virtual void deallocate(void* ptr, size_t bytes)
		{
			std::lock_guard<Lock> guard(m_lock);

			const size_t chunks = ((bytes - 1) / ALIGNMENT) + 1;
			const size_t starting_chunk = (static_cast<char*>(ptr) - m_memory) / ALIGNMENT;

			for (size_t i = starting_chunk; i < starting_chunk + chunks; ++i)
			{
				m_chunks.unset(i);
			}
		}

		virtual size_t getFreeMemory() const
		{
			return (m_chunks.falsebits().count() * ALIGNMENT);
		}

		virtual size_t getUsedMemory() const
		{
			return (m_chunks.truebits().count() * ALIGNMENT);
		}

	private:
		struct DummyMutex
		{
			void lock() {};
			void unlock() {};
		};

		typedef std::conditional_t<std::is_same<Mutex, void>::value, DummyMutex, Mutex> Lock;

		Bitset<SIZE / ALIGNMENT> m_chunks;
		Lock m_lock;
		char m_memory[SIZE];
	};

	template<class T>
	class Allocator
	{
	public:
		typedef T value_type;

		template<class U>
		struct rebind
		{
			typedef Allocator<U> other;
		};

		Allocator(IMemoryPool* memory = nullptr) : m_memory(memory)
		{
		}

		template<class U>
		Allocator(const Allocator<U>& other) : m_memory(other.m_memory)
		{
		}

		Allocator& operator=(const Allocator&) = delete;

		T* allocate(size_t n)
		{
			if (m_memory)
				return reinterpret_cast<T*>(m_memory->allocate(n * sizeof(T)));
			else
				return reinterpret_cast<T*>(::operator new(n * sizeof(T)));
		}

		void deallocate(T* ptr, size_t n)
		{
			if (m_memory)
				m_memory->deallocate(ptr, n * sizeof(T));
			else
				delete reinterpret_cast<void*>(ptr);
		}

		IMemoryPool* getMemoryPool() const
		{
			return m_memory;
		}

		template<class U>
		bool operator==(const Allocator<U>& other) const
		{
			return (m_memory == other.m_memory);
		}

		template<class U>
		bool operator!=(const Allocator<U>& other) const
		{
			return (m_memory != other.m_memory);
		}

	private:
		mutable IMemoryPool* m_memory;

		template<class U>
		friend class Allocator;
	};

	namespace literal
	{
		constexpr unsigned long long operator"" _KB(unsigned long long size)
		{
			return (size * 1024);
		}

		constexpr unsigned long long operator"" _MB(unsigned long long size)
		{
			return (size * 1024_KB);
		}

		constexpr unsigned long long operator"" _GB(unsigned long long size)
		{
			return (size * 1024_MB);
		}
	}
}

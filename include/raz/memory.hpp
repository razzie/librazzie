/*
Copyright (C) 2016 - G�bor "Razzie" G�rzs�ny

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

#include "raz/bitset.hpp"

namespace raz
{
	class IMemoryPool
	{
	public:
		virtual ~IMemoryPool() = default;
		virtual void* allocate(size_t bytes) = 0;
		virtual void  deallocate(void* ptr, size_t bytes) = 0;
	};

	template<size_t SIZE, size_t ALIGNMENT = 128>
	class MemoryPool : public IMemoryPool
	{
		static_assert(SIZE % ALIGNMENT == 0, "Incorrect alignment");

	public:
		MemoryPool() = default;
		MemoryPool(const MemoryPool&) = delete;
		~MemoryPool() = default;

		virtual void* allocate(size_t bytes)
		{
			auto available = m_chunks.falsebits();
			auto begin_iter = available.begin();
			auto end_iter = available.end();

			if (begin_iter == end_iter)
				throw std::bad_alloc();

			const size_t chunks = ((bytes - 1) / (ALIGNMENT)) + 1;
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
			const size_t chunks = ((bytes - 1) / (ALIGNMENT)) + 1;
			const size_t starting_chunk = (static_cast<char*>(ptr) - m_memory) / ALIGNMENT;

			for (size_t i = starting_chunk; i < starting_chunk + chunks; ++i)
			{
				m_chunks.unset(i);
			}
		}

	private:
		Bitset<SIZE / ALIGNMENT> m_chunks;
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

		Allocator(IMemoryPool& memory) : m_memory(&memory)
		{
		}

		template<class U>
		Allocator(const Allocator<U>& other) : m_memory(&other.getMemoryPool())
		{
		}

		T* allocate(size_t n)
		{
			return reinterpret_cast<T*>(m_memory->allocate(n * sizeof(T)));
		}

		void deallocate(T* ptr, size_t n)
		{
			m_memory->deallocate(ptr, n * sizeof(T));
		}

		IMemoryPool& getMemoryPool() const
		{
			return *m_memory;
		}

		template<class U>
		bool operator==(const Allocator<U>& other) const
		{
			return (m_memory == &other.getMemoryPool());
		}

		template<class U>
		bool operator!=(const Allocator<U>& other) const
		{
			return (m_memory != &other.getMemoryPool());
		}

	private:
		mutable IMemoryPool* m_memory;
	};
}

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

#include <vector>
#include "raz/memory.hpp"

namespace raz
{
	template<class T>
	class CallbackSystem;

	template<class T>
	class Callback
	{
	public:
		typedef T ValueType;

		Callback(CallbackSystem<T>& system) : m_system(&system)
		{
			m_system->bind(this);
		}

		Callback(const Callback& other) : m_system(other.m_system)
		{
			if (m_system) m_system->bind(this);
		}

		~Callback()
		{
			if (m_system) m_system->unbind(this);
		}

		virtual void handle(const T& t) = 0;

	private:
		friend class CallbackSystem<T>;

		CallbackSystem<T>* m_system;
	};

	template<class T>
	class CallbackSystem
	{
	public:
		CallbackSystem() :
			m_handling_recursion(0)
		{
		}

		explicit CallbackSystem(IMemoryPool& memory) :
			m_callbacks(memory),
			m_inserted_callbacks(memory),
			m_removed_callbacks(memory),
			m_handling_recursion(0)
		{
		}

		CallbackSystem(CallbackSystem&& other) :
			m_callbacks(std::move(other.m_callbacks)),
			m_inserted_callbacks(std::move(other.m_inserted_callbacks)),
			m_removed_callbacks(std::move(other.m_removed_callbacks))
			m_handling_recursion(0)
		{
			processInsertedCallbacks();
			processRemovedCallbacks();

			for (auto callback : m_callbacks)
			{
				callback->m_system = this;
			}
		}

		~CallbackSystem()
		{
			processInsertedCallbacks();
			processRemovedCallbacks();

			for (auto callback : m_callbacks)
			{
				callback->m_system = nullptr;
			}
		}

		void handle(const T& t)
		{
			if (m_handling_recursion == 0)
			{
				++m_handling_recursion;
				processInsertedCallbacks();
				processRemovedCallbacks();
			}

			for (auto callback : m_callbacks)
			{
				callback->handle(t);
			}

			--m_handling_recursion;
		}

	private:
		friend class Callback<T>;

		void bind(Callback<T>* callback)
		{
			m_inserted_callbacks.push_back(callback);
		}

		void unbind(Callback<T>* callback)
		{
			m_removed_callbacks.push_back(callback);
		}

	private:
		typedef std::vector<Callback<T>*, raz::Allocator<Callback<T>*>> CallbackContainer;

		CallbackContainer m_callbacks;
		CallbackContainer m_inserted_callbacks;
		CallbackContainer m_removed_callbacks;
		int m_handling_recursion;

		void processInsertedCallbacks()
		{
			m_callbacks.insert(m_callbacks.end(), m_inserted_callbacks.begin(), m_inserted_callbacks.end());
			m_inserted_callbacks.clear();
		}

		void processRemovedCallbacks()
		{
			for (auto callback : m_removed_callbacks)
			{
				for (auto it = m_callbacks.begin(), end = m_callbacks.end(); it != end; ++it)
				{
					if (*it == callback)
					{
						m_callbacks.erase(it);
						break;
					}
				}
			}
			m_removed_callbacks.clear();
		}
	};
}

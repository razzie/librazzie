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

#include <mutex>
#include <vector>

namespace raz
{
	template<class T>
	class CallbackSystem;

	template<class T>
	class Callback
	{
	public:
		typedef void(Callback::*Handler)(const T&); // should be a member function of derived class

		template<class Derived>
		Callback(CallbackSystem<T>& system, void(Derived::*handler)(const T&)) :
			m_system(&system),
			m_handler(static_cast<Handler>(handler))
		{
			m_system->bind(this);
		}

		~Callback()
		{
			m_system->unbind(this);
		}

		void handle(const T& t)
		{
			(*this.*m_handler)(t);
		}

	private:
		CallbackSystem<T>* m_system;
		Handler m_handler;
	};

	template<class T>
	class CallbackSystem // must always live longer than the bound callbacks
	{
	public:
		void handle(const T& t) const
		{
			std::lock_guard<decltype(m_lock)> guard(m_lock);
			for (auto callback : m_callbacks)
			{
				callback->handle(t); // adding or deleting a callback here causes a deadlock
			}
		}

	private:
		mutable std::mutex m_lock;
		std::vector<Callback<T>*> m_callbacks;

		friend class Callback<T>;

		void bind(Callback<T>* callback)
		{
			std::lock_guard<decltype(m_lock)> guard(m_lock);
			m_callbacks.push_back(callback);
		}

		void unbind(Callback<T>* callback)
		{
			std::lock_guard<decltype(m_lock)> guard(m_lock);
			for (auto it = m_callbacks.begin(), end = m_callbacks.end(); it != end; ++it)
			{
				if (*it == callback)
				{
					m_callbacks.erase(it);
					return;
				}
			}
		}
	};
}

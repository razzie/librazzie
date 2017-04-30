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

#include <exception>
#include <functional>
#include <future>
#include <mutex>
#include <list>
#include <thread>

namespace raz
{
	template<class T>
	class Thread
	{
	public:
		template<class... Args>
		Thread(Args... args)
		{
			m_thread = std::thread(&Thread<T>::run<Args...>, this, std::forward<Args>(args)...);
		}

		Thread(const Thread&) = delete;

		Thread& operator=(const Thread&) = delete;

		~Thread()
		{
			m_exit_token.set_value();
			m_thread.join();
		}

		template<class... Args>
		void operator()(Args... args)
		{
			std::lock_guard<std::mutex> guard(m_mutex);
			m_call_queue.emplace_back([args...](T& object) { object(args...); });
		}

	private:
		typedef std::function<void(T&)> ForwardedCall;

		std::thread m_thread;
		std::promise<void> m_exit_token;
		std::mutex m_mutex;
		std::list<ForwardedCall> m_call_queue;

		template<class... Args>
		class HasParenthesisOp
		{
			template<class U>
			static auto test(bool) -> decltype(std::declval<U>()(std::declval<Args>()...), void(), std::true_type{})
			{
				return {};
			}

			template<class U>
			static auto test(int) -> std::false_type
			{
				return {};
			}

		public:
			static constexpr bool value = decltype(test<T>(true))::value;
		};

		template<bool>
		void loop(T&);

		template<>
		void loop<true>(T& object)
		{
			object();
		}

		template<>
		void loop<false>(T& object)
		{
		}

		template<bool>
		void error(T&, std::exception&);

		template<>
		void error<true>(T& object, std::exception& e)
		{
			object(e);
		}

		template<>
		void error<false>(T& object, std::exception& e)
		{
		}

		template<class... Args>
		void run(Args... args)
		{
			alignas(alignof(T)) char object_storage[sizeof(T)];

			try
			{
				T* object = new (object_storage) T(std::forward<Args>(args)...);

				std::future<void> exit_token = m_exit_token.get_future();
				std::list<ForwardedCall> call_queue;

				for (;;)
				{
					m_mutex.lock();
					std::swap(m_call_queue, call_queue);
					m_mutex.unlock();

					for (auto& call : call_queue)
						call(*object);

					call_queue.clear();

					loop<HasParenthesisOp<>::value>(*object);

					auto exit_status = exit_token.wait_for(std::chrono::milliseconds(1));
					if (exit_status == std::future_status::ready)
					{
						object->~T();
						return;
					}
				}
			}
			catch (std::exception& e)
			{
				T* object = reinterpret_cast<T*>(object_storage);

				// please note that this is unsafe if the exception was thrown during construction of T
				error<HasParenthesisOp<std::exception&>::value>(*object, e);
				object->~T();
			}
		}
	};
}

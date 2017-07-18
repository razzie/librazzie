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
#include <thread>
#include <vector>
#include "raz/memory.hpp"

namespace raz
{
	struct ThreadStop
	{
	};

	template<class T>
	class Thread
	{
	public:
		// please note that IMemoryPool must be thread-safe
		Thread(IMemoryPool* memory = nullptr) :
			m_memory(memory),
			m_exit_token(std::allocator_arg, raz::Allocator<int>(memory)),
			m_thread_result(std::allocator_arg, raz::Allocator<int>(memory)),
			m_call_queue(memory)
		{
		}

		Thread(const Thread&) = delete;

		Thread& operator=(const Thread&) = delete;

		~Thread()
		{
			if (m_thread.joinable())
			{
				m_exit_token.set_value();
				m_thread.join();
			}
		}

		template<class... Args>
		std::future<void> start(Args... args)
		{
			std::lock_guard<std::mutex> guard(m_mutex);

			if (m_thread.joinable())
			{
				m_exit_token.set_value();
				m_thread.join();
			}

			m_exit_token = std::move(std::promise<void>(std::allocator_arg, raz::Allocator<int>(m_memory)));
			m_thread_result = std::move(std::promise<void>(std::allocator_arg, raz::Allocator<int>(m_memory)));
			m_thread = std::thread(&Thread<T>::run<Args...>, this, std::forward<Args>(args)...);
			return m_thread_result.get_future();
		}

		void stop()
		{
			std::lock_guard<std::mutex> guard(m_mutex);

			if (m_thread.joinable())
			{
				m_exit_token.set_value();
				m_thread.join();
			}
		}

		void clear()
		{
			std::lock_guard<std::mutex> guard(m_mutex);
			m_call_queue.clear();
		}

		template<class... Args>
		void operator()(Args... args)
		{
			std::lock_guard<std::mutex> guard(m_mutex);
			m_call_queue.emplace_back(std::allocator_arg, raz::Allocator<char>(m_memory), [args...](T& object) { object(args...); });
		}

	private:
		typedef std::function<void(T&)> ForwardedCall;
		typedef std::vector<ForwardedCall, raz::Allocator<ForwardedCall>> ForwardedCallQueue;

		IMemoryPool* m_memory;
		std::thread m_thread;
		std::promise<void> m_exit_token;
		std::promise<void> m_thread_result;
		std::mutex m_mutex;
		ForwardedCallQueue m_call_queue;

		template<class... Args>
		class OpCaller
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

			static constexpr bool has_parenthesis_op = decltype(test<T>(true))::value;

			template<bool value>
			static std::enable_if_t<value> _call(T& object, Args... args)
			{
				object(std::forward<Args>(args)...);
			}

			template<int value>
			static std::enable_if_t<!value> _call(T& object, Args... args)
			{
			}

		public:
			static void call(T& object, Args... args)
			{
				_call<has_parenthesis_op>(object, std::forward<Args>(args)...);
			}
		};

		template<class... Args>
		void run(Args... args)
		{
			try
			{
				T object(std::forward<Args>(args)...);

				std::future<void> exit_token = m_exit_token.get_future();
				ForwardedCallQueue call_queue(m_memory);

				for (;;)
				{
					m_mutex.lock();
					std::swap(m_call_queue, call_queue);
					m_mutex.unlock();

					for (auto& call : call_queue)
					{
						try
						{
							call(object);
						}
						catch (ThreadStop)
						{
							m_thread_result.set_value();
							return;
						}
						catch (std::exception& e)
						{
							OpCaller<std::exception&>::call(object, e);
						}
						catch (...)
						{
							OpCaller<std::exception_ptr>::call(object, std::current_exception());
						}
					}

					call_queue.clear();

					try
					{
						OpCaller<>::call(object);
					}
					catch (ThreadStop)
					{
						m_thread_result.set_value();
						return;
					}
					catch (std::exception& e)
					{
						OpCaller<std::exception&>::call(object, e);
					}
					catch (...)
					{
						OpCaller<std::exception_ptr>::call(object, std::current_exception());
					}

					auto exit_status = exit_token.wait_for(std::chrono::milliseconds(1));
					if (exit_status == std::future_status::ready)
					{
						m_thread_result.set_value();
						return;
					}
				}
			}
			catch (...)
			{
				m_thread_result.set_exception(std::current_exception());
			}
		}
	};
}

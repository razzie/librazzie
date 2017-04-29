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
		Thread(Args... args) :
			m_object(std::forward<Args>(args)...)
		{
			m_thread = std::thread(&Thread<T>::run, this);
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
			m_call_queue.emplace_back([this, args...]() { m_object(args...); });
		}

	private:
		std::thread m_thread;
		std::promise<void> m_exit_token;
		std::mutex m_mutex;
		std::list<std::function<void()>> m_call_queue;
		T m_object;

		class HasParenthesisOp
		{
			template<class U>
			static auto test(bool) -> decltype(std::declval<U>()(), void(), std::true_type{})
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
		void loop();

		template<>
		void loop<true>()
		{
			m_object();
		}

		template<>
		void loop<false>()
		{
		}

		void run()
		{
			std::future<void> exit_token = m_exit_token.get_future();
			std::list<std::function<void()>> call_queue;

			for (;;)
			{
				m_mutex.lock();
				std::swap(m_call_queue, call_queue);
				m_mutex.unlock();

				for (auto& call : call_queue)
					call();

				call_queue.clear();

				loop<HasParenthesisOp::value>();

				auto exit_status = exit_token.wait_for(std::chrono::milliseconds(1));
				if (exit_status == std::future_status::ready) return;
			}
		}
	};
}

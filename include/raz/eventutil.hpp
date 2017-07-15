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

#include <deque>
#include <mutex>
#include "raz/callback.hpp"
#include "raz/event.hpp"

namespace raz
{
	template<class Event>
	class EventCallbackSystem : public raz::CallbackSystem<Event>
	{
	public:
		EventCallbackSystem() = default;

		explicit EventCallbackSystem(IMemoryPool& memory) :
			raz::CallbackSystem<Event>(memory)
		{
		}

		EventCallbackSystem(EventCallbackSystem&& other) :
			raz::CallbackSystem<Event>(other)
		{
		}

		static constexpr EventType getEventType()
		{
			return Event::getType();
		}

		template<class Serializer>
		raz::EnableSerializer<Serializer> handleSerialized(Serializer& serializer)
		{
			Event e(serializer);
			handle(e);
		}
	};


	template<class Event>
	class EventQueue
	{
	public:
		EventQueue(IMemoryPool* memory = nullptr) : m_queue(memory)
		{
		}

		EventQueue(EventQueue&& other) : m_queue(std::move(other.m_queue))
		{
		}

		static constexpr EventType getEventType()
		{
			return Event::getType();
		}

		template<class... Params>
		auto enqueue(Params... params) -> decltype(void(new Event(std::forward<Params>(params)...)))
		{
			std::lock_guard<decltype(m_lock)> guard(m_lock);
			m_queue.emplace_back(std::forward<Params>(params)...);
		}

		template<class Serializer>
		raz::EnableSerializer<Serializer> enqueueSerialized(Serializer& serializer)
		{
			std::lock_guard<decltype(m_lock)> guard(m_lock);
			m_queue.emplace_back(serializer);
		}

		template<class Handler>
		void dequeue(Handler& handler)
		{
			std::lock_guard<decltype(m_lock)> guard(m_lock);

			while (!m_queue.empty())
			{
				handler.handle(m_queue.front());
				m_queue.pop_front();
			}
		}

		template<class Handler>
		void dequeue(Handler& handler, unsigned n)
		{
			std::lock_guard<decltype(m_lock)> guard(m_lock);
			unsigned count = 0;

			while (!m_queue.empty() && count < n)
			{
				handler.handle(m_queue.front());
				m_queue.pop_front();
				++count;
			}
		}

		void clear()
		{
			m_queue.clear();
		}

	private:
		std::recursive_mutex m_lock;
		std::deque<Event, raz::Allocator<Event>> m_queue;
	};

	template<class... Events>
	class EventQueueSystem
	{
	public:
		explicit EventQueueSystem(IMemoryPool* memory = nullptr) :
			m_queues( (sizeof(Events), memory)... )
		{
		}

		EventQueueSystem(EventQueueSystem&& other) :
			m_queues(std::move(other.m_queues))
		{
		}

		template<EventType Type, class... Params>
		void enqueue(Params... params)
		{
			_enqueue<Type, 0>(std::forward<Params>(params)...);
		}

		template<class Serializer>
		raz::EnableSerializer<Serializer> enqueueSerialized(EventType type, Serializer& serializer)
		{
			_enqueueSerialized<0>(type, serializer);
		}

		template<class Handler>
		void dequeue(Handler& handler) // Event::CallbackSystem, EventDispatcher, etc
		{
			_dequeue<0>(handler);
		}

		void clear()
		{
			_clear<0>();
		}

	private:
		std::tuple<EventQueue<Events>...> m_queues;

		template<EventType Type>
		void _enqueue(...)
		{
		}

		template<EventType Type, class Queue, class... Params>
		auto _enqueue(Queue& queue, Params... params) -> decltype(queue.enqueue(std::forward<Params>(params)...))
		{
			if (Queue::getEventType() == Type)
				queue.enqueue(std::forward<Params>(params)...);
		}

		template<EventType Type, size_t N, class... Params>
		std::enable_if_t<(N < sizeof...(Events))> _enqueue(Params... params)
		{
			_enqueue<Type>(std::get<N>(m_queues), std::forward<Params>(params)...);
			_enqueue<Type, N + 1>(std::forward<Params>(params)...);
		}

		template<EventType Type, size_t N, class... Params>
		std::enable_if_t<(N >= sizeof...(Events))> _enqueue(Params...)
		{
		}

		template<size_t N, class Serializer>
		std::enable_if_t<(N < sizeof...(Events))> _enqueueSerialized(EventType type, Serializer& serializer)
		{
			auto& queue = std::get<N>(m_queues);
			if (queue.getEventType() == type)
				queue.enqueueSerialized(serializer);

			_enqueueSerialized<N + 1>(type, serializer);
		}

		template<size_t N, class Serializer>
		std::enable_if_t<(N >= sizeof...(Events))> _enqueueSerialized(EventType, Serializer&)
		{
		}

		void _dequeue(...)
		{
		}

		template<class Queue, class Handler>
		auto _dequeue(Queue& queue, Handler& handler) -> decltype(queue.dequeue(handler))
		{
			queue.dequeue(handler);
		}

		template<size_t N, class Handler>
		std::enable_if_t<(N < sizeof...(Events))> _dequeue(Handler& handler)
		{
			_dequeue(std::get<N>(m_queues), handler);
			_dequeue<N + 1>(handler);
		}

		template<size_t N, class Handler>
		std::enable_if_t<(N >= sizeof...(Events))> _dequeue(Handler&)
		{
		}

		template<size_t N>
		std::enable_if_t<(N < sizeof...(Events))> _clear()
		{
			std::get<N>(m_queues).clear();
			_clear<N + 1>();
		}

		template<size_t N>
		std::enable_if_t<(N >= sizeof...(Events))> _clear()
		{
		}
	};


	template<class... Events>
	class EventDispatcher
	{
	public:
		EventDispatcher(EventCallbackSystem<Events>&... callback_system) : // CallbackSystem references must not become invalid
			m_callback_systems((&callback_system)...)
		{
		}

		EventDispatcher(const EventDispatcher& other) :
			m_callback_systems(other.m_callback_systems)
		{
		}

		template<class Event>
		void handle(const Event& e)
		{
			_handle<0>(e);
		}

		template<class Serializer>
		raz::EnableSerializer<Serializer> handleSerialized(EventType type, Serializer& s)
		{
			_handleSerialized<0>(type, s);
		}

	private:
		std::tuple<EventCallbackSystem<Events>*...> m_callback_systems;

		void _handle(...)
		{
		}

		template<class CallbackSystem, class Event>
		auto _handle(CallbackSystem* handler, const Event& e) -> decltype(handler->handle(e))
		{
			handler->handle(e);
		}

		template<size_t N, class Event>
		std::enable_if_t<(N < sizeof...(Events))> _handle(const Event& e)
		{
			auto handler = std::get<N>(m_callback_systems);
			if (handler->getEventType() == e.getType())
				_handle(handler, e);
		}

		template<size_t N, class Event>
		std::enable_if_t<(N >= sizeof...(Events))> _handle(const Event&)
		{
		}

		template<size_t N, class Serializer>
		std::enable_if_t<(N < sizeof...(Events))> _handleSerialized(EventType type, Serializer& s)
		{
			auto handler = std::get<N>(m_callback_systems);
			if (handler->getEventType() == type)
				handler->handleSerialized<Serializer>(s);

			_handleSerialized<N + 1>(type, s);
		}

		template<size_t N, class Serializer>
		std::enable_if_t<(N >= sizeof...(Events))> _handleSerialized(EventType, Serializer&)
		{
		}
	};

	template<class... Events>
	class EventSystem : public EventDispatcher<Events...>
	{
	public:
		EventSystem() :
			EventDispatcher(std::get<EventCallbackSystem<Events>>(m_callback_systems)...)
		{
		}

		explicit EventSystem(IMemoryPool& memory) :
			EventDispatcher(std::get<EventCallbackSystem<Events>>(m_callback_systems)...)
			m_callback_systems( (sizeof(Events), memory)... )
		{
		}

		EventSystem(EventSystem&& other) :
			EventDispatcher(std::get<EventCallbackSystem<Events>>(m_callback_systems)...)
			m_callback_systems(std::move(other.m_callback_systems))
		{
		}

		template<class Event>
		EventCallbackSystem<Event>& getCallbackSystem(Event* = nullptr)
		{
			return std::get<EventCallbackSystem<Event>>(m_callback_systems);
		}

	private:
		std::tuple<EventCallbackSystem<Events>...> m_callback_systems;
	};
}

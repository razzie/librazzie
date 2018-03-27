/*
Copyright (C) Gábor "Razzie" Görzsöny

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

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <typeindex>
#include <vector>
#include "raz/thread.hpp"

#define RAZ_AUTOREMOVE_EXPIRED_EVENTRECEIVER

namespace raz
{
	class EventDispatcher
	{
	public:
		EventDispatcher(TaskManager* taskmgr, IMemoryPool* memory = nullptr) :
			m_taskmgr(taskmgr),
			m_memory(memory),
			m_handlers(memory)
		{

		}

		template<class Event>
		void operator()(const Event& evt)
		{
			std::unique_lock<std::mutex> lock(m_mutex);
			auto& handlers = getEventHandlerList(typeid(Event));

			if (m_taskmgr)
			{
				for (auto& handler : handlers)
					m_taskmgr->operator()(handler, &evt, &handler);
			}
			else
			{
				std::vector<std::shared_future<void>, raz::Allocator<std::shared_future<void>>> tasks(m_memory);

				for (auto& handler : handlers)
					tasks.push_back(TaskManager::pack(handler, &evt, &handler));

				lock.unlock();

				for (auto& task : tasks)
					task.wait();
			}
		}

		template<class Event0, class... Events, class EventReceiver>
		void bind(std::shared_ptr<EventReceiver> receiver)
		{
			bindEventReceiver<Event0>(receiver);
			bind<Events...>(receiver);
		}

		template<class EventReceiver>
		void bind(std::shared_ptr<EventReceiver> receiver)
		{
		}

	private:
		typedef std::function<void(const void*, const void*)> EventHandler;
		typedef std::vector<EventHandler, raz::Allocator<EventHandler>> EventHandlerList;

		std::mutex m_mutex;
		TaskManager* m_taskmgr;
		IMemoryPool* m_memory;
		std::map<std::type_index, EventHandlerList,
			     std::less<std::type_index>,
			     raz::Allocator<std::pair<const std::type_index, EventHandlerList>>> m_handlers;

		EventHandlerList& getEventHandlerList(std::type_index evt_type)
		{
			auto it = m_handlers.find(evt_type);
			if (it == m_handlers.end())
			{
				return m_handlers.emplace(evt_type, m_memory).first->second;
			}
			else
			{
				return it->second;
			}
		}

		template<class Event, class EventReceiver>
		void bindEventReceiver(std::shared_ptr<EventReceiver> receiver)
		{
			std::lock_guard<std::mutex> guard(m_mutex);
			auto handler = std::bind(&EventDispatcher::wrappedEventHandler<Event, EventReceiver>, this, receiver, std::placeholders::_1, std::placeholders::_2);
			getEventHandlerList(typeid(Event)).push_back(std::move(handler));
		}

		void removeEventHandler(std::type_index evt_type, const void* handler)
		{
			std::lock_guard<std::mutex> guard(m_mutex);
			auto& handlers = getEventHandlerList(evt_type);
			for (auto it = handlers.begin(), end = handlers.end(); it != end; ++it)
			{
				if (&*it == handler)
				{
					handlers.erase(it);
					return;
				}
			}
		}

		template<class Event, class EventReceiver>
		void wrappedEventHandler(std::weak_ptr<EventReceiver>&& ptr, const void* evt, const void* this_handler)
		{
			auto receiver = ptr.lock();
			if (receiver)
			{
				receiver->operator()( *reinterpret_cast<const Event*>(evt) );
			}
#ifdef RAZ_AUTOREMOVE_EXPIRED_EVENTRECEIVER
			else
			{
				removeEventHandler(typeid(Event), this_handler);
			}
#endif // RAZ_AUTOREMOVE_EXPIRED_EVENTRECEIVER
		}
	};

	template<class T>
	class EventReceiver : public std::enable_shared_from_this<T>
	{
	public:
		template<class... Events>
		void bind(EventDispatcher& dispatcher)
		{
			dispatcher.bind<Events...>(shared_from_this());
		}
	};
}

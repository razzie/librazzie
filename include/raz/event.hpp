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

		~EventDispatcher()
		{
			std::lock_guard<std::mutex> guard(m_mutex);
			for (auto& handler_list : m_handlers)
			{
				for (auto& handler : handler_list.second)
				{
					handler->onDispatcherDestroyed();
				}
			}
		}

		template<class Event>
		void operator()(Event event)
		{
			std::unique_lock<std::mutex> lock(m_mutex);
			auto& handlers = getEventHandlerList(typeid(Event));

			if (m_taskmgr)
			{
				for (auto& handler : handlers)
				{
					auto ptr = std::static_pointer_cast<EventHandler<Event>>(handler);
					m_taskmgr->operator()(ptr, event);
				}
			}
			else
			{
				std::vector<std::shared_future<void>, raz::Allocator<std::shared_future<void>>> tasks(m_memory);

				for (auto& handler : handlers)
				{
					auto ptr = std::static_pointer_cast<EventHandler<Event>>(handler);
					tasks.push_back(TaskManager::pack(ptr, event));
				}

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

		template<class Event0, class... Events, class EventReceiver>
		void unbind(EventReceiver* receiver)
		{
			unbindEventReceiver<Event0>(receiver);
			unbind<Events...>(receiver);
		}

		template<class EventReceiver>
		void unbind(EventReceiver* receiver)
		{
		}

	private:
		class IEventHandler
		{
		public:
			virtual ~IEventHandler() = default;
			virtual bool hasEventReceiver(void*) const = 0;
			virtual void onDispatcherDestroyed() = 0;
		};

		template<class Event>
		class EventHandler : public IEventHandler
		{
		public:
			virtual void operator()(const Event&) = 0;
		};

		template<class EventReceiver, class Event>
		class EventHandlerImpl : public EventHandler<Event>
		{
		public:
			EventHandlerImpl(EventDispatcher* dispatcher, std::shared_ptr<EventReceiver> receiver) :
				m_dispatcher(dispatcher),
				m_receiver(receiver),
				m_receiver_ptr_unsafe(receiver.get())
			{
			}

			virtual void operator()(const Event& event)
			{
				auto receiver = m_receiver.lock();
				if (receiver)
				{
					receiver->operator()(event);
				}
				else if (m_dispatcher)
				{
					m_dispatcher->removeEventHandler(typeid(Event), this);
				}
			}

			virtual bool hasEventReceiver(void* receiver) const
			{
				return (m_receiver_ptr_unsafe == receiver);
			}

			virtual void onDispatcherDestroyed()
			{
				m_dispatcher = nullptr;
			}

		private:
			EventDispatcher* m_dispatcher;
			std::weak_ptr<EventReceiver> m_receiver;
			EventReceiver* m_receiver_ptr_unsafe;
		};

		typedef std::shared_ptr<IEventHandler> EventHandlerPtr;
		typedef std::vector<EventHandlerPtr, raz::Allocator<EventHandlerPtr>> EventHandlerList;

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
			auto handler = std::make_shared<EventHandlerImpl<EventReceiver, Event>>(this, receiver);
			getEventHandlerList(typeid(Event)).push_back(handler);
		}

		template<class Event, class EventReceiver>
		void unbindEventReceiver(EventReceiver* receiver)
		{
			std::lock_guard<std::mutex> guard(m_mutex);
			auto& handlers = getEventHandlerList(typeid(Event));
			for (auto it = handlers.begin(), end = handlers.end(); it != end; ++it)
			{
				if ((*it)->hasEventReceiver(receiver))
				{
					handlers.erase(it);
					return;
				}
			}
		}

		void removeEventHandler(std::type_index evt_type, IEventHandler* handler)
		{
			std::lock_guard<std::mutex> guard(m_mutex);
			auto& handlers = getEventHandlerList(evt_type);
			for (auto it = handlers.begin(), end = handlers.end(); it != end; ++it)
			{
				if (it->get() == handler)
				{
					handlers.erase(it);
					return;
				}
			}
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

		template<class... Events>
		void unbind(EventDispatcher& dispatcher)
		{
			dispatcher.unbind<Events...>(this);
		}
	};
}

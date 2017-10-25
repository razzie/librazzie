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

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <vector>
#include "raz/hash.hpp"
#include "raz/memory.hpp"

namespace raz
{
	typedef uint32_t EventType;
	typedef uint32_t EventReceiverID;
	typedef uint32_t EventRouteID;

	template<EventType Type>
	struct Event
	{
		static constexpr EventType event_type = Type;
		EventReceiverID source = 0;
		EventReceiverID target = 0;
	};

	namespace e
	{
		struct IEventReceiver
		{
		};
	}

	template<EventReceiverID ReceiverID = __COUNTER__>
	struct EventReceiver : public e::IEventReceiver
	{
		static constexpr EventReceiverID receiver_id = ReceiverID; // must be unique
	};

	namespace e
	{
		template<class Event>
		using EventRouteCondition = bool(*)(const Event&, void*);

		template<class Event>
		class EventRoute
		{
		public:
			template<class EventReceiver>
			EventRoute(EventReceiver* receiver, EventRouteCondition<Event> condition = nullptr, EventRouteID id = 0) :
				m_receiver(receiver),
				m_handler(&__handler<EventReceiver>),
				m_condition(condition),
				m_id(id)
			{
			}

			EventRouteID getID() const
			{
				return m_id;
			}

			IEventReceiver* getEventReceiver()
			{
				return m_receiver;
			}

			void operator()(const Event& e, void* cookie) const
			{
				if (!m_condition || (m_condition && m_condition(e, cookie)))
				{
					m_handler(m_receiver, e);
				}
			}

		private:
			typedef void(*EventHandler)(IEventReceiver*, const Event&);

			template<class EventReceiver>
			static void __handler(IEventReceiver* receiver, const Event& e)
			{
				static_cast<EventReceiver*>(receiver)->operator()(e);
			}

			IEventReceiver* m_receiver;
			EventHandler m_handler;
			EventRouteCondition<Event> m_condition;
			EventRouteID m_id;
		};

		class IEventRouteTable
		{
		public:
			virtual ~IEventRouteTable() = default;
			virtual void remove(IEventReceiver* receiver) = 0;
		};

		template<class Event>
		class EventRouteTable : public IEventRouteTable
		{
		public:
			EventRouteTable(IMemoryPool* memory = nullptr) :
				m_routes(memory)
			{
			}

			template<class EventReceiver>
			void add(EventReceiver* receiver, EventRouteCondition<Event> condition = nullptr, EventRouteID id = 0)
			{
				m_routes.emplace_back(receiver, condition, id);
			}

			void remove(EventRouteID id)
			{
				for (auto it = m_routes.begin(); it != m_routes.end(); )
				{
					if (it->getID() == id)
						it = m_routes.erase(it);
					else
						++it;
				}
			}

			virtual void remove(IEventReceiver* receiver)
			{
				for (auto it = m_routes.begin(); it != m_routes.end(); )
				{
					if (it->getEventReceiver() == receiver)
						it = m_routes.erase(it);
					else
						++it;
				}
			}

			void handle(const Event& e, void* cookie) const
			{
				for (auto& route : m_routes)
				{
					route(e, cookie);
				}
			}

		private:
			std::vector<EventRoute<Event>, raz::Allocator<EventRoute<Event>>> m_routes;
		};
	}

	class EventDispatcher
	{
	public:
		EventDispatcher(IMemoryPool* memory = nullptr) :
			m_memory(memory)
		{
		}

		void unbindReceiver(e::IEventReceiver* receiver)
		{
			std::lock_guard<std::mutex> guard(m_mutex);

			for (auto& it : m_route_tables)
			{
				it.second->remove(receiver);
			}
		}

		template<class Event, class EventReceiver>
		void addEventRoute(EventReceiver* receiver, e::EventRouteCondition<Event> condition = nullptr, EventRouteID id = 0)
		{
			std::lock_guard<std::mutex> guard(m_mutex);
			getRouteTable<Event>()->add(receiver, condition, id);
		}

		template<class Event>
		void removeEventRoute(EventRouteID id)
		{
			std::lock_guard<std::mutex> guard(m_mutex);
			getRouteTable<Event>()->remove(id);
		}

		template<class Event>
		void dispatch(const Event& e, void* cookie = nullptr) const
		{
			std::lock_guard<std::mutex> guard(m_mutex);

			const auto* table = getRouteTable<Event>();
			if (table)
			{
				table->handle(e, cookie);
			}
		}

	private:
		class CustomRouteDeleter
		{
		public:
			CustomRouteDeleter(IMemoryPool* memory) :
				m_memory(memory)
			{
			}

			void operator()(e::IEventRouteTable* ptr)
			{
				if (m_memory)
					m_memory->destroy(ptr);
				else
					delete ptr;
			}

		private:
			IMemoryPool* m_memory;
		};

		mutable std::mutex m_mutex;
		IMemoryPool* m_memory;
		std::map<EventType, std::unique_ptr<e::IEventRouteTable, CustomRouteDeleter>> m_route_tables;

		template<class Event>
		e::EventRouteTable<Event>* getRouteTable()
		{
			auto it = m_route_tables.find(Event::event_type);
			if (it == m_route_tables.end())
			{
				decltype(m_route_tables)::mapped_type ptr(m_memory ? m_memory->create<e::EventRouteTable<Event>>() : new e::EventRouteTable<Event>(), m_memory);
				it = m_route_tables.emplace(Event::event_type, std::move(ptr)).first;
			}

			return static_cast<e::EventRouteTable<Event>*>(it->second.get());
		}

		template<class Event>
		const e::EventRouteTable<Event>* getRouteTable() const
		{
			auto it = m_route_tables.find(Event::event_type);
			if (it == m_route_tables.end())
				return nullptr;

			return static_cast<const e::EventRouteTable<Event>*>(it->second.get());
		}
	};

	namespace literal
	{
		inline constexpr EventType operator"" _event(const char* evt, size_t)
		{
			return (EventType)hash(evt);
		}

		inline constexpr EventType operator"" _recv(const char* evt, size_t)
		{
			return (EventType)hash(evt);
		}
	}
}

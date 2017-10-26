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
#include <typeindex>
#include <vector>
#include "raz/memory.hpp"

namespace raz
{
	typedef uint32_t EventRouteID;
	typedef void EventReceiver;
	typedef void Cookie;

	template<class EventT>
	using EventRouteCondition = bool(*)(const EventT&, Cookie*);

	class EventDispatcher
	{
	public:
		EventDispatcher(IMemoryPool* memory = nullptr) :
			m_memory(memory)
		{
		}

		void unbindReceiver(EventReceiver* receiver)
		{
			std::lock_guard<decltype(m_mutex)> guard(m_mutex);

			for (auto& it : m_route_tables)
			{
				it.second->remove(receiver);
			}
		}

		template<class EventT, class EventReceiverT>
		void addEventRoute(EventReceiverT* receiver, EventRouteCondition<EventT> condition = nullptr, EventRouteID id = 0)
		{
			std::lock_guard<decltype(m_mutex)> guard(m_mutex);
			getRouteTable<EventT>()->add(receiver, condition, id);
		}

		template<class EventT>
		void removeEventRoute(EventRouteID id)
		{
			std::lock_guard<decltype(m_mutex)> guard(m_mutex);
			getRouteTable<EventT>()->remove(id);
		}

		template<class EventT>
		void dispatch(const EventT& e, Cookie* cookie = nullptr) const
		{
			std::lock_guard<decltype(m_mutex)> guard(m_mutex);

			const auto* table = getRouteTable<EventT>();
			if (table)
			{
				table->handle(e, cookie);
			}
		}

	private:
		template<class EventT>
		class EventRoute
		{
		public:
			template<class EventReceiverT>
			EventRoute(EventReceiverT* receiver, EventRouteCondition<EventT> condition = nullptr, EventRouteID id = 0) :
				m_receiver(receiver),
				m_handler(&__handler<EventReceiverT>),
				m_condition(condition),
				m_id(id)
			{
			}

			EventRouteID getID() const
			{
				return m_id;
			}

			EventReceiver* getEventReceiver()
			{
				return m_receiver;
			}

			void operator()(const EventT& e, Cookie* cookie) const
			{
				if (!m_condition || (m_condition && m_condition(e, cookie)))
				{
					m_handler(m_receiver, e);
				}
			}

		private:
			typedef void(*EventHandler)(EventReceiver*, const EventT&);

			template<class EventReceiverT>
			static void __handler(EventReceiver* receiver, const EventT& e)
			{
				static_cast<EventReceiverT*>(receiver)->operator()(e);
			}

			EventReceiver* m_receiver;
			EventHandler m_handler;
			EventRouteCondition<EventT> m_condition;
			EventRouteID m_id;
		};

		class IEventRouteTable
		{
		public:
			virtual ~IEventRouteTable() = default;
			virtual void remove(EventReceiver* receiver) = 0;
		};

		template<class EventT>
		class EventRouteTable : public IEventRouteTable
		{
		public:
			EventRouteTable(IMemoryPool* memory = nullptr) :
				m_routes(memory)
			{
			}

			template<class EventReceiverT>
			void add(EventReceiverT* receiver, EventRouteCondition<EventT> condition = nullptr, EventRouteID id = 0)
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

			virtual void remove(EventReceiver* receiver)
			{
				for (auto it = m_routes.begin(); it != m_routes.end(); )
				{
					if (it->getEventReceiver() == receiver)
						it = m_routes.erase(it);
					else
						++it;
				}
			}

			void handle(const EventT& e, Cookie* cookie) const
			{
				for (auto& route : m_routes)
				{
					route(e, cookie);
				}
			}

		private:
			std::vector<EventRoute<EventT>, raz::Allocator<EventRoute<EventT>>> m_routes;
		};

		class CustomRouteDeleter
		{
		public:
			CustomRouteDeleter(IMemoryPool* memory) :
				m_memory(memory)
			{
			}

			void operator()(IEventRouteTable* ptr)
			{
				if (m_memory)
					m_memory->destroy(ptr);
				else
					delete ptr;
			}

		private:
			IMemoryPool* m_memory;
		};

		mutable std::recursive_mutex m_mutex;
		IMemoryPool* m_memory;
		std::map<std::type_index, std::unique_ptr<IEventRouteTable, CustomRouteDeleter>> m_route_tables;

		template<class EventT>
		EventRouteTable<EventT>* getRouteTable()
		{
			auto it = m_route_tables.find(typeid(EventT));
			if (it == m_route_tables.end())
			{
				decltype(m_route_tables)::mapped_type ptr(m_memory ? m_memory->create<EventRouteTable<EventT>>() : new EventRouteTable<EventT>(), m_memory);
				it = m_route_tables.emplace(typeid(EventT), std::move(ptr)).first;
			}

			return static_cast<EventRouteTable<EventT>*>(it->second.get());
		}

		template<class EventT>
		const EventRouteTable<EventT>* getRouteTable() const
		{
			auto it = m_route_tables.find(typeid(EventT));
			if (it == m_route_tables.end())
				return nullptr;

			return static_cast<const EventRouteTable<EventT>*>(it->second.get());
		}
	};
}

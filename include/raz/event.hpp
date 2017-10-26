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
	class EventDispatcher
	{
	public:
		template<class Event, class Cookie = void>
		using EventRouteCondition = bool(*)(const Event&, Cookie*);

		EventDispatcher(IMemoryPool* memory = nullptr) :
			m_memory(memory),
			m_recursion(0)
		{
		}

		template<class Event, class Cookie = void, class EventReceiver = void>
		void addEventRoute(EventReceiver* receiver, EventRouteCondition<Event, Cookie> condition = nullptr, int route_id = 0)
		{
			std::lock_guard<decltype(m_mutex)> guard(m_mutex);
			RecursionGuard rguard(m_recursion);

			getRouteTable<Event>()->add(receiver, condition, route_id);
		}

		template<class Event>
		void removeEventRoute(int route_id)
		{
			std::lock_guard<decltype(m_mutex)> guard(m_mutex);
			RecursionGuard rguard(m_recursion);

			getRouteTable<Event>()->remove(route_id);
		}

		template<class Event, class Cookie = void>
		void dispatch(const Event& e, Cookie* cookie = nullptr) const
		{
			std::lock_guard<decltype(m_mutex)> guard(m_mutex);
			RecursionGuard rguard(m_recursion);

			const auto* table = getRouteTable<Event>();
			if (table)
			{
				table->handle(e, cookie);
			}
		}

		void unbindReceiver(void* receiver)
		{
			std::lock_guard<decltype(m_mutex)> guard(m_mutex);
			RecursionGuard rguard(m_recursion);

			for (auto& it : m_route_tables)
			{
				it.second->remove(receiver);
			}
		}

		struct RecursionDetected : public std::exception
		{
		};

		struct CookieTypeMismatch : public std::exception
		{
		};

	private:
		template<class Event>
		class EventRoute
		{
		public:
			template<class EventReceiver, class Cookie>
			EventRoute(EventReceiver* receiver, EventRouteCondition<Event, Cookie> condition = nullptr, int route_id = 0) :
				m_route_id(route_id),
				m_receiver(receiver),
				m_handler(&__handler<EventReceiver>),
				m_condition(reinterpret_cast<EventRouteCondition<Event>>(condition)),
				m_cookie_type(typeid(Cookie))
			{
			}

			int getID() const
			{
				return m_route_id;
			}

			void* getvoid()
			{
				return m_receiver;
			}

			template<class Cookie>
			void operator()(const Event& e, Cookie* cookie) const
			{
				if (m_cookie_type != typeid(Cookie))
				{
					throw CookieTypeMismatch();
				}

				if (!m_condition || (m_condition && m_condition(e, cookie)))
				{
					m_handler(m_receiver, e);
				}
			}

		private:
			typedef void(*EventHandler)(void*, const Event&);

			template<class EventReceiver>
			static void __handler(void* receiver, const Event& e)
			{
				static_cast<EventReceiver*>(receiver)->operator()(e);
			}

			int m_route_id;
			void* m_receiver;
			EventHandler m_handler;
			EventRouteCondition<Event> m_condition;
			std::type_index m_cookie_type;
		};

		class IEventRouteTable
		{
		public:
			virtual ~IEventRouteTable() = default;
			virtual void remove(void* receiver) = 0;
		};

		template<class Event>
		class EventRouteTable : public IEventRouteTable
		{
		public:
			EventRouteTable(IMemoryPool* memory = nullptr) :
				m_routes(memory)
			{
			}

			template<class EventReceiver, class Cookie>
			void add(EventReceiver* receiver, EventRouteCondition<Event, Cookie> condition = nullptr, int route_id = 0)
			{
				m_routes.emplace_back(receiver, condition, route_id);
			}

			void remove(int route_id)
			{
				for (auto it = m_routes.begin(); it != m_routes.end(); )
				{
					if (it->getID() == route_id)
						it = m_routes.erase(it);
					else
						++it;
				}
			}

			virtual void remove(void* receiver)
			{
				for (auto it = m_routes.begin(); it != m_routes.end(); )
				{
					if (it->getvoid() == receiver)
						it = m_routes.erase(it);
					else
						++it;
				}
			}

			template<class Cookie>
			void handle(const Event& e, Cookie* cookie) const
			{
				for (auto& route : m_routes)
				{
					route(e, cookie);
				}
			}

		private:
			std::vector<EventRoute<Event>, raz::Allocator<EventRoute<Event>>> m_routes;
		};

		class EventRouteTableDeleter
		{
		public:
			EventRouteTableDeleter(IMemoryPool* memory) :
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

		class RecursionGuard
		{
		public:
			RecursionGuard(int& recursion) : m_recursion(recursion)
			{
				if (m_recursion > 0)
					throw RecursionDetected();

				++m_recursion;
			}

			~RecursionGuard()
			{
				--m_recursion;
			}

		private:
			int& m_recursion;
		};

		mutable std::recursive_mutex m_mutex;
		mutable int m_recursion;
		IMemoryPool* m_memory;
		std::map<std::type_index, std::unique_ptr<IEventRouteTable, EventRouteTableDeleter>> m_route_tables;

		template<class Event>
		EventRouteTable<Event>* getRouteTable()
		{
			auto it = m_route_tables.find(typeid(Event));
			if (it == m_route_tables.end())
			{
				decltype(m_route_tables)::mapped_type ptr(
					m_memory ? m_memory->create<EventRouteTable<Event>>(m_memory) : new EventRouteTable<Event>(),
					m_memory);
				it = m_route_tables.emplace(typeid(Event), std::move(ptr)).first;
			}

			return static_cast<EventRouteTable<Event>*>(it->second.get());
		}

		template<class Event>
		const EventRouteTable<Event>* getRouteTable() const
		{
			auto it = m_route_tables.find(typeid(Event));
			if (it == m_route_tables.end())
				return nullptr;

			return static_cast<const EventRouteTable<Event>*>(it->second.get());
		}
	};
}

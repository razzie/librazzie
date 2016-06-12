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
#include "raz/callback.hpp"
#include "raz/serialization.hpp"
#include "raz/storage.hpp"

namespace raz
{
	typedef uint32_t EventType;

	template<unsigned N, class T>
	struct EventParamTag
	{
		static const unsigned Param = N;
		typedef T Type;
	};

	template<EventType Type, class... Params>
	class Event : public Storage<Params...>
	{
	public:
		typedef Callback<Event> Callback;
		typedef CallbackSystem<Event> CallbackSystem;

		template<class _, class Serializer = EnableSerializer<_>>
		Event(Serializer& s)
		{
			if (s.getMode() == ISerializer::Mode::DESERIALIZE)
				(*this)(s);
		}

		Event(Params... params) : Storage(std::forward<Params>(params)...)
		{
		}

		EventType getType()
		{
			return Type;
		}

		template<class Tag>
		const typename Tag::Type& get(Tag tag = {})
		{
			return get<typename Tag::Type>(Tag::Param);
		}

		template<class Tag>
		void get(const Tag*& tag)
		{
			tag = &get<typename Tag::Type>(Tag::Param);
		}

		template<class _, class Serializer = EnableSerializer<_>>
		void operator()(Serializer& s)
		{
			serialize<0, Params...>(s, *this);
		}

		friend constexpr EventType operator"" _event(const char* evt, size_t)
		{
			return (EventType)stringhash(evt);
		}

	private:
		template<class Serializer, size_t N>
		void serialize(Serializer& s)
		{
		}

		template<class Serializer, size_t N, class Type0, class... Types>
		void serialize(Serializer& s)
		{
			s(get<Type0>(N));
			serialize<N + 1, Types...>(packet);
		}

		static constexpr uint64_t stringhash(const char* str, const uint64_t h = 5381)
		{
			return (str[0] == 0) ? h : stringhash(&str[1], h * 33 + str[0]);
		}
	};
}

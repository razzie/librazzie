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
#include <tuple>
#include <utility>
#include "raz/hash.hpp"
#include "raz/serialization.hpp"

namespace raz
{
	typedef uint32_t EventType;

	template<unsigned N>
	struct EventNamedParam
	{
		static const unsigned ParamNumber = N;
	};

	template<EventType Type, class... Params>
	class Event : public std::tuple<Params...>
	{
	public:
		template<class _, class Serializer = EnableSerializer<_>>
		explicit Event(Serializer& serializer)
		{
			if (serializer.getMode() == ISerializer::Mode::DESERIALIZE)
				_serialize<0>(serializer);
			else
				throw SerializationError();
		}

		Event(Params... params) :
			std::tuple<Params...>(std::forward<Params>(params)...)
		{
		}

		static constexpr EventType getType()
		{
			return Type;
		}

		template<size_t N>
		auto get() -> decltype(std::get<N>(*this))
		{
			return std::get<N>(*this);
		}

		template<size_t N>
		auto get() const -> decltype(std::get<N>(*this))
		{
			return std::get<N>(*this);
		}

		template<class NamedParam, size_t N = NamedParam::ParamNumber>
		auto get() -> decltype(std::get<N>(*this))
		{
			return std::get<N>(*this);
		}

		template<class NamedParam, size_t N = NamedParam::ParamNumber>
		auto get() const -> decltype(std::get<N>(*this))
		{
			return std::get<N>(*this);
		}

		template<class NamedParam, size_t N = NamedParam::ParamNumber>
		void get(NamedParam*& tag)
		{
			tag = &get<N>();
		}

		template<class NamedParam, size_t N = NamedParam::ParamNumber>
		void get(const NamedParam*& tag) const
		{
			tag = &get<N>();
		}

		template<class Serializer>
		raz::EnableSerializer<Serializer> operator()(Serializer& serializer)
		{
			_serialize<0>(serializer);
		}

	private:
		template<size_t N, class Serializer>
		std::enable_if_t<(N < sizeof...(Params))> _serialize(Serializer& serializer)
		{
			serializer(get<N>());
			_serialize<N + 1>(serializer);
		}

		template<size_t N, class Serializer>
		std::enable_if_t<(N >= sizeof...(Params))> _serialize(Serializer&)
		{
		}
	};

	namespace literal
	{
		inline constexpr EventType operator"" _event(const char* evt, size_t)
		{
			return (EventType)hash(evt);
		}
	}
}

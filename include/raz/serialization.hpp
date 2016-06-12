/*
Copyright (C) 2016 - G�bor "Razzie" G�rzs�ny

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
#include <string>
#include <type_traits>
#include "raz/ieee754.hpp"

namespace raz
{
	class SerializationError : public std::exception
	{
	public:
		SerializationError() : exception("Serialization error")
		{
		}
	};

	struct SerializerTag
	{
	};

	enum SerializationMode
	{
		SERIALIZE,
		DESERIALIZE
	};

	template<class BufferType, bool EndiannessConversion = false>
	class Serializer : public SerializerTag
	{
	public:
		template<class... Args>
		Serializer(Args... args) : m_buffer(std::forward<Args>(args)...)
		{
		}

		SerializationMode getMode() const
		{
			return m_buffer.getMode();
		}

		template<class I, class = std::enable_if_t<std::is_integral<I>::value>>
		Serializer& operator()(I& i)
		{
			if (getMode() == SerializationMode::SERIALIZE)
			{
				I tmp = i;

				if (EndiannessConversion && !isBigEndian())
					tmp = swapEndianness(tmp);

				if (m_buffer.write(reinterpret_cast<const char*>(&tmp), sizeof(I)) < sizeof(I))
					throw SerializationError();
			}
			else
			{
				I tmp;

				if (m_buffer.read(reinterpret_cast<char*>(&tmp), sizeof(I)) < sizeof(I))
					throw SerializationError();

				if (EndiannessConversion && !isBigEndian())
					tmp = swapEndianness(tmp);

				i = tmp;
			}

			return *this;
		}

		Serializer& operator()(float& f)
		{
			if (getMode() == SerializationMode::SERIALIZE)
			{
				uint32_t tmp = static_cast<uint32_t>(pack754_32(f));
				(*this)(tmp);
			}
			else
			{
				uint32_t tmp;
				(*this)(tmp);
				f = static_cast<float>(unpack754_32(tmp));
			}

			return *this;
		}

		Serializer& operator()(double& d)
		{
			if (getMode() == SerializationMode::SERIALIZE)
			{
				uint64_t tmp = pack754_32(d);
				(*this)(tmp);
			}
			else
			{
				uint64_t tmp;
				(*this)(tmp);
				d = unpack754_32(tmp);
			}

			return *this;
		}

		Serializer& operator()(std::string& str)
		{
			if (getMode() == SerializationMode::SERIALIZE)
			{
				uint32_t len = static_cast<uint32_t>(str.length());
				(*this)(len);

				if (write(str.c_str(), len) < len)
					throw SerializationError();
			}
			else
			{
				uint32_t len;
				(*this)(len);

				str.resize(len);
				if (read(&str[0], len) < len)
					throw SerializationError();
			}

			return *this;
		}

		template<class T>
		Serializer& operator()(T& t)
		{
			t(*this);
			return *this;
		}

	private:
		BufferType m_buffer;

		static constexpr bool isBigEndian()
		{
			union
			{
				uint32_t i;
				char c[4];
			} chk = { 0x01020304 };

			return (chk.c[0] == 1);
		}

		template<class T>
		static T swapEndianness(T t)
		{
			union
			{
				T t;
				unsigned char t8[sizeof(T)];
			} source, dest;

			source.t = t;

			for (size_t i = 0; i < sizeof(T); i++)
				dest.t8[i] = source.t8[sizeof(T) - i - 1];

			return dest.u;
		}
	};

	//template<class T>
	//class IsSerializer
	//{
	//	using value = std::is_base_of<SerializerTag, T>::value;
	//};

	template<class T>
	using EnableSerializer = std::enable_if_t<std::is_base_of<SerializerTag, T>::value, T>;
}

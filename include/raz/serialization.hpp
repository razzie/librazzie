/*
Copyright (C) G�bor "Razzie" G�rzs�ny

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

#include <array>
#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include <tuple>
#include <type_traits>
#include "raz/hash.hpp"

namespace raz
{
	class SerializationError : public std::exception
	{
	public:
		virtual const char* what() const
		{
			return "Serialization error";
		}
	};

	enum SerializationMode
	{
		SERIALIZE,
		DESERIALIZE
	};

	template<class BufferType, bool EndiannessConversion = false>
	class Serializer : public BufferType
	{
	public:
		template<class... Args>
		Serializer(Args&&... args) : BufferType(std::forward<Args>(args)...)
		{
		}

		template<class I>
		typename std::enable_if_t<std::is_integral<I>::value, Serializer>&
			operator()(I& i)
		{
			if (BufferType::getMode() == SerializationMode::SERIALIZE)
			{
				I tmp = i;

				if (sizeof(I) > 1 && EndiannessConversion && !isBigEndian())
					tmp = swapEndianness(tmp);

				if (BufferType::write(reinterpret_cast<const char*>(&tmp), sizeof(I)) < sizeof(I))
					throw SerializationError();
			}
			else
			{
				I tmp;

				if (BufferType::read(reinterpret_cast<char*>(&tmp), sizeof(I)) < sizeof(I))
					throw SerializationError();

				if (sizeof(I) > 1 && EndiannessConversion && !isBigEndian())
					tmp = swapEndianness(tmp);

				i = tmp;
			}

			return *this;
		}

		Serializer& operator()(float& f)
		{
			if (BufferType::getMode() == SerializationMode::SERIALIZE)
			{
				uint32_t tmp = static_cast<uint32_t>(pack754(f, 32, 8));
				(*this)(tmp);
			}
			else
			{
				uint32_t tmp;
				(*this)(tmp);
				f = static_cast<float>(unpack754(tmp, 32, 8));
			}

			return *this;
		}

		Serializer& operator()(double& d)
		{
			if (BufferType::getMode() == SerializationMode::SERIALIZE)
			{
				uint64_t tmp = pack754(d, 64, 11);
				(*this)(tmp);
			}
			else
			{
				uint64_t tmp;
				(*this)(tmp);
				d = static_cast<double>(unpack754(tmp, 64, 11));
			}

			return *this;
		}

		template<class T, size_t N>
		Serializer& operator()(std::array<T, N>& arr)
		{
			for (auto& t : arr)
				(*this)(t);
		}

		template<class CharType, class Allocator>
		Serializer& operator()(std::basic_string<CharType, std::char_traits<CharType>, Allocator>& str)
		{
			if (BufferType::getMode() == SerializationMode::SERIALIZE)
			{
				uint32_t len = static_cast<uint32_t>(str.length());
				(*this)(len);

				len *= sizeof(CharType);

				if (BufferType::write(reinterpret_cast<const char*>(str.c_str()), len) < len)
					throw SerializationError();
			}
			else
			{
				uint32_t len;
				(*this)(len);

				str.resize(len);

				len *= sizeof(CharType);

				if (BufferType::read(reinterpret_cast<char*>(&str[0]), len) < len)
					throw SerializationError();
			}

			return *this;
		}

		template<class T, class Allocator>
		Serializer& operator()(std::vector<T, Allocator>& vec)
		{
			if (BufferType::getMode() == SerializationMode::SERIALIZE)
			{
				uint32_t len = static_cast<uint32_t>(vec.size());
				(*this)(len);

				for (auto& t : vec)
					(*this)(t);
			}
			else
			{
				uint32_t len;
				(*this)(len);

				vec.resize(vec.size() + len);

				for (auto& t : vec)
					(*this)(t);
			}

			return *this;
		}

		template<class KT, class T, class Comp, class Allocator>
		Serializer& operator()(std::map<KT, T, Comp, Allocator>& map)
		{
			if (BufferType::getMode() == SerializationMode::SERIALIZE)
			{
				uint32_t len = static_cast<uint32_t>(map.size());
				(*this)(len);

				for (auto& t : map)
					(*this)(t.first)(t.second);
			}
			else
			{
				uint32_t len;
				(*this)(len);

				for (size_t i = 0; i < len; ++i)
				{
					KT key;
					(*this)(key)(map[key]);
				}
			}

			return *this;
		}

		template<class... T>
		Serializer& operator()(std::tuple<T...>& t)
		{
			serialize_tuple(t, std::make_index_sequence<sizeof...(T)>());
			return *this;
		}

		template<class T>
		typename std::enable_if_t<std::is_enum<T>::value, Serializer>&
			operator()(T& t)
		{
			return (*this)( *reinterpret_cast<std::underlying_type_t<T>*>(&t) );
		}

		template<class T>
		typename std::enable_if_t<!std::is_arithmetic<T>::value && !std::is_enum<T>::value, Serializer>&
			operator()(T& t)
		{
			t(*this);
			return *this;
		}

	private:
		template<class... T>
		void serialize_tuple(std::tuple<T...>&t, std::index_sequence<> i)
		{
		}

		template<class... T, size_t I0, size_t... I>
		void serialize_tuple(std::tuple<T...>& t, std::index_sequence<I0, I...> i)
		{
			(*this)(std::get<I0>(t));
			serialize_tuple(t, std::index_sequence<I...>());
		}

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

			return dest.t;
		}

#pragma warning(push)
#pragma warning(disable: 4244) // possible loss of data

		/*
		Original public domain code:
		http://beej.us/guide/bgnet/examples/ieee754.c
		*/

		static uint64_t pack754(long double f, unsigned bits, unsigned expbits)
		{
			long double fnorm;
			int shift;
			long long sign, exp, significand;
			unsigned significandbits = bits - expbits - 1; // -1 for sign bit

			if (f == 0.0) return 0; // get this special case out of the way

									// check sign and begin normalization
			if (f < 0) { sign = 1; fnorm = -f; }
			else { sign = 0; fnorm = f; }

			// get the normalized form of f and track the exponent
			shift = 0;
			while (fnorm >= 2.0) { fnorm /= 2.0; shift++; }
			while (fnorm < 1.0) { fnorm *= 2.0; shift--; }
			fnorm = fnorm - 1.0;

			// calculate the binary form (non-float) of the significand data
			significand = fnorm * ((1LL << significandbits) + 0.5f);

			// get the biased exponent
			exp = shift + ((1 << (expbits - 1)) - 1); // shift + bias

													  // return the final answer
			return (sign << (bits - 1)) | (exp << (bits - expbits - 1)) | significand;
		}

		static long double unpack754(uint64_t i, unsigned bits, unsigned expbits)
		{
			long double result;
			long long shift;
			unsigned bias;
			unsigned significandbits = bits - expbits - 1; // -1 for sign bit

			if (i == 0) return 0.0;

			// pull the significand
			result = (i&((1LL << significandbits) - 1)); // mask
			result /= (1LL << significandbits); // convert back to float
			result += 1.0f; // add the one back on

							// deal with the exponent
			bias = (1 << (expbits - 1)) - 1;
			shift = ((i >> significandbits)&((1LL << expbits) - 1)) - bias;
			while (shift > 0) { result *= 2.0; shift--; }
			while (shift < 0) { result /= 2.0; shift++; }

			// sign it
			result *= (i >> (bits - 1)) & 1 ? -1.0 : 1.0;

			return result;
		}

#pragma warning(pop)

	};

	template<class T>
	class IsSerializer
	{
		template<class U, bool E>
		static std::true_type test(Serializer<U, E> const &);

		static std::false_type test(...);

	public:
		static bool const value = decltype(test(std::declval<T>()))::value;
	};

	template<class Serializer, class T = void>
	using EnableSerializer = std::enable_if_t<IsSerializer<Serializer>::value, T>;

	template<class T, class... Ts>
	class DeserializerHelper
	{
	public:
		virtual void handle(T&&) = 0;
		virtual uint32_t getTypeHash() const { return raz::hash32<T>(); }
	};

	template<class T, class... Ts>
	class Deserializer : public DeserializerHelper<T>, public DeserializerHelper<Ts>...
	{
	public:
		template<class Serializer>
		bool operator()(uint32_t type_hash, Serializer& serializer)
		{
			serializer.setMode(raz::SerializationMode::DESERIALIZE);
			return tryDeserialize<Serializer, T, Ts...>(type_hash, serializer);
		}

	private:
		template<class Serializer, class U, class... Us>
		bool tryDeserialize(uint32_t type_hash, Serializer& serializer)
		{
			auto handler = static_cast<DeserializerHelper<U>*>(this);

			if (type_hash == handler->getTypeHash())
			{
				U obj;
				serializer(obj);
				handler->handle(std::move(obj));
				return true;
			}

			return tryDeserialize<Serializer, Us...>(type_hash, serializer);
		}

		template<class Serializer>
		bool tryDeserialize(uint32_t, Serializer&)
		{
			return false;
		}
	};
}

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
#include <cstring>
#include <type_traits>
#include "raz/event.hpp"
#include "raz/serialization.hpp"

namespace raz
{
	typedef uint32_t PacketType;

	template<SerializationMode MODE, size_t SIZE>
	class PacketBuffer
	{
	public:
		PacketBuffer() :
			m_data_len(0),
			m_data_pos(0)
		{
		}

		SerializationMode getMode() const
		{
			return MODE;
		}

		size_t getSize() const
		{
			m_data_len;
		}

		static size_t getCapacity()
		{
			return SIZE;
		}

		size_t write(const char* ptr, size_t len)
		{
			if (getMode() != Mode::SERIALIZE)
				throw SerializationError();

			if (BUF_SIZE - m_data_len < len)
				len = BUF_SIZE - m_data_len;

			std::memcpy(&m_data[m_data_len], ptr, len);
			m_data_len += len;
			return len;
		}

		size_t read(char* ptr, size_t len)
		{
			if (getMode() != Mode::DESERIALIZE)
				throw SerializationError();

			if (m_data_len - m_data_pos < len)
				len = m_data_len - m_data_pos;

			std::memcpy(ptr, &m_data[m_data_pos], len);
			m_data_pos += len;
			return len;
		}

		char* getData()
		{
			return m_data;
		}

		const char* getData() const
		{
			return m_data;
		}

	private:
		char m_data[SIZE];
		size_t m_data_len;
		size_t m_data_pos;
	};

	typedef uint32_t PacketType;

	template<SerializationMode MODE, size_t SIZE = 4096>
	class Packet : public Serializer<PacketBuffer<MODE, SIZE>>
	{
	public:
		struct Head
		{
			PacketType type;
			uint16_t size;
		};

		struct Tail
		{
			uint32_t n = 0;
			bool ok() { return n == 0; }
		};

		Packet(PacketType type) :
			m_type(type)
		{
		}

		PacketType getType() const
		{
			return m_type;
		}

	private:
		PacketType m_type;
	};

	template<size_t SIZE>
	using ReadablePacket = Packet<SerializationMode::DESERIALIZE, SIZE>;
	template<size_t SIZE>
	using WritablePacket = Packet<SerializationMode::SERIALIZE, SIZE>;

	template<class CLIENT, size_t SIZE>
	struct NetworkReadData
	{
		CLIENT client;
		ReadablePacket<SIZE> packet;
	};

	template<class CLIENT, size_t SIZE>
	struct NetworkWriteData
	{
		CLIENT client;
		WritablePacket<SIZE> packet;
	};
}

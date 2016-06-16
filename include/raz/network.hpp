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

	template<size_t SIZE>
	class PacketBuffer
	{
	public:
		PacketBuffer(SerializationMode mode) :
			m_mode(mode),
			m_data_len(0),
			m_data_pos(0)
		{
		}

		SerializationMode getMode() const
		{
			return m_mode;
		}

		void setMode(SerializationMode mode)
		{
			m_mode = mode;
		}

		char* getData()
		{
			return m_data;
		}

		const char* getData() const
		{
			return m_data;
		}

		char* getRemainingData()
		{
			return &m_data[m_data_pos];
		}

		const char* getRemainingData() const
		{
			return &m_data[m_data_pos];
		}

		size_t getRemainingDataLength() const
		{
			m_data_len - m_data_pos;
		}

		size_t getDataLength() const
		{
			m_data_len;
		}

		void setDataLength(size_t len)
		{
			m_data_len = len;
			if (m_data_pos > m_data_len)
				m_data_pos = m_data_len;
		}

		static size_t getCapacity()
		{
			return SIZE;
		}

		size_t write(const char* ptr, size_t len)
		{
			if (SIZE - m_data_len < len)
				len = SIZE - m_data_len;

			std::memcpy(&m_data[m_data_len], ptr, len);
			m_data_len += len;
			return len;
		}

		size_t read(char* ptr, size_t len)
		{
			if (m_data_len - m_data_pos < len)
				len = m_data_len - m_data_pos;

			std::memcpy(ptr, &m_data[m_data_pos], len);
			m_data_pos += len;
			return len;
		}

	private:
		char m_data[SIZE];
		SerializationMode m_mode;
		size_t m_data_len;
		size_t m_data_pos;
	};

	typedef uint32_t PacketType;

	template<size_t SIZE = 4096>
	class Packet : public Serializer<PacketBuffer<SIZE>>
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

		Packet() :
			Serializer<PacketBuffer<SIZE>>(SerializationMode::DESERIALIZE),
			m_type(0)
		{
		}

		Packet(PacketType type, SerializationMode mode) :
			Serializer<PacketBuffer<SIZE>>(mode),
			m_type(type)
		{
		}

		PacketType getType() const
		{
			return m_type;
		}

		void setType(PacketType type)
		{
			m_type = type;
		}

	private:
		PacketType m_type;
	};

	template<class CLIENT, size_t SIZE>
	struct NetworkData
	{
		CLIENT client;
		Packet<SIZE> packet;
	};
}

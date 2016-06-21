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
		struct Head
		{
			PacketType packet_type;
			uint16_t packet_size;
		};

		struct Tail
		{
			uint32_t n = 0;
			bool ok() { return n == 0; }
		};

		struct PacketData
		{
			Head head;
			char data[SIZE];
			Tail tail;
		};

		PacketBuffer(SerializationMode mode = SerializationMode::DESERIALIZE, PacketType type = 0) :
			m_mode(mode),
			m_data_pos(0)
		{
			m_data.packet_type = type;
			m_data.packet_size = 0;
		}

		SerializationMode getMode() const
		{
			return m_mode;
		}

		void setMode(SerializationMode mode)
		{
			m_mode = mode;
		}

		PacketType getType() const
		{
			return m_data.type;
		}

		void setType(PacketType type)
		{
			m_data.type = type;
		}

		PacketData* getPacketData()
		{
			return &m_data;
		}

		const PacketData& getPacketData() const
		{
			return &m_data;
		}

		static size_t getDataCapacity()
		{
			return SIZE;
		}

		size_t write(const char* ptr, size_t len)
		{
			if (SIZE - m_data.packet_size < len)
				len = SIZE - m_data.packet_size;

			std::memcpy(&m_data.data[m_data.packet_size], ptr, len);
			m_data.packet_size += len;
			return len;
		}

		size_t read(char* ptr, size_t len)
		{
			if (m_data.packet_size - m_data_pos < len)
				len = m_data.packet_size - m_data_pos;

			std::memcpy(ptr, &m_data.data[m_data_pos], len);
			m_data_pos += len;
			return len;
		}

	private:
		PacketData m_data;
		SerializationMode m_mode;
		size_t m_data_pos;
	};

	template<size_t SIZE = 4096>
	using Packet = typename Serializer<PacketBuffer<SIZE>>;

	class NetworkException : public std::exception
	{
	public:
		NetworkException(const char* what) : m_what(what)
		{
		}

		virtual const char* what() const
		{
			return m_what;
		}

	private:
		const char* m_what;
	};

	template<class ClientBackend>
	class NetworkClient
	{
	public:
		template<class... Args>
		NetworkClient(Args... args) : m_backend(std::forward<Args>(args)...)
		{
		}

		template<class Packet>
		bool receive(Packet& packet, uint32_t timeous_ms = 0)
		{
			decltype(packet)::PacketData* pdata = packet.getPacketData();
			size_t netbuffer_len;

			netbuffer_len = m_backend.wait(timeous_ms); // waits until data is available and returns its size

			// check if at least the packet head could be read
			if (netbuffer_len < sizeof(pdata->head))
				return false;

			m_backend.peek(reinterpret_cast<char*>(&pdata->head), sizeof(pdata->head));

			// check if the whole packet data is available
			if ((netbuffer_len - sizeof(pdata->head) - sizeof(pdata->tail) < pdata->head.packet_size))
				return false;

			if (pdata->head.packet_size > packet.getDataCapacity())
				throw NetworkException("Insufficient packet capacity");

			// reading actual data to the packet
			m_backend.read(reinterpret_cast<char*>(&pdata->head), sizeof(pdata->head));
			m_backend.read(pdata->data, pdata->head.packet_size);
			m_backend.read(reinterpret_cast<char*>(&pdata->tail), sizeof(pdata->tail));

			if (!pdata->tail.ok())
				throw NetworkException("Corrupted packet");

			return true;
		}

		template<class Packet>
		void send(Packet& packet)
		{
			const decltype(packet)::PacketData* data = packet.getPacketData();

			// move tailing bytes to the proper position if necessary
			if (reinterpret_cast<const char*>(&pdata->tail) != &pdata->data[pdata->packet_size])
				std::memcpy(&pdata->data[pdata->packet_size], &pdata->tail, sizeof(pdata->tail));

			m_backend.write(reinterpret_cast<const char*>(data), sizeof(pdata->head) + pdata->packet_size + sizeof(pdata->tail));
		}

		ClientBackend& getBackend()
		{
			return m_backend;
		}

		const ClientBackend& getBackend() const
		{
			return m_backend;
		}

	private:
		ClientBackend m_backend;
	};

	template<class ServerBackend>
	class NetworkServer
	{
	public:
		typedef typename ServerBackend::Client Client;
		typedef typename ServerBackend::ClientState ClientState;

		template<size_t SIZE>
		struct ClientData
		{
			//enum State
			//{
			//	INITIAL,
			//	CONNECTED,
			//	PACKET_RECEIVED,
			//	DISCONNECTED,
			//	ERROR
			//};

			Client client;
			Packet<SIZE> packet;
			ClientState state;
		};

		template<class... Args>
		NetworkServer(Args... args) : m_backend(std::forward<Args>(args)...)
		{
		}

		template<class ClientData>
		bool receive(ClientData& data)
		{
			decltype(data.packet)::PacketData* pdata = data.packet.getPacketData();
			size_t netbuffer_len;

			netbuffer_len = m_backend.wait(data.client, data.state, timeous_ms); // waits until data is available and returns its size

			// check if at least the packet head could be read
			if (netbuffer_len < sizeof(pdata->head))
				return false;

			m_backend.peek(data.client, reinterpret_cast<char*>(&pdata->head), sizeof(pdata->head));

			// check if the whole packet data is available
			if ((netbuffer_len - sizeof(pdata->head) - sizeof(pdata->tail) < pdata->head.packet_size))
				return false;

			if (pdata->head.packet_size > data.packet.getDataCapacity())
				throw NetworkException("Insufficient packet capacity");

			// reading actual data to the packet
			m_backend.read(data.client, reinterpret_cast<char*>(&pdata->head), sizeof(pdata->head));
			m_backend.read(data.client, pdata->data, pdata->head.packet_size);
			m_backend.read(data.client, reinterpret_cast<char*>(&pdata->tail), sizeof(pdata->tail));

			if (!pdata->tail.ok())
				throw NetworkException("Corrupted packet");

			return true;
		}

		template<class Packet>
		void send(Client& client, Packet& packet)
		{
			const decltype(packet)::PacketData* data = packet.getPacketData();

			// move tailing bytes to the proper position if necessary
			if (reinterpret_cast<const char*>(&pdata->tail) != &pdata->data[pdata->packet_size])
				std::memcpy(&pdata->data[pdata->packet_size], &pdata->tail, sizeof(pdata->tail));

			m_backend.write(client, reinterpret_cast<const char*>(data), sizeof(pdata->head) + pdata->packet_size + sizeof(pdata->tail));
		}

		ServerBackend& getBackend()
		{
			return m_backend;
		}

		const ServerBackend& getBackend() const
		{
			return m_backend;
		}

	private:
		ServerBackend m_backend;
	};
}

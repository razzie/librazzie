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

#pragma warning (disable : 4250)
#pragma comment(lib, "ws2_32.lib")
#include <winsock2.h>
#include <ws2tcpip.h>

#include <cstdint>
#include <cstring>
#include <exception>
#include <string>
#include <vector>

namespace raz
{
	class NetworkConnectionError : public std::exception
	{
	public:
		virtual const char* what() const
		{
			return "Connection error";
		}
	};

	class NetworkSocketError : public std::exception
	{
	public:
		virtual const char* what() const
		{
			return "Socket error";
		}
	};

	class ClientBackendTCP
	{
	public:
		ClientBackendTCP(const char* host, uint16_t port, bool ipv6 = false)
		{
			std::string port_str = std::to_string(port);
			struct addrinfo hints, *result = NULL, *ptr = NULL;

			std::memset(&m_sockaddr, 0, sizeof(SOCKADDR_STORAGE));

			std::memset(&hints, 0, sizeof(struct addrinfo));
			hints.ai_family = ipv6 ? AF_INET6 : AF_INET;
			hints.ai_socktype = SOCK_STREAM;
			hints.ai_protocol = IPPROTO_TCP;
			hints.ai_flags = AI_PASSIVE;

			// resolve the local address and port to be used by the server
			if (getaddrinfo(host, port_str.c_str(), &hints, &result) != 0)
			{
				throw NetworkConnectionError();
			}

			m_socket = INVALID_SOCKET;

			// attempt to connect to the first possible address in the list returned by getaddrinfo
			for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
			{
				m_socket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
				if (m_socket == INVALID_SOCKET)
				{
					continue;
				}

				if (connect(m_socket, ptr->ai_addr, (int)ptr->ai_addrlen) == SOCKET_ERROR)
				{
					closesocket(m_socket);
					m_socket = INVALID_SOCKET;
					continue;
				}

				// everything is OK if we get here
				break;
			}

			freeaddrinfo(result);

			if (m_socket == INVALID_SOCKET)
				throw NetworkConnectionError();
		}

		size_t wait(uint32_t timeous_ms)
		{
			fd_set set;
			FD_ZERO(&set);
			FD_SET(m_socket, &set);

			struct timeval timeout;
			timeout.tv_sec = timeous_ms / 1000;
			timeout.tv_usec = (timeous_ms % 1000) * 1000;

			int rc = select(m_socket + 1, &set, NULL, NULL, &timeout);
			if (rc == SOCKET_ERROR)
			{
				throw NetworkSocketError();
			}
			else if (rc > 0)
			{
				return rc;
			}
			else
			{
				return 0;
			}
		}

		size_t peek(char* ptr, size_t len)
		{
			int rc = recv(m_socket, ptr, len, MSG_PEEK);
			if (rc == SOCKET_ERROR)
				throw NetworkSocketError();
			else
				return rc;
		}

		size_t read(char* ptr, size_t len)
		{
			int rc = recv(m_socket, ptr, len, 0);
			if (rc == SOCKET_ERROR)
				throw NetworkSocketError();
			else
				return rc;
		}

		size_t write(const char* ptr, size_t len)
		{
			int rc = send(m_socket, ptr, len, 0);
			if (rc == SOCKET_ERROR)
				throw NetworkSocketError();
			else
				return rc;
		}

		void disconnect()
		{
			closesocket(m_socket);
			m_socket = INVALID_SOCKET;
		}

	private:
		SOCKET m_socket;
		SOCKADDR_STORAGE m_sockaddr;
	};

	class ServerBackendTCP
	{
	public:
		struct Client
		{
			SOCKET socket;
			SOCKADDR_STORAGE sockaddr;
		};

		enum ClientState
		{
			UNSET,
			CONNECTED,
			PACKET_RECEIVED,
			DISCONNECTED
		};

		ServerBackendTCP(uint16_t port, bool ipv6 = false)
		{
			std::string port_str = std::to_string(port);
			struct addrinfo hints, *result = NULL, *ptr = NULL;

			std::memset(&m_sockaddr, 0, sizeof(SOCKADDR_STORAGE));

			std::memset(&hints, 0, sizeof(struct addrinfo));
			hints.ai_family = ipv6 ? AF_INET6 : AF_INET;
			hints.ai_socktype = SOCK_STREAM;
			hints.ai_protocol = IPPROTO_TCP;
			hints.ai_flags = AI_PASSIVE;

			// resolve the local address and port to be used by the server
			if (getaddrinfo(NULL, port_str.c_str(), &hints, &result) != 0)
			{
				throw NetworkSocketError();
			}

			m_socket = INVALID_SOCKET;

			// attempt to connect to the first possible address in the list returned by getaddrinfo
			for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
			{
				m_socket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
				if (m_socket == INVALID_SOCKET)
				{
					continue;
				}

				int no = 0;
				setsockopt(m_socket, IPPROTO_IPV6, IPV6_V6ONLY, (const char*)&no, sizeof(no));
				setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)ptr->ai_addr, (int)ptr->ai_addrlen);

				if (bind(m_socket, ptr->ai_addr, (int)ptr->ai_addrlen) == SOCKET_ERROR)
				{
					closesocket(m_socket);
					m_socket = INVALID_SOCKET;
					continue;
				}

				if (listen(m_socket, SOMAXCONN) == SOCKET_ERROR)
				{
					closesocket(m_socket);
					m_socket = INVALID_SOCKET;
					continue;
				}

				// everything is OK if we get here
				break;
			}

			freeaddrinfo(result);

			if (m_socket == INVALID_SOCKET)
			{
				throw NetworkSocketError();
			}
		}

		size_t wait(Client& client, ClientState& state, uint32_t timeous_ms)
		{
			fd_set set;
			FD_ZERO(&set);
			FD_SET(m_socket, &set);
			for (Client& c : m_clients)
				FD_SET(c.socket, &set);

			struct timeval timeout;
			timeout.tv_sec = timeous_ms / 1000;
			timeout.tv_usec = (timeous_ms % 1000) * 1000;

			int rc = select(m_socket + 1, &set, NULL, NULL, &timeout);
			if (rc == SOCKET_ERROR)
			{
				throw NetworkSocketError();
			}
			else if (rc > 0)
			{
				if (FD_ISSET(m_socket, &set))
				{
					int addrlen = sizeof(client.sockaddr);
					client.socket = accept(m_socket, reinterpret_cast<struct sockaddr*>(&client.sockaddr), &addrlen);
					if (client.socket == INVALID_SOCKET)
					{
						throw NetworkSocketError();
					}

					state = ClientState::CONNECTED;
					return 0;
				}

				for (size_t i = 0; i < m_clients.size(); ++i)
				{
					size_t index = (i + m_client_to_check) % m_clients.size();
					SOCKET sock = m_clients[index].socket;
					if (FD_ISSET(sock, &set))
					{
						client = m_clients[index];
						state = ClientState::PACKET_RECEIVED;

						u_long bytes_available = 0;
						ioctlsocket(sock, FIONREAD, &bytes_available);
						return static_cast<size_t>(bytes_available);
					}
				}
			}

			state = ClientState::UNSET;
			return 0;
		}

		size_t peek(Client& client, char* ptr, size_t len)
		{
			int rc = recv(client.socket, ptr, len, MSG_PEEK);
			if (rc == SOCKET_ERROR)
				throw NetworkSocketError();
			else
				return rc;
		}

		size_t read(Client& client, char* ptr, size_t len)
		{
			int rc = recv(client.socket, ptr, len, 0);
			if (rc == SOCKET_ERROR)
				throw NetworkSocketError();
			else
				return rc;
		}

		size_t write(Client& client, const char* ptr, size_t len)
		{
			int rc = send(client.socket, ptr, len, 0);
			if (rc == SOCKET_ERROR)
				throw NetworkSocketError();
			else
				return rc;
		}

		void disconnect()
		{
			for (Client& client : m_clients)
				closesocket(client.socket);
			m_clients.clear();

			closesocket(m_socket);
			m_socket = INVALID_SOCKET;
		}

	private:
		SOCKET m_socket;
		SOCKADDR_STORAGE m_sockaddr;
		std::vector<Client> m_clients;
		size_t m_client_to_check = 0;
	};
}

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

#include <chrono>
#include <future>
#include <iostream>
#include <thread>
#include "raz/network.hpp"
#include "raz/networkbackend.hpp"

struct Foo
{
	int value;

	template<class Serializer>
	raz::EnableSerializer<Serializer> operator()(Serializer& serializer)
	{
		serializer(value);
	}
};

template<class Server>
void runServer(uint16_t port, std::future<void> exit_token)
{
	try
	{
		Server server(port);
		Server::ClientData<512> data;
		Foo foo;

		std::cout << "Server started (port: " << port << ")" << std::endl;

		for (;;)
		{
			data.packet.reset();
			while (!server.receive(data, 1000)) // wait until packet is received
			{
				auto exit_status = exit_token.wait_for(std::chrono::milliseconds(1));
				if (exit_status == std::future_status::ready) return;
			}

			data.packet.setMode(raz::SerializationMode::DESERIALIZE);
			data.packet(foo); // deserialize foo
			std::cout << "Server: foo received (value: " << foo.value << ")" << std::endl;

			foo.value += foo.value; // change foo value before sending it back

			data.packet.reset();
			data.packet.setMode(raz::SerializationMode::SERIALIZE);
			data.packet(foo); // serialize foo

			server.send(data.client, data.packet); // send foo to client
			std::cout << "Server: foo sent to client (value: " << foo.value << ")" << std::endl;
		}
	}
	catch (raz::NetworkConnectionError&)
	{
		std::cout << "Failed to start server" << std::endl;
	}
	catch (std::exception& e)
	{
		std::cout << "Server exception: " << e.what() << std::endl;
	}
}

template<class Client>
void runClient(uint16_t port)
{
	try
	{
		Client client("localhost", port);
		raz::Packet<512> packet;
		Foo foo;

		std::cout << "Client connected to server (localhost:" << port << ")" << std::endl;

		foo.value = 99;
		packet.setMode(raz::SerializationMode::SERIALIZE);
		packet(foo); // serialize foo

		client.send(packet); // send foo to server
		std::cout << "Client: foo sent to server (value: " << foo.value << ")" << std::endl;

		packet.reset();
		while (!client.receive(packet, 1000)); // wait until packet is received

		packet.setMode(raz::SerializationMode::DESERIALIZE);
		packet(foo); // deserialize foo
		std::cout << "Client: foo received (value: " << foo.value << ")" << std::endl;
	}
	catch (raz::NetworkConnectionError&)
	{
		std::cout << "Failed to connect to server" << std::endl;
	}
	catch (std::exception& e)
	{
		std::cout << "Client exception: " << e.what() << std::endl;
	}
}

raz::NetworkInitializer __init_network;

int main()
{
	std::promise<void> server_exit_token;
	int port = 12345;
	int protocol;

	std::cout << "Choose protocol\n 1 - TCP\n 2 - UDP" << std::endl;
	std::cin >> protocol;
	std::cout << std::endl;

	if (protocol == 1)
	{
		std::thread t(runServer<raz::NetworkServerTCP>, port, server_exit_token.get_future());
		runClient<raz::NetworkClientTCP>(port);
		server_exit_token.set_value();
		t.join();
	}
	else if (protocol == 2)
	{
		std::thread t(runServer<raz::NetworkServerUDP>, port, server_exit_token.get_future());
		runClient<raz::NetworkClientUDP>(port);
		server_exit_token.set_value();
		t.join();
	}
	else
	{
		std::cout << "Wrong selection" << std::endl;
	}

	return 0;
}

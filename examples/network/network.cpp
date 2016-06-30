#include <chrono>
#include <iostream>
#include <thread>
#include "raz/network.hpp"
#include "raz/networkbackend.hpp"

typedef raz::NetworkClient<raz::NetworkClientBackendTCP> ClientTCP;
typedef raz::NetworkServer<raz::NetworkServerBackendTCP> ServerTCP;

typedef raz::NetworkClient<raz::NetworkClientBackendUDP>   ClientUDP;
typedef raz::NetworkServer<raz::NetworkServerBackendUDP<>> ServerUDP;

struct Foo
{
	int i;

	template<class Serializer>
	raz::EnableSerializer<Serializer> operator()(Serializer& s)
	{
		s(i);
	}
};

template<class Server>
void runServer(uint16_t port)
{
	Server server;
	Server::ClientData<512> data;
	Foo foo;

	if (!server.getBackend().open(port))
	{
		std::cout << "Failed to start server" << std::endl;
		return;
	}

	try
	{
		data.packet.setMode(raz::SerializationMode::DESERIALIZE);

		for (int i = 0; i < 5; ++i)
		{
			data.packet.reset();

			while (!server.receive(data, 1000));

			data.packet(foo);
			std::cout << "Packet received (Foo:" << foo.i << ")" << std::endl;
		}
	}
	catch (std::exception& e)
	{
		std::cout << "Server exception: " << e.what() << std::endl;
	}
}

template<class Client>
void runClient(uint16_t port)
{
	Client client;
	raz::Packet<512> packet;
	Foo foo;

	if (!client.getBackend().open("localhost", port))
	{
		std::cout << "Failed to connect to server" << std::endl;
		return;
	}

	try
	{
		packet.setMode(raz::SerializationMode::SERIALIZE);

		for (int i = 0; i < 5; ++i)
		{
			foo.i = i;

			packet.reset();
			packet(foo);

			client.send(packet);

			std::this_thread::sleep_for(std::chrono::milliseconds(500));
		}
	}
	catch (std::exception& e)
	{
		std::cout << "Client exception: " << e.what() << std::endl;
	}
}

raz::NetworkInitializer __init_network;

int main()
{
	int port = 12345;
	int protocol;

	std::cout << "Choose protocol\n 1 - TCP\n 2 - UDP" << std::endl;
	std::cin >> protocol;
	std::cout << std::endl;

	if (protocol == 1)
	{
		std::thread t(runServer<ServerTCP>, port);

		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		runClient<ClientTCP>(port);

		t.join();
	}
	else if (protocol == 2)
	{
		std::thread t(runServer<ServerUDP>, port);

		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		runClient<ClientUDP>(port);

		t.join();
	}
	else
	{
		std::cout << "Wrong selection" << std::endl;
	}

	return 0;
}

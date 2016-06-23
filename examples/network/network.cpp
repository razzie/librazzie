#include <chrono>
#include <iostream>
#include <thread>
#include "raz/network.hpp"
#include "raz/networkbackend.hpp"

typedef raz::NetworkClient<raz::ClientBackendTCP> Client;
typedef raz::NetworkServer<raz::ServerBackendTCP> Server;

struct Foo
{
	int i;

	template<class Serializer>
	raz::EnableSerializer<Serializer> operator()(Serializer& s)
	{
		s(i);
	}
};

void serverThreadFunc()
{
	Server server;
	Server::ClientData<512> data;
	Foo foo;

	if (!server.getBackend().open(12345))
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
			std::cout << "packet received (Foo:" << foo.i << ")" << std::endl;
		}
	}
	catch (std::exception& e)
	{
		std::cout << "server exception: " << e.what() << std::endl;
	}
}

raz::NetworkInitializer __init_network;

int main()
{
	std::thread t(serverThreadFunc);
	Client client;
	raz::Packet<512> packet;
	Foo foo;

	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	if (client.getBackend().open("localhost", 12345))
	{
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
			std::cout << "client exception: " << e.what() << std::endl;
		}
	}
	else
	{
		std::cout << "Failed to connect to server" << std::endl;
	}

	t.join();

	return 0;
}

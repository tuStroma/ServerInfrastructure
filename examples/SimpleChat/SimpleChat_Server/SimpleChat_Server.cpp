#include <iostream>
#include "../SimpleChat_Context.h"
#include <server_infrastructure.h>


class Chat_server : public net::server::IServer<ChatContext>
{
public:
	Chat_server(int port) : net::server::IServer<ChatContext>(port) {}
protected:
	virtual void OnMessage(net::common::Message<ChatContext>* msg, uint64_t sender)
	{
		switch (msg->getHeader().getType())
		{
		case ChatContext::Message:
		{
			char text[150];
			msg->getString(text);
			std::cout << sender << ": " << text << "\n\n";

			this->ForEachClient([&](uint64_t client_id) {
				if (client_id != sender)
				{
					this->Send(*msg, client_id);
				}
				});
			break;
		}
		case ChatContext::Disconnect:
		{
			this->DisconnectClient(sender);
			break;
		}
		default:
			break;
		}
	}

	virtual void OnClientDisconnect(uint64_t client_id)
	{
		std::cout << "Client " << client_id << " disconnected\n";
	}
};

int main()
{
	std::cout << "Server start\n";

	Chat_server server(60000);
	server.Start();

	while (true)
	{
		std::string command;
		std::cin >> command;

		if (command == "q")
			break;
	}

	server.Stop();
}
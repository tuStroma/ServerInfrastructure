#include <iostream>

#include <server_infrastructure.h>
#include "../SimpleChat_Context.h"

class Chat_client : public net::client::IClient<ChatContext>
{
public:
	Chat_client() : net::client::IClient<ChatContext>() {}

protected:
	virtual void OnMessage(net::common::Message<ChatContext>* msg) 
	{
		char text[150];
		msg->getString(text);
		std::cout << text << "\n\n";
	}
	virtual void OnDisconnect() {
		std::cout << "Disconnected\n";
	}
};

int main()
{
	Chat_client client = Chat_client();
	client.Connect("127.0.0.1", 60000);
	while (true)
	{
		std::string command;
		std::cin >> command;

		if (command == "q")
			break;

		if (command == "w")
		{
			std::string s;
			std::getchar();
			std::getline(std::cin, s);

			net::common::Message<ChatContext> msg(Message, s.size() + 1);

			msg.putString(s.c_str());

			client.Send(msg);
		}
	}

	net::common::Message<ChatContext> msg(Disconnect, 0);
	client.Send(msg);

	client.Disconnect();
}


#include <iostream>
#include <string>
#include <unordered_map>

#include <server_infrastructure.h>

template<typename Type>
class client_test : public net::client::IClient<Type>
{
public:
	client_test() : net::client::IClient<Type>() {}
protected:
	virtual void OnMessage(net::common::Message<Type>* msg)
	{
		net::common::Header<int> header = msg->getHeader();
		std::cout << "Message: " << header.getType() << " (" << header.getSize() << "):" << '\n';

		char a[30];
		msg->getString(a);
		std::cout << a << "\n";
	}

	virtual void OnDisconnect()
	{
		std::cout << "Disconnected\n";
	}
};

int main()
{
	client_test<int>* client =  new client_test<int>();
	client->Connect("127.0.0.1", 60000);

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

			net::common::Message<int> msg(69, s.size() + 1);

			msg.putString(s.c_str());

			client->Send(msg);
		}

		if (command == "r")
		{
			net::common::Message<int>* msg;
			bool success = client->Read(msg);
			if (!success)
				std::cout << "No messages\n";
			else
			{
				net::common::Header<int> header = msg->getHeader();
				std::cout << "Message: " << header.getType() << " (" << header.getSize() << "):" << '\n';

				char a[30];
				msg->getString(a);
				std::cout << a << "\n";

			}
		}
	}


	return 0;
}
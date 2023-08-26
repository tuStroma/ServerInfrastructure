#include <iostream>
#include <string>
#include <unordered_map>

#include <server_infrastructure.h>

void simpleClientTest()
{
	/*/
	std::cout << "Client started\n";


	asio::error_code ec;
	asio::io_context context;
	asio::io_context::work idle_work(context);
	std::thread thrContext = std::thread([&]() { context.run(); });
	asio::ip::tcp::endpoint endpoint(asio::ip::make_address("127.0.0.1", ec), 60000);

	asio::ip::tcp::socket socket(context);
	socket.connect(endpoint, ec);

	if (ec)
	{
		std::cout << "Connection failed\n";
		return 0;
	}

	std::cout << "Connected successfully!\n";

	net::common::Connection<int>* connection = new net::common::Connection<int>(std::move(socket));

	uint32_t buffer = 0;

	connection->Read(buffer);


	while (true)
	{
		std::string command;
		std::cin >> command;

		if (command == "q")
			break;

		if (command == "r")
			std::cout << buffer << '\n';

		if (command == "w")
		{
			uint32_t msg;
			std::cin >> msg;
			connection->Write(msg);
		}
	}

	context.stop();
	thrContext.join();
	//*/
}


int main()
{
	net::client::Client<int>* client =  new net::client::Client<int>();
	client->Connect("127.0.0.1", 60000);

	while (true)
	{
		std::string command;
		std::cin >> command;

		if (command == "q")
			break;

		if (command == "w")
		{
			net::common::Message<int> msg(69, 8);

			std::string s;
			std::getline(std::cin, s);
			msg.putString(s.c_str());

			client->Send(&msg);
		}

		if (command == "r")
		{
			net::common::Message<int>* msg;
			bool success = client->Read(&msg);
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
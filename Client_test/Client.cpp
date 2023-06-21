#include <iostream>
#include <string>
#include <server_infrastructure.h>


net::common::Connection<int>* Connect(std::string ip, int port)
{
	asio::error_code ec;
	asio::io_context context;
	asio::io_context::work idle_work(context);
	std::thread thrContext = std::thread([&]() { context.run(); });
	asio::ip::tcp::endpoint endpoint(asio::ip::make_address(ip, ec), port);

	asio::ip::tcp::socket socket(context);
	socket.connect(endpoint, ec);

	if (ec)
	{
		std::cout << "Connection failed\n";
		return nullptr;
	}

	std::cout << "Connected successfully!\n";

	return new net::common::Connection<int>(std::move(socket));
}

int main()
{
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

	return 0;
}
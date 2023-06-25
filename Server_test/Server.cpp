#include <iostream>
#include <string>
#include <list>
#include <server_infrastructure.h>

enum class types
{
	message,
	broadcast,
	invite
};

std::list<net::common::Connection<int>*> connections;

void message_test()
{
	int a = 4;
	float b = 5.9;
	bool c = true;

	std::string s1 = "some text";
	char s2[] = "popop";

	net::common::Message<types> m(types::message, sizeof(a) + sizeof(b) + sizeof(c) + strlen(s1.c_str()) + strlen(s2) + 2);

	std::cout << "a:\t" << m.put(&a, sizeof(a)) << '\n';
	std::cout << "b:\t" << m.put(&b, sizeof(b)) << '\n';
	std::cout << "c:\t" << m.put(&c, sizeof(c)) << '\n';
	std::cout << "c:\t" << m.putString(s1.c_str()) << '\n';
	std::cout << "c:\t" << m.putString(s2) << '\n';
	std::cout << "d:\t" << m.put(&a, 1) << '\n';

	int d;
	float e;
	bool f;
	char s3[20] = { 0 };
	char s4[20] = { 0 };

	m.get(&d, sizeof(d));
	m.get(&e, sizeof(e));
	m.get(&f, sizeof(f));
	m.getString(s3);
	m.getString(s4);

	std::cout << "int d:\t\t" << d << '\n';
	std::cout << "float e:\t" << e << '\n';
	std::cout << "bool f:\t\t" << f << '\n';
	std::cout << "string s1:\t" << s3 << '\n';
	std::cout << "string s2:\t" << s4 << '\n';
}

int main()
{
	std::cout << "Server start\n";

	net::server::Server<int> server(60000);
	server.Start();

	while (true)
	{
		std::string command;
		std::cin >> command;

		if (command == "q")
			break;

		if (command == "w")
		{
			uint32_t msg;
			std::cin >> msg;
			server.Send(msg);
		}
	}

	server.Stop();

	return 0;
}
#include <iostream>
#include <string>
#include <list>
#include <semaphore>
#include <server_infrastructure.h>



enum class types
{
	message,
	broadcast,
	invite
};

std::list<net::common::Connection<int>*> connections;

void server()
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

		if (command == "r")
		{
			int msg;
			bool success = server.Read(&msg);
			if (!success)
				std::cout << "No messages\n";
			else
				std::cout << msg << '\n';
		}
	}

	server.Stop();
}

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

void thread_test()
{
	int count = 0;
	std::binary_semaphore s(1);

	#define t_count 7
	std::thread threads[t_count];

	auto job = [&]() {
		for (int i = 0; i < 10000; i++)
		{
			s.acquire();
			count++;
			s.release();
		}
	};

	for (int i = 0; i < t_count; i++)
		threads[i] = std::thread(job);

	for (int i = 0; i < t_count; i++)
		threads[i].join();

	std::cout << count << '\n';

}

void ts_queue_test()
{
	net::common::ThreadSharedQueue<int> tsq;// = net::common::ThreadSharedQueue<int>();

	std::thread t = std::thread([&]() {
		for (int i = 1; i < 10001; i++)
		tsq.push(i);
		});

	int count = 0;
	while (true)
	{
		int n;
		if (tsq.pop(&n))
		{
			count += n;
			if (n == 10000)
				break;
		}
	}

	t.join();

	std::cout << count << '\n';
}


int main()
{
	server();

	//thread_test();

	//ts_queue_test();

	return 0;
}
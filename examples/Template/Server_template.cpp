#include <server_infrastructure.h>
#include "Context_template.h"


class Server_template : public net::server::IServer<Context_template>
{
public:
	Server_template(int port) : net::server::IServer<Context_template>(port) {}
protected:
	virtual void OnMessage(net::common::Message<Context_template>* msg, uint64_t sender)
	{
		// Process message

		delete msg;
	}

	virtual bool OnClientConnect(std::string address, uint64_t client_id)
	{
		std::cout << "Welcome " << client_id << " with address " << address << "\n";
		return true;
	}

	virtual void OnClientDisconnect(uint64_t client_id)
	{
		std::cout << "Client " << client_id << " disconnected\n";
	}
};

int main()
{
	std::cout << "Server start\n";

	Server_template server(60000);
	server.Start();

	// Server programm

	server.Stop();
}
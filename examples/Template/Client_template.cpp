#include <server_infrastructure.h>
#include "Context_template.h"

class Client_template : public net::client::IClient<Context_template>
{
public:
	Client_template() : net::client::IClient<Context_template>() {}

protected:
	virtual void OnMessage(net::common::Message<Context_template>* msg) 
	{
		// Process message

		delete msg;
	}
	virtual void OnDisconnect() {
		std::cout << "Disconnected\n";
	}
};

int main()
{
	Client_template client = Client_template();
	client.Connect("127.0.0.1", 60000);
	
	// Client programm

	client.Disconnect();
}


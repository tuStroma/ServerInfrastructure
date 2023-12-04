#pragma once


#include "../common/Connection.h"

namespace net
{
	namespace client
	{
		template<typename Type>
		class Client
		{
		private:
			asio::io_context context;
			std::thread thrContext;

			common::ThreadSharedQueue<common::Message<Type>*> incomming_queue;

			common::Connection<Type>* connection = nullptr;

			// Cleanup
			bool closing_connection = false;

		public:
			Client() {}
			~Client() 
			{
				Disconnect();
			}

			bool Connect(std::string ip, uint32_t port)
			{
				asio::error_code ec;
				asio::io_context::work idle_work(context);
				thrContext = std::thread([&]() { context.run(); });
				asio::ip::tcp::endpoint endpoint(asio::ip::make_address(ip, ec), port);

				asio::ip::tcp::socket socket(context);
				socket.connect(endpoint, ec);

				if (ec)
				{
					std::cout << "Connection failed\n";
					return false;
				}

				connection = new common::Connection<Type>(std::move(socket), &incomming_queue);

				connection->Read();

				return true;
			}

			void Disconnect()
			{
				context.stop();
				thrContext.join();

				closing_connection = true;
				if (connection) delete connection;
				connection = nullptr;
				closing_connection = false;

				common::Message<Type>* msg;
				while (incomming_queue.pop(&msg))
					delete msg;
			}

			void Send(common::Message<Type>& msg)
			{
				if (connection && connection->isConnected() && !closing_connection)
					connection->Write(msg);
			}

			bool Read(common::Message<Type>*& destination)
			{
				return incomming_queue.pop(&destination);
			}
		};
	} // client
} // net
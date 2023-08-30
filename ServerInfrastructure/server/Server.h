#pragma once

#include <list>
#include <unordered_map>

#include "../common/Connection.h"

namespace net 
{
	namespace server
	{
		template<typename Type>
		class ClientConnection
		{
		private:
		public:
			common::Connection<Type>* connection;
			uint64_t client_id;

			ClientConnection(common::Connection<Type>* connection, uint64_t id)
				:connection(connection), client_id(id)
			{}
		};

		template<typename Type>
		class Server {
		private:
			// Communication
			asio::io_service service;
			asio::io_context context;
			std::thread context_thread;
			asio::ip::tcp::acceptor acceptor;

			// Server management
			common::Connection<Type>* connection = nullptr;
			common::ThreadSharedQueue<common::Message<Type>*> incomming_queue;

			void WaitForConnection()
			{
				acceptor.async_accept([&](std::error_code ec, asio::ip::tcp::socket socket) {
					if (ec)
					std::cout << "Error while connecting: " << ec.message() << '\n';
					else
					{
						std::cout << "Connected to " << socket.remote_endpoint() << "\n";

						connection = new net::common::Connection<Type>(std::move(socket) , &incomming_queue);

						connection->Read();
					}
				});
			}

		public:
			Server(uint32_t port) 
				:acceptor(context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
			{}
			~Server() {}

			void Start()
			{

				WaitForConnection();

				context_thread = std::thread([&]() { context.run(); });
			}

			void Stop()
			{
				// TODO clean all connections
				context.stop();
				context_thread.join();
			}

			void Send(common::Message<Type>& msg)
			{
				if (connection && connection->isConnected())
					connection->Write(msg);
			}

			bool Read(common::Message<Type>*& destination)
			{
				return incomming_queue.pop(&destination);
			}
		};
	} // server
} // net
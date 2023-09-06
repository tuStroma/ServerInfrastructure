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
			bool is_running = false;

			// Communication
			asio::io_service service;
			asio::io_context context;
			std::thread context_thread;
			asio::ip::tcp::acceptor acceptor;

			// Server management
			common::Connection<Type>* connection = nullptr;
			common::ThreadSharedQueue<common::Message<Type>*> incomming_queue;
			common::ThreadSharedQueue<common::ownedMessage<Type>> incomming_queue_2;	// Owned messages

			// Multiple clients
			uint64_t next_id = 0;
			std::unordered_map<uint64_t, common::Connection<Type>*> connections;

			void WaitForConnection()
			{
				acceptor.async_accept([&](std::error_code ec, asio::ip::tcp::socket socket) {
					if (ec)
					std::cout << "Error while connecting: " << ec.message() << '\n';
					else
					{
						std::cout << "Connected to " << socket.remote_endpoint() << "\n";

						common::Connection<Type>* connection = new net::common::Connection<Type>(std::move(socket) , &incomming_queue);
						connections[next_id++] = connection;

						connection->Read();

						WaitForConnection();
					}
				});
			}

		public:
			Server(uint32_t port) 
				:acceptor(context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
			{}
			~Server() {
				if (is_running)
					Stop();
			}

			void Start()
			{
				is_running = true;

				WaitForConnection();
				context_thread = std::thread([&]() { context.run(); });
			}

			void Stop()
			{
				context.stop();
				context_thread.join();
				if (connection) delete connection;

				common::Message<Type>* msg;
				while (incomming_queue.pop(&msg))
					delete msg;

				is_running = false;
			}

			void Send(common::Message<Type>& msg, uint64_t client_id)
			{
				common::Connection<Type>* connection = connections[client_id];
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
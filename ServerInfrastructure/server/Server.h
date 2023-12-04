#pragma once

#include <list>
#include <unordered_map>

#include "../common/Connection.h"

namespace net 
{
	namespace server
	{
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
			common::ThreadSharedQueue<common::ownedMessage<Type>> incomming_queue;

			// Multiple clients
			uint64_t next_id = 0;
			std::unordered_map<uint64_t, common::Connection<Type>*> connections;

			// Cleanup
			bool closing_connections = false;

			void WaitForConnections()
			{
				acceptor.async_accept([&](std::error_code ec, asio::ip::tcp::socket socket) {
					if (ec)
					std::cout << "Error while connecting: " << ec.message() << '\n';
					else
					{
						std::cout << "Connected to " << socket.remote_endpoint() << "\n";

						common::Connection<Type>* connection = new net::common::Connection<Type>(std::move(socket) , &incomming_queue, next_id);
						connections[next_id++] = connection;

						connection->Read();

						WaitForConnections();
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

				WaitForConnections();
				context_thread = std::thread([&]() { context.run(); });
			}

			void Stop()
			{
				// Closing ASIO context
				context.stop();
				context_thread.join();

				// Closing connections
				closing_connections = true;
				for (std::pair<uint64_t, common::Connection<Type>*> connection : connections)
					delete connection.second;
				connections.clear();
				closing_connections = false;

				// Cleaning incomming queue
				common::ownedMessage<Type> msg;
				while (incomming_queue.pop(&msg))
					delete msg.message;

				is_running = false;
			}

			void Send(common::Message<Type>& msg, uint64_t client_id)
			{
				common::Connection<Type>* connection = connections[client_id];
				if (connection && connection->isConnected() && !closing_connections)
					connection->Write(msg);
					
			}

			bool Read(common::ownedMessage<Type>& destination)
			{
				return incomming_queue.pop(&destination);
			}

			void DisconnectClient(uint64_t client_id)
			{
				delete connections[client_id];
				connections.erase(client_id);
			}
		};
	} // server
} // net
#pragma once

#include <list>
#include <unordered_map>
#include <semaphore>

#include "../common/Connection.h"

namespace net 
{
	namespace server
	{
		template<typename Type>
		class IServer {
		private:
			bool is_running = false;

			// Communication
			asio::io_service service;
			asio::io_context context;
			std::thread context_thread;
			asio::ip::tcp::acceptor acceptor;

			// Multiple clients
			uint64_t next_id = 0;
			std::unordered_map<uint64_t, common::Connection<Type>*> connections;
			std::binary_semaphore connections_lock; // Protects map from simultaneous deletions and insertions

			// Message processing
			common::ThreadSharedQueue<common::ownedMessage<Type>> incomming_queue;
			std::thread worker; bool closing_worker = false;
			std::condition_variable wait_for_messages;

			void WaitForConnections()
			{
				acceptor.async_accept([&](std::error_code ec, asio::ip::tcp::socket socket) {
					if (ec)
					std::cout << "Error while connecting: " << ec.message() << '\n';
					else
					{
						// Verify new client
						if (OnClientConnect(socket.remote_endpoint().address().to_string(), next_id))
						{
							uint64_t current_id = next_id;
							common::Connection<Type>* connection = new net::common::Connection<Type>(std::move(socket),
								[&, current_id](net::common::Message<Type>* msg) // On message
								{
									incomming_queue.push(common::ownedMessage<Type> { msg, current_id });
									wait_for_messages.notify_all();
								},
								[&, current_id]() // On disconnect
								{
									std::thread([&]() { DisconnectClient(current_id); }).detach();
								});
							connections[next_id++] = connection;
							connection->Read();
						}

						WaitForConnections();
					}
				});
			}

			void WorkerJob()
			{
				std::mutex next_messages_m;
				std::unique_lock<std::mutex> lk_for_messages(next_messages_m);

				while (true)
				{
					common::ownedMessage<Type> message;
					while (incomming_queue.pop(&message))
						OnMessage(message.message, message.owner);

					// Wait for next messages
					if (!closing_worker) wait_for_messages.wait(lk_for_messages);
					if (closing_worker) return; // Close worker
				}
			}

			common::Connection<Type>* getConnection(uint64_t client_id)
			{
				common::Connection<Type>* connection = nullptr;

				connections_lock.acquire();

				if (connections.find(client_id) != connections.end())
					connection = connections[client_id];

				connections_lock.release();

				return connection;
			}

		public:
			IServer(uint32_t port) 
				:acceptor(context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)),
				 connections_lock(std::binary_semaphore(1))
			{}
			~IServer() {
				if (is_running)
					Stop();
			}

			void Start()
			{
				if (!is_running)
				{
					is_running = true;

					// Start waiting for connections and open ASIO context
					WaitForConnections();
					context_thread = std::thread([&]() { context.run(); });

					// Start message processing
					closing_worker = false;
					worker = std::thread(&IServer::WorkerJob, this);
				}
			}

			void Stop()
			{
				// Stop message processing
				closing_worker = true;
				wait_for_messages.notify_all();
				worker.join();

				// Closing ASIO context
				context.stop();
				context_thread.join();

				// Closing connections
				connections_lock.acquire();
				for (std::pair<uint64_t, common::Connection<Type>*> connection : connections)
					delete connection.second;
				connections.clear();
				connections_lock.release();

				// Cleaning incomming queue
				common::ownedMessage<Type> msg;
				while (incomming_queue.pop(&msg))
					delete msg.message;

				is_running = false;
			}

			void Send(common::Message<Type>& msg, uint64_t client_id)
			{
				common::Connection<Type>* connection = getConnection(client_id);

				connections_lock.acquire();
				if (connection && connection->isConnected())
				{
					connection->Write(msg);
					connections_lock.release();
				}
				else
				{
					connections_lock.release();
					DisconnectClient(client_id);
				}
			}

			void DisconnectClient(uint64_t client_id)
			{
				common::Connection<Type>* connection = getConnection(client_id);
				if (connection)
				{
					connections_lock.acquire();
					connections.erase(client_id);
					delete connection;
					connections_lock.release();

					OnClientDisconnect(client_id);
				}
			}


			// Server interface
			protected:
			virtual void OnMessage(net::common::Message<Type>* msg, uint64_t client_id) {}
			virtual bool OnClientConnect(std::string address, uint64_t client_id) { return true; }
			virtual void OnClientDisconnect(uint64_t client_id) {}

			// Perform an action for every client
			void ForEachClient(std::function<void(uint64_t)> const & execute)
			{
				std::list<uint64_t> clients;

				// Create clients' id list
				connections_lock.acquire();

				for (auto& [id, connection] : connections)
					clients.push_back(id);

				connections_lock.release();

				// Execute for every client
				for (uint64_t client : clients)
					execute(client);
			}
		};
	} // server
} // net
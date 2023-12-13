#pragma once


#include "../common/Connection.h"

namespace net
{
	namespace client
	{
		template<typename Type>
		class IClient
		{
		private:
			asio::io_context context;
			std::thread thrContext;

			common::ThreadSharedQueue<common::Message<Type>*> incomming_queue;

			common::Connection<Type>* connection = nullptr;

			// Cleanup
			bool closing_connection = false;

			// Message processing
			std::thread worker; bool closing_worker = false;
			std::condition_variable wait_for_messages;

			void WorkerJob()
			{
				std::mutex next_messages_m;
				std::unique_lock<std::mutex> lk_for_messages(next_messages_m);

				while (true)
				{
					common::Message<Type>* message;
					while (incomming_queue.pop(&message))
						OnMessage(message);

					// Wait for next messages
					wait_for_messages.wait(lk_for_messages);

					// Close worker
					if (closing_worker) return;
				}
			}

		public:
			IClient() {}
			~IClient() 
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

				connection = new common::Connection<Type>(std::move(socket), [&](net::common::Message<Type>* msg) {
					incomming_queue.push(msg);
					wait_for_messages.notify_all();
					});

				// Start message processing
				closing_worker = false;
				worker = std::thread(&IClient::WorkerJob, this);

				// Start listening
				connection->Read();

				return true;
			}

			void Disconnect()
			{
				// Stop message processing
				closing_worker = true;
				wait_for_messages.notify_all();
				if (worker.joinable()) worker.join();

				// Closing ASIO context
				context.stop();
				if (thrContext.joinable()) thrContext.join();

				// Closing connection
				bool connection_was_open = (bool)connection;
				closing_connection = true;
				if (connection) delete connection;
				connection = nullptr;
				closing_connection = false;

				// Cleaning incomming queue
				common::Message<Type>* msg;
				while (incomming_queue.pop(&msg))
					delete msg;

				if (connection_was_open)
					OnDisconnect();
			}

			void Send(common::Message<Type>& msg)
			{
				if (connection && connection->isConnected() && !closing_connection)
					connection->Write(msg);
				else
					Disconnect();
			}

			bool Read(common::Message<Type>*& destination)
			{
				return incomming_queue.pop(&destination);
			}


			// Server interface
		protected:
			virtual void OnMessage(net::common::Message<Type>* msg) {}
			virtual void OnDisconnect() {}
		};
	} // client
} // net
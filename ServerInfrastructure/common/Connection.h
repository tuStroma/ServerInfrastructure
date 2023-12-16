#pragma once
#include <iostream>
#include <list>
#include <asio.hpp>

#include "Message.h"
#include "ThreadSharedQueue.h"

namespace net
{
	namespace common
	{
		template<typename communication_context>
		struct ownedMessage
		{
			Message<communication_context>* message;
			uint64_t owner;
		};

		template<typename communication_context>
		class Connection
		{
		private:
			// Communication
			asio::ip::tcp::socket socket;

			// Receiving messages
			Header<communication_context> header_buffer;
			Message<communication_context>* message_buffer;

			// Execute on async events
			std::function<void(Message<communication_context>*)> onMessage;
			std::function<void()> onDisconnect;

			// Sending messages
			std::thread sender_thread; bool finish_sending_job = false;
			std::condition_variable wait_for_messages, wait_till_sent;
			net::common::ThreadSharedQueue<Message<communication_context>*> message_sending_queue;
			Message<communication_context>* msg_to_send = nullptr;


			void SendingJob()
			{
				std::mutex next_messages_m, till_sent_m;
				std::unique_lock<std::mutex> lk_for_messages(next_messages_m), lk_till_sent(till_sent_m);

				while (true)
				{
					// Send all messages
					while (message_sending_queue.pop(&msg_to_send))
					{
						WriteHeader();
						// Wait until the message is sent (not to override "msg_to_send")
						if (!finish_sending_job) wait_till_sent.wait(lk_till_sent);
						if (finish_sending_job) return;
					}
					
					// And wait for next messages
					if (!finish_sending_job) wait_for_messages.wait(lk_for_messages);
					if (finish_sending_job) return;
				}
			}

			void closeSender()
			{
				finish_sending_job = true;
				wait_for_messages.notify_all();
				wait_till_sent.notify_all();
				sender_thread.join();
			}

			void ReadHeader()
			{
				asio::async_read(socket, asio::buffer(&header_buffer, sizeof(header_buffer)), [&](std::error_code ec, std::size_t length) {
					if (ec)
					{
						if (ec.value() == 10054) onDisconnect(); // Connection forcebly closed by remote host
						return;
					}

					message_buffer = new Message<communication_context>(header_buffer);

					if(header_buffer.getSize() == 0)
					{
						onMessage(message_buffer);
						ReadHeader();
					}
					else
						ReadBody();
					});
			}

			void ReadBody()
			{
				asio::async_read(socket, asio::buffer(message_buffer->getBody(), message_buffer->getSize()), [&](std::error_code ec, std::size_t length) {
					if (ec)
					{
						if (ec.value() == 10054) onDisconnect(); // Connection forcebly closed by remote host
						return;
					}

					onMessage(message_buffer);
					ReadHeader();
					});
			}

			void WriteHeader()
			{
				Header<communication_context> header = msg_to_send->getHeader();
				asio::async_write(socket, asio::buffer(&header, sizeof(header)), [&](std::error_code ec, std::size_t length) {
					if (ec)
					{
						if (ec.value() == 10054) onDisconnect(); // Connection forcebly closed by remote host
						return;
					}

					if (header.getSize() == 0)
					{
						delete msg_to_send;
						wait_till_sent.notify_one();
					}
					else
						WriteBody();
					});
			}

			void WriteBody()
			{
				asio::async_write(socket, asio::buffer(msg_to_send->getBody(), msg_to_send->getHeader().getSize()), [&](std::error_code ec, std::size_t length) {
					if (ec)
					{
						if (ec.value() == 10054) onDisconnect(); // Connection forcebly closed by remote host
						return;
					}

					delete msg_to_send;
					wait_till_sent.notify_one();
					});
			}

		public:
			Connection(	asio::ip::tcp::socket socket,
						std::function<void(Message<communication_context>*)> const& onMessage,
						std::function<void()> const& onDisconnect)
				:socket(std::move(socket)), onMessage(onMessage), onDisconnect(onDisconnect)
			{
				sender_thread = std::thread(&Connection::SendingJob, this);
			}

			~Connection()
			{
				// Close sending job
				closeSender();

				// Close socket
				if (socket.is_open()) socket.close();

				// Delete all messages
				Message<communication_context>* msg;
				while (message_sending_queue.pop(&msg))
					delete msg;
			}

			bool isConnected()
			{
				return socket.is_open();
			}

			void Read()
			{
				ReadHeader();
			}

			void Write(Message<communication_context>& msg)
			{
				message_sending_queue.push(new Message<communication_context>(msg));
				wait_for_messages.notify_one();
			}
		};

	} // common
} // net
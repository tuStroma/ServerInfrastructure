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
			asio::ip::tcp::socket socket;

			// Receiving messages

			Header<communication_context> header_buffer;
			Message<communication_context>* message_buffer;

			bool is_server_connection = false;

			// Client Connection (raw messages)
			net::common::ThreadSharedQueue<Message<communication_context>*>* client_message_destination = nullptr;

			// Server Connection (owned messages)
			net::common::ThreadSharedQueue<ownedMessage<communication_context>>* server_message_destination = nullptr;
			uint64_t client_id;

			// Sending messages
			std::thread sender_thread; bool finish_sending = false;
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
						wait_till_sent.wait(lk_till_sent);
						if (finish_sending) return;
					}
					
					// And wait for next messages
					wait_for_messages.wait(lk_for_messages);
					if (finish_sending) return;
				}
			}

			void closeSender()
			{
				finish_sending = true;
				wait_for_messages.notify_one();
				wait_till_sent.notify_one();
				sender_thread.join();
			}

			void saveMessage()
			{
				if (is_server_connection)
					server_message_destination->push(ownedMessage<communication_context> { message_buffer, client_id });
				else
					client_message_destination->push(message_buffer);
			}

			void ReadHeader()
			{
				asio::async_read(socket, asio::buffer(&header_buffer, sizeof(header_buffer)), [&](std::error_code ec, std::size_t length) {
					if (ec)
					{
						socket.close();
						return;
					}

					message_buffer = new Message<communication_context>(header_buffer);

					if(header_buffer.getSize() == 0)
					{
						saveMessage();
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
						socket.close();
						return;
					}

					saveMessage();
					ReadHeader();
					});
			}

			void WriteHeader()
			{
				Header<communication_context> header = msg_to_send->getHeader();
				asio::async_write(socket, asio::buffer(&header, sizeof(header)), [&](std::error_code ec, std::size_t length) {
					if (ec)
					{
						socket.close();
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
						socket.close();
						return;
					}

					delete msg_to_send;
					wait_till_sent.notify_one();
					});
			}

		public:
			// Client Connection
			Connection(asio::ip::tcp::socket socket, ThreadSharedQueue<Message<communication_context>*>* destination_queue)
				:socket(std::move(socket)), client_message_destination(destination_queue)
			{
				sender_thread = std::thread(&Connection::SendingJob, this);
			}

			// Server Connection
			Connection(asio::ip::tcp::socket socket, ThreadSharedQueue<ownedMessage<communication_context>>* destination_queue, uint64_t client_id)
				:socket(std::move(socket)), server_message_destination(destination_queue), client_id(client_id), is_server_connection(true)
			{
				sender_thread = std::thread(&Connection::SendingJob, this);
			}

			~Connection()
			{
				// Close sending job
				closeSender();

				// Close socket
				socket.close();

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
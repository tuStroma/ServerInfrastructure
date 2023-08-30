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
		class Connection
		{
		private:
			asio::ip::tcp::socket socket;

			net::common::ThreadSharedQueue<Message<communication_context>*>* message_destination;

			Header<communication_context> header_buffer;
			Message<communication_context>* message_buffer;

			// Sending messages
			std::thread sender_thread;
			std::condition_variable sender_cv, wait_till_sent;
			net::common::ThreadSharedQueue<Message<communication_context>*> message_sending_queue;
			Message<communication_context>* msg_to_send = nullptr;

			void SendingJob()
			{
				std::mutex sender_m, till_sent_m;
				std::unique_lock<std::mutex> lk_sender(sender_m), lk_till_sent(till_sent_m);
				std::cout << "Hi, it's sender\n";

				while (true)
				{

					while (message_sending_queue.pop(&msg_to_send))
					{
						WriteHeader();
						wait_till_sent.wait(lk_till_sent);
					}
					
					sender_cv.wait(lk_sender);
				}
				
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

					message_destination->push(message_buffer);
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
					wait_till_sent.notify_one();
					});
			}

		public:
			Connection(asio::ip::tcp::socket socket, ThreadSharedQueue<Message<communication_context>*>* destination_queue)
				:socket(std::move(socket)), message_destination(destination_queue)
			{
				sender_thread = std::thread(&Connection::SendingJob, this);
				//WriteHeader();
			}

			~Connection()
			{
				sender_thread.join();
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
				sender_cv.notify_one();

				//msg_to_send = &msg;
				//WriteHeader();
			}
		};

	} // common
} // net
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

			// ASIO tasks
			std::binary_semaphore sending_lock;
			std::binary_semaphore reading_lock;


			void ReadHeader()
			{
				asio::async_read(socket, asio::buffer(&header_buffer, sizeof(header_buffer)), [&](std::error_code ec, std::size_t length) {
					if (ec)
					{
						onDisconnect();
						reading_lock.release();
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
						onDisconnect();
						reading_lock.release();
						return;
					}

					onMessage(message_buffer);
					ReadHeader();
					});
			}

			void WriteHeader(Message<communication_context>* msg)
			{
				Header<communication_context> header = msg->getHeader();
				asio::async_write(socket, asio::buffer(&header, sizeof(header)), [&, msg](std::error_code ec, std::size_t length) {
					if (ec)
					{
						onDisconnect();
						sending_lock.release();
						return;
					}

					if (msg->getHeader().getSize() == 0)
					{
						delete msg;
						sending_lock.release();
					}
					else
						WriteBody(msg);
					});
			}

			void WriteBody(Message<communication_context>* msg)
			{
				asio::async_write(socket, asio::buffer(msg->getBody(), msg->getHeader().getSize()), [&, msg](std::error_code ec, std::size_t length) {
					if (ec)
					{
						onDisconnect();
						sending_lock.release();
						return;
					}

					delete msg;
					sending_lock.release();
					});
			}

		public:
			Connection(	asio::ip::tcp::socket socket,
						std::function<void(Message<communication_context>*)> const& onMessage,
						std::function<void()> const& onDisconnect)
				:socket(std::move(socket)), onMessage(onMessage), onDisconnect(onDisconnect), sending_lock(std::binary_semaphore(1)), reading_lock(std::binary_semaphore(0))
			{}

			~Connection()
			{
				// Close socket
				if (socket.is_open()) socket.close();

				// Wait for all asio jobs to return
				reading_lock.acquire();
				sending_lock.acquire();
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
				Message<communication_context>* copied_msg = new Message<communication_context>(msg);

				sending_lock.acquire();
				WriteHeader(copied_msg);
			}
		};

	} // common
} // net
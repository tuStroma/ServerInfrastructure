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
		template<typename Type>
		class Connection
		{
		private:
			asio::ip::tcp::socket socket;

			net::common::ThreadSharedQueue<Type>* message_destination;

			uint32_t buffer = 0;

		public:
			Connection(asio::ip::tcp::socket socket, ThreadSharedQueue<Type>* destination_queue)
				:socket(std::move(socket)), message_destination(destination_queue)
			{}

			bool isConnected()
			{
				return socket.is_open();
			}

			void Read()
			{
				asio::async_read(socket, asio::buffer(&buffer, sizeof(buffer)), [&](std::error_code ec, std::size_t length) {
					if (ec)
					{
						socket.close();
						return;
					}

					std::cout << "Recv:\t" << buffer << std::endl;
					message_destination->push(buffer);
					Read();
					});
			}

			void Write(uint32_t buffer)
			{
				std::cout << "Sending: " << buffer << '\n';
				asio::async_write(socket, asio::buffer(&buffer, sizeof(buffer)), [&](std::error_code ec, std::size_t length) {	// NOTE: 'buffer' variable is passed by refference,
					if (ec)																										// when lambda is executed it's value may change
					{
						socket.close();
						return;
					}
					std::cout << "Send:\t" << buffer << std::endl;
					});
			}
		};

	} // common
} // net
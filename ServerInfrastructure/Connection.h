#pragma once
#include <iostream>
#include <asio.hpp>

#include "Message.h"

namespace net
{
	namespace common
	{
		template<typename Type>
		class Connection
		{
		private:
			asio::ip::tcp::socket socket;

		public:
			Connection(asio::ip::tcp::socket socket) 
				:socket(std::move(socket))
			{}

			// Test only
			asio::ip::tcp::socket* getSocket()
			{
				return &socket;
			}

			void Read(uint32_t& buffer)
			{
				asio::async_read(socket, asio::buffer(&buffer, sizeof(buffer)), [&](std::error_code ec, std::size_t length) {
					std::cout << "Recv:\t" << buffer << std::endl;
					});
			}

			void Write(uint32_t buffer)
			{
				std::cout << "Sending: " << buffer << '\n';
				asio::async_write(socket, asio::buffer(&buffer, sizeof(buffer)), [&](std::error_code ec, std::size_t length) {	// NOTE: 'buffer' variable is passed by refference,
					std::cout << "Send:\t" << buffer << std::endl;																// when lambda is executed it's value may change
					});
			}
		};

	} // common
} // net
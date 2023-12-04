#pragma once

#include <iostream>

namespace net
{
	namespace common
	{
		template<typename Type>
		class Header
		{
		private:
			Type type = Type();
			size_t size = 0;

		public:
			Header() {}
			Header(Type type, size_t size)
				:type(type), size(size)
			{}

			size_t getSize() { return size; }
			Type getType() { return type; }
		};

		template<typename Type>
		class Message
		{
		private:
			Header<Type> header;
			void* body;

			uint32_t write_offset = 0;
			uint32_t read_offset = 0;

		public:
			Message() {}
			Message(Type type, size_t size)
			{
				header = Header<Type>(type, size);
				body = malloc(size);
				if (!body)
				{
					std::cerr << "Failed to allocate memory\n";
					exit(-1);
				}
			}

			Message(Header<Type> header)
				:header(header)
			{
				body = malloc(header.getSize());
				if (!body)
				{
					std::cerr << "Failed to allocate memory\n";
					exit(-1);
				}
			}

			Message(Message<Type>& msg)
			{
				header = msg.getHeader();
				body = malloc(header.getSize());
				if (!body)
				{
					std::cerr << "Failed to allocate memory\n";
					exit(-1);
				}

				std::memcpy(body, msg.getBody(), header.getSize());
			}

			~Message()
			{
				free(body);
			}

			bool put(void* source, size_t size)
			{
				uint32_t next_offset = write_offset + size;
				if (next_offset > header.getSize())
					return false;

				std::memcpy((char*)body + write_offset, source, size);
				write_offset = next_offset;
				return true;
			}

			bool putString(const char* source)
			{
				size_t size = strlen(source) + 1;
				return put((void*)source, size);
			}

			bool get(void* destination, size_t size)
			{
				uint32_t next_offset = read_offset + size;
				if (next_offset > header.getSize())
					return false;

				std::memcpy(destination, (char*)body + read_offset, size);
				read_offset = next_offset;
				return true;
			}

			bool getString(char* destination)
			{
				char* str = (char*)body + read_offset;
				uint32_t size = std::strlen(str) + 1;
				return get(destination, size);
			}

			Header<Type> getHeader() { return header; }

			void* getBody() { return body; }

			size_t getSize() { return header.getSize(); }
		};
	} // common
} // net

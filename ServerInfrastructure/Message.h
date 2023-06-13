#pragma once

#include <iostream>

namespace net
{
	namespace common
	{
		template<typename Type>
		class Message
		{
		private:
			template<typename Type>
			class Header
			{
			private:
				Type type;
				size_t size = 0;

			public:
				Header() {}
				Header(Type type, size_t size)
					:type(type), size(size)
				{}

				uint32_t getSize() { return size; }
			};

			Header<Type> header;
			void* body;

			uint32_t write_offset = 0;
			uint32_t read_offset = 0;

		public:
			Message(Type type, size_t size)
			{
				header = Header<Type>(type, size);
				body = malloc(size);
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
		};
	} // common
} // net

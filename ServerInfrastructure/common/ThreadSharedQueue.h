#pragma once

#include <iostream>
#include <list>
#include <semaphore>

namespace net
{
	namespace common
	{
		// Queue suitable to use 
		// by multiple threads
		template<typename T>
		class ThreadSharedQueue
		{
		private:
			std::list<T> queue;
			std::binary_semaphore semaphore;
		public:
			ThreadSharedQueue()
				: semaphore(std::binary_semaphore(1))
			{}

			void push(T item)
			{
				semaphore.acquire();
				queue.push_back(item);
				semaphore.release();
			}

			bool pop(T* destination)
			{
				semaphore.acquire();
				if (queue.size() == 0)
				{ 
					semaphore.release();
					return false;
				}

				*destination = queue.front();
				queue.pop_front();


				semaphore.release();
				return true;
			}
		};
	} // common
} // net
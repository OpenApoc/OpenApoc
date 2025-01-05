#pragma once

#include "framework/framework.h"
#include <mutex>
#include <optional>
#include <queue>

namespace OpenApoc
{

template <typename T, size_t max_size = 0, bool blocks = false> class locked_queue
{
  private:
	std::queue<T> queue;
	std::mutex mutex;
	std::condition_variable cv;
	bool quit = false;

  public:
	locked_queue() = default;
	~locked_queue()
	{
		{
			{
				std::unique_lock lk(mutex);
				quit = true;
			}
			cv.notify_all();
			{
				std::unique_lock lk(mutex);
			}
		}

		cv.notify_all();
	}

	std::optional<T> pop()
	{
		std::optional<T> result;
		{
			std::unique_lock lock(mutex);
			cv.wait(lock, [&] { return !queue.empty() || quit; });
			if (!quit)

			{
				result = queue.front();
				queue.pop();
			}
		}
		cv.notify_all();
		return result;
	}

	bool push(const T &obj)
	{
		{
			std::unique_lock lock(mutex);
			if (max_size != 0)
			{
				if (blocks)
				{
					cv.wait(lock, [&] { return queue.size() < max_size || quit; });
				}
				else
				{
					if (queue.size() >= max_size)
					{
						LogWarning("Dropping object");
						return false;
					}
				}
			}
			if (quit)
			{
				return false;
			}
			queue.push(obj);
		}

		cv.notify_all();
		return true;
	}
};

} // namespace OpenApoc

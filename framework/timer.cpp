#include "framework/timer.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "library/strings_format.h"

namespace OpenApoc
{

static Uint32 sdl_timer_callback(const Uint32 interval, void *param)
{
	auto info = static_cast<Framework::sdl_callback_info *>(param);
	info->callback();
	return interval;
}

TimerID start_timer(Framework &fw, std::chrono::duration<uint32_t, std::milli> interval,
                    std::function<void()> callback)
{
	auto callback_info = mkup<Framework::sdl_callback_info>();
	LogInfo("Start timer, interval %d", interval.count());
	callback_info->callback = callback;
	const TimerID id{SDL_AddTimer(interval.count(), sdl_timer_callback, callback_info.get())};
	if (id == 0)
	{
		LogError("Failed to start timer: %s", SDL_GetError());
	}
	fw.timer_data[id] = std::move(callback_info);
	LogInfo("Started timer, ID %d", (int)id);
	return id;
}

void destroy_timer(Framework &fw, TimerID id)
{
	LogInfo("End timer");
	SDL_RemoveTimer(id);
	fw.timer_data.erase(id);
}

} // namespace OpenApoc
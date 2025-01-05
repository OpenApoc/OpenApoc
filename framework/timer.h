#pragma once

#include <SDL.h>
#include <chrono>
#include <functional>

namespace OpenApoc
{
class Framework;
using TimerID = SDL_TimerID;

TimerID start_timer(Framework &fw, std::chrono::duration<uint32_t, std::milli> interval,
                    std::function<void()> callback);

void destroy_timer(Framework &fw, TimerID id);

} // namespace OpenApoc
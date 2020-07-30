#include "framework/logger_sdldialog.h"
#include "framework/configfile.h"
#include "framework/logger.h"
#include "framework/options.h"

#include <SDL_messagebox.h>
#include <atomic>

namespace OpenApoc
{

namespace
{

LogFunction previousFunction; // To allow chaining log functions

std::atomic<SDL_Window *> parentWindow = nullptr;
LogLevel dialogLogLevel = LogLevel::Nothing;

void SDLDialogLogFunction(LogLevel level, UString prefix, const UString &text)
{
	previousFunction(level, prefix, text);
	if (level > dialogLogLevel)
	{
		return;
	}
	auto message = OpenApoc::format("%s: %s", prefix, text);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "OpenApoc error", message.c_str(), parentWindow);
}

} // namespace

void enableSDLDialogLogger(SDL_Window *win)
{
	LogAssert(win);
	if (parentWindow)
	{
		LogError("SDL Dialog already enabled");
		return;
	}
	parentWindow = win;
	dialogLogLevel = (LogLevel)Options::dialogLogLevelOption.get();
	previousFunction = getLogCallback();
	setLogCallback(SDLDialogLogFunction);
}

} // namespace OpenApoc

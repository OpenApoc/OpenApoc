#include "forms/ui.h"
#include "framework/framework.h"
#include "framework/trace.h"
#include "game/ui/boot.h"
#include "game/ui/city/cityview.h"
#include "version.h"
#include <SDL_main.h>

using namespace OpenApoc;

int main(int argc, char *argv[])
{
	bool enable_trace = false;
	LogInfo("Starting OpenApoc \"%s\"", OPENAPOC_VERSION);
	std::vector<UString> cmdline;
	size_t frameLimit = 0;
	UString saveFile;

	for (int i = 1; i < argc; i++)
	{
		// Special handling of tracing as we want it to be started before the framework
		// parses the rest of the options
		if (UString(argv[i]) == "--enable-tracing")
		{
			enable_trace = true;
			continue;
		}
		else if (UString(argv[i]) == "--disable-tracing")
		{
			enable_trace = false;
			continue;
		}
		else if (UString(argv[i]).substr(0, strlen("--frame-limit=")) == "--frame-limit=")
		{
			auto split = UString(argv[i]).split('=');
			if (split.size() != 2)
			{
				LogWarning("Error reading argument \"%s\", skipping frame limit", argv[i]);
				continue;
			}
			frameLimit = Strings::ToInteger(split[1]);
			LogWarning("Quitting after %llu frames", (unsigned long long)frameLimit);
			continue;
		}
		else if (UString(argv[i]).substr(0, strlen("--load-save=")) == "--load-save=")
		{
			auto split = UString(argv[i]).split('=');
			if (split.size() != 2)
			{
				LogWarning("Error reading argument \"%s\", skipping save", argv[i]);
				continue;
			}
			frameLimit = Strings::ToInteger(split[1]);
			saveFile = split[1];
			LogWarning("Loading save \"%s\" ", saveFile.c_str());
			continue;
		}
		cmdline.emplace_back(UString(argv[i]));
	}

	if (enable_trace)
	{
		Trace::enable();
		LogInfo("Tracing enabled");
	}

	{
		Trace::setThreadName("main");

		TraceObj obj("main");
		Framework *fw = new Framework(UString(argv[0]), cmdline, true);

		if (saveFile == "")
		{
			fw->Run(mksp<BootUp>(), frameLimit);
		}
		else
		{
			auto state = mksp<GameState>();
			if (state->loadGame(saveFile) == false)
			{
				LogError("Failed to load '%s'", saveFile.c_str());
				return EXIT_FAILURE;
			}
			state->initState();
			fw->Run(mksp<CityView>(state, StateRef<City>{state.get(), "CITYMAP_HUMAN"}),
			        frameLimit);
		}

		UI::unload();
		delete fw;
#ifdef DUMP_TRANSLATION_STRINGS
		dumpStrings();
#endif
	}

	if (enable_trace)
	{
		Trace::disable();
	}

	return 0;
}

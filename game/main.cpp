#include "forms/ui.h"
#include "framework/configfile.h"
#include "framework/framework.h"
#include "framework/trace.h"
#include "game/ui/boot.h"
#include "game/ui/tileview/cityview.h"
#include "version.h"
#include <SDL_main.h>

using namespace OpenApoc;

int main(int argc, char *argv[])
{
	if (config().parseOptions(argc, argv))
	{
		return EXIT_FAILURE;
	}
	LogInfo("Starting OpenApoc \"%s\"", OPENAPOC_VERSION);

	{
		Trace::setThreadName("main");

		TraceObj obj("main");
		up<Framework> fw(new Framework(UString(argv[0]), true));

		fw->run(mksp<BootUp>());

		UI::unload();
#ifdef DUMP_TRANSLATION_STRINGS
		dumpStrings();
#endif
	}

	Trace::disable();

	return 0;
}

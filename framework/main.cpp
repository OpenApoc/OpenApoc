#include <physfs.h>
#include "framework/framework.h"
#include "framework/trace.h"
#include "version.h"
#include <SDL_main.h>

using namespace OpenApoc;

int main(int argc, char *argv[])
{
	bool enable_trace = false;
	LogInfo("Starting OpenApoc \"%s\"", OPENAPOC_VERSION);
	std::vector<UString> cmdline;

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
		cmdline.emplace_back(UString(argv[i]));
	}

	if (enable_trace)
	{
		Trace::enable();
		LogInfo("Tracing enabled");
	}

	Trace::setThreadName("main");

	TraceObj obj("main");

	Framework *fw = new Framework(UString(argv[0]), cmdline);
	fw->Run();
	delete fw;
	return 0;
}

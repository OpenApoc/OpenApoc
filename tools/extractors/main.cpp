#include "framework/framework.h"
#include "framework/trace.h"
#include "tools/extractors/extractors.h"
#include <SDL_main.h>

using namespace OpenApoc;

int main(int argc, char *argv[])
{
	bool enable_trace = false;
	LogInfo("Starting OpenApoc_DataExtractor");
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

	{

		Trace::setThreadName("main");

		TraceObj obj("main");
		Framework *fw = new Framework(UString(argv[0]), cmdline, false);
		InitialGameStateExtractor e;

		auto bullet_sprites = e.extractBulletSprites();

		for (auto &sprite_pair : bullet_sprites)
		{
			auto path = "data/" + sprite_pair.first;
			fw->data->writeImage(path, sprite_pair.second);
		}

		std::map<UString, InitialGameStateExtractor::Difficulty> difficultyOutputFiles = {
		    {"data/difficulty1", InitialGameStateExtractor::Difficulty::DIFFICULTY_1},
		    {"data/difficulty2", InitialGameStateExtractor::Difficulty::DIFFICULTY_2},
		    {"data/difficulty3", InitialGameStateExtractor::Difficulty::DIFFICULTY_3},
		    {"data/difficulty4", InitialGameStateExtractor::Difficulty::DIFFICULTY_4},
		    {"data/difficulty5", InitialGameStateExtractor::Difficulty::DIFFICULTY_5},
		};

		std::list<std::future<void>> tasks;

		// auto dpair = std::pair<UString,
		// InitialGameStateExtractor::Difficulty>("data/difficulty5",
		// InitialGameStateExtractor::Difficulty::DIFFICULTY_5);
		for (auto &dpair : difficultyOutputFiles)
		{
			auto future = fw->threadPool->enqueue([dpair, &e]() {
				GameState s;
				LogWarning("Extracting initial game state for \"%s\"", dpair.first.cStr());
				e.extract(s, dpair.second);
				LogWarning("Finished extracting initial game state for \"%s\"", dpair.first.cStr());

				LogWarning("Importing common patch");
				s.loadGame("data/common_patch");
				LogWarning("Done importing common patch");

				UString patchName = dpair.first + "_patch";
				LogWarning("Trying to import patch \"%s\"", patchName.cStr());
				s.loadGame(patchName);
				LogWarning("Patching finished");

				UString patchedOutputName = dpair.first + "_patched";
				LogWarning("Saving patched state to \"%s\"", patchedOutputName.cStr());
				s.saveGame(patchedOutputName, false);
				LogWarning("Done saving patched state");
			});
			tasks.push_back(std::move(future));
		}

		for (auto &task : tasks)
			task.wait();

		delete fw;
	}

	if (enable_trace)
	{
		Trace::disable();
	}

	return 0;
}

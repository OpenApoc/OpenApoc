#include "framework/framework.h"
#include "framework/trace.h"
#include "game/city/city.h"
#include "game/resources/gamecore.h"
#include "tools/extractors/extractors.h"
#include "version.h"
#include <SDL_main.h>

using namespace OpenApoc;

int main(int argc, char *argv[])
{
	bool enable_trace = false;
	bool serialization_test = false;
	bool extract_data = false;
	bool run_game = true;
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
		else if (UString(argv[i]) == "--serialization-test")
		{
			run_game = false;
			serialization_test = true;
			// serialization-test is on top of extract-data
			extract_data = true;
			continue;
		}
		else if (UString(argv[i]) == "--extract-data")
		{
			run_game = false;
			extract_data = true;
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
		Framework *fw = new Framework(UString(argv[0]), cmdline);

		if (extract_data)
		{
			// FIXME: Remove ruleset
			UString ruleset = fw->Settings->getString("GameRules");
			fw->gamecore.reset(new GameCore());
			fw->gamecore->Load(ruleset);
			std::map<UString, InitialGameStateExtractor::Difficulty> difficultyOutputFiles = {
			    {"data/difficulty1", InitialGameStateExtractor::Difficulty::DIFFICULTY_1},
			    {"data/difficulty2", InitialGameStateExtractor::Difficulty::DIFFICULTY_2},
			    {"data/difficulty3", InitialGameStateExtractor::Difficulty::DIFFICULTY_3},
			    {"data/difficulty4", InitialGameStateExtractor::Difficulty::DIFFICULTY_4},
			    {"data/difficulty5", InitialGameStateExtractor::Difficulty::DIFFICULTY_5},
			};

			for (auto &dpair : difficultyOutputFiles)
			{
				GameState s;
				InitialGameStateExtractor e;
				LogWarning("Extracting initial game state for \"%s\"", dpair.first.c_str());
				e.extract(s, dpair.second);
				LogWarning("Finished extracting initial game state for \"%s\"",
				           dpair.first.c_str());

				if (serialization_test)
				{
					LogWarning("Saving initial state to \"%s\"", dpair.first.c_str());
					s.saveGame(dpair.first);
					LogWarning("Done saving initial state");
				}

				LogWarning("Importing common patch");
				s.loadGame("data/common_patch");
				LogWarning("Done importing common patch");

				UString patchName = dpair.first + "_patch";
				LogWarning("Trying to import patch \"%s\"", patchName.c_str());
				s.loadGame(patchName);
				LogWarning("Patching finished");

				UString patchedOutputName = dpair.first + "_patched";
				LogWarning("Saving patched state to \"%s\"", patchedOutputName.c_str());
				s.saveGame(patchedOutputName);
				LogWarning("Done saving patched state");

				if (serialization_test)
				{
					auto outPath2 = patchedOutputName + "2";

					GameState s2;

					LogWarning("Running serialization-in test");
					s2.loadGame(patchedOutputName);
					LogWarning("serialization-in done");

					LogWarning("Running serialization-out2 test");
					s2.saveGame(outPath2);
					LogWarning("serialization-out2 done");

					LogWarning("'starting' game");
					s2.startGame();
					s2.initState();
					LogWarning("Finished starting game");

					LogWarning("Saving 'in progress' game");
					s2.saveGame(dpair.first + "_saved");
					LogWarning("Done saving game");

					GameState s3;
					LogWarning("Loading 'in progress' game");
					s3.loadGame(dpair.first + "_saved");
					LogWarning("Done loading 'in progress' game");

					LogWarning("Re-saving 'in progress' game");
					s3.saveGame(dpair.first + "_saved2");
					LogWarning("Done re-saving 'in progress' game");
				}
			}
		}

		if (run_game)
		{
			fw->Run();
		}
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

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include "game/ui/boot.h"
#include "forms/ui.h"
#include "framework/configfile.h"
#include "framework/data.h"
#include "framework/framework.h"
#include "framework/jukebox.h"
#include "framework/modinfo.h"
#include "framework/options.h"
#include "game/state/gamestate.h"
#include "game/ui/general/loadingscreen.h"
#include "game/ui/general/mainmenu.h"
#include "game/ui/general/videoscreen.h"
#include "game/ui/tileview/battleview.h"
#include "game/ui/tileview/cityview.h"
#include <cmath>

namespace OpenApoc
{

void BootUp::begin() {}

void BootUp::pause() {}

void BootUp::resume() {}

void BootUp::finish() {}

void BootUp::eventOccurred(Event *e) { std::ignore = e; }

void BootUp::update()
{
	bool skipIntro = Options::skipIntroOption.get();
	// The first forms instance causes it to get loaded
	sp<GameState> loadedState;
	std::shared_future<void> loadTask;
	bool loadGame = false;

	fw().setupModDataPaths();
	fw().jukebox->loadPlaylists();

	if (Options::loadGameOption.get().empty())
	{
		loadTask = fw().threadPoolEnqueue([]() {
			auto &ui_instance = ui();
			std::ignore = ui_instance;
		});
	}
	else
	{
		loadGame = true;
		auto path = Options::loadGameOption.get();
		loadedState = mksp<GameState>();
		loadTask = fw().threadPoolEnqueue([loadedState, path]() {
			auto &ui_instance = ui();
			std::ignore = ui_instance;
			LogWarning("Loading save \"%s\"", path);

			if (!loadedState->loadGame(path))
			{
				LogError("Failed to load supplied game \"%s\"", path);
			}
			loadedState->initState();
		});
	}

	sp<Stage> nextScreen;
	if (loadGame == true)
	{
		nextScreen =
		    mksp<LoadingScreen>(nullptr, std::move(loadTask), [loadedState]() -> sp<Stage> {
			    if (loadedState->current_battle)
			    {
				    return mksp<BattleView>(loadedState);
			    }
			    else
			    {
				    return mksp<CityView>(loadedState);
			    }
		    });
	}
	else
	{
		nextScreen = mksp<LoadingScreen>(nullptr, std::move(loadTask),
		                                 []() -> sp<Stage> { return mksp<MainMenu>(); });
	}

	fw().stageQueueCommand(
	    {StageCmd::Command::REPLACE,
	     mksp<VideoScreen>(skipIntro ? "" : "SMK:xcom3/smk/intro1.smk", nextScreen)});
}

void BootUp::render() {}

bool BootUp::isTransition() { return false; }
} // namespace OpenApoc

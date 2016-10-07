#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include "game/ui/boot.h"
#include "forms/ui.h"
#include "framework/configfile.h"
#include "framework/framework.h"
#include "game/ui/general/mainmenu.h"
#include "game/ui/general/videoscreen.h"
#include <cmath>

namespace OpenApoc
{
ConfigOptionBool skipIntroOption("Game", "SkipIntro", "Skip intro video", false);

void BootUp::begin() {}

void BootUp::pause() {}

void BootUp::resume() {}

void BootUp::finish() {}

void BootUp::eventOccurred(Event *e) { std::ignore = e; }

void BootUp::update()
{
	bool skipIntro = skipIntroOption.get();
	// The first forms instance causes it to get loaded
	auto loadTask = fw().threadPoolEnqueue([]() {
		auto &ui_instance = ui();
		std::ignore = ui_instance;
	});

	fw().stageQueueCommand(
	    {StageCmd::Command::REPLACE,
	     mksp<VideoScreen>(skipIntro ? "" : "SMK:xcom3/smk/intro1.smk", std::move(loadTask),
	                       []() -> sp<Stage> { return mksp<MainMenu>(); })});
}

void BootUp::render() {}

bool BootUp::isTransition() { return false; }

}; // namespace OpenApoc

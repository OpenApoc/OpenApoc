#define _USE_MATH_DEFINES
#include "game/ui/boot.h"
#include "forms/ui.h"
#include "framework/framework.h"
#include "game/ui/general/mainmenu.h"
#include "game/ui/general/videoscreen.h"
#include <cmath>

namespace OpenApoc
{

void BootUp::begin() {}

void BootUp::pause() {}

void BootUp::resume() {}

void BootUp::finish() {}

void BootUp::eventOccurred(Event *e) { std::ignore = e; }

void BootUp::update(StageCmd *const cmd)
{
	// The first forms instance causes it to get loaded
	auto loadTask = fw().threadPool->enqueue([]() {
		auto &ui_instance = ui();
		std::ignore = ui_instance;
	});

	stageCmd.cmd = StageCmd::Command::REPLACE;
	stageCmd.nextStage = mksp<VideoScreen>("SMK:xcom3/smk/intro1.smk", std::move(loadTask),
	                                       []() -> sp<Stage> { return mksp<MainMenu>(); });
	*cmd = stageCmd;
	return;
}

void BootUp::render() {}

bool BootUp::isTransition() { return false; }

}; // namespace OpenApoc

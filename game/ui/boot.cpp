#define _USE_MATH_DEFINES
#include "game/ui/boot.h"
#include "forms/ui.h"
#include "framework/framework.h"
#include "game/ui/general/mainmenu.h"
#include "game/ui/general/videoscreen.h"
#include <cmath>

namespace OpenApoc
{

void BootUp::Begin() {}

void BootUp::Pause() {}

void BootUp::Resume() {}

void BootUp::Finish() {}

void BootUp::EventOccurred(Event *e) { std::ignore = e; }

void BootUp::Update(StageCmd *const cmd)
{
	// The first forms instance causes it to get loaded
	auto loadTask = fw().threadPool->enqueue([]() { auto &ui_instance = ui(); });

	stageCmd.cmd = StageCmd::Command::REPLACE;
	stageCmd.nextStage = mksp<VideoScreen>("SMK:xcom3/smk/intro1.smk", std::move(loadTask),
	                                       []() -> sp<Stage> { return mksp<MainMenu>(); });
	*cmd = stageCmd;
	return;
}

void BootUp::Render() {}

bool BootUp::IsTransition() { return false; }

}; // namespace OpenApoc

#include "game/ui/general/difficultymenu.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "game/state/city/city.h"
#include "game/state/gamestate.h"
#include "game/ui/city/cityview.h"
#include "game/ui/general/loadingscreen.h"

namespace OpenApoc
{

DifficultyMenu::DifficultyMenu() : Stage(), difficultymenuform(ui().GetForm("FORM_DIFFICULTYMENU"))
{
	assert(difficultymenuform);
}

DifficultyMenu::~DifficultyMenu() {}

void DifficultyMenu::Begin() {}

void DifficultyMenu::Pause() {}

void DifficultyMenu::Resume() {}

void DifficultyMenu::Finish() {}

std::future<sp<GameState>> loadGame(const UString &path)
{
	auto loadTask = fw().threadPool->enqueue([path]() -> sp<GameState> {

		auto state = mksp<GameState>();
		if (!state->loadGame(path))
		{
			LogError("Failed to load '%s'", path.c_str());
			return nullptr;
		}
		state->startGame();
		state->initState();
		return state;
	});

	return loadTask;
}

void DifficultyMenu::EventOccurred(Event *e)
{
	difficultymenuform->EventOccured(e);

	if (e->Type() == EVENT_KEY_DOWN)
	{
		if (e->Keyboard().KeyCode == SDLK_ESCAPE)
		{
			stageCmd.cmd = StageCmd::Command::POP;
			return;
		}
	}

	if (e->Type() == EVENT_FORM_INTERACTION && e->Forms().EventFlag == FormEventType::ButtonClick)
	{
		UString initialStatePath;
		if (e->Forms().RaisedBy->Name.compare("BUTTON_DIFFICULTY1") == 0)
		{
			initialStatePath = "data/difficulty1_patched";
		}
		else if (e->Forms().RaisedBy->Name.compare("BUTTON_DIFFICULTY2") == 0)
		{
			initialStatePath = "data/difficulty2_patched";
		}
		else if (e->Forms().RaisedBy->Name.compare("BUTTON_DIFFICULTY3") == 0)
		{
			initialStatePath = "data/difficulty3_patched";
		}
		else if (e->Forms().RaisedBy->Name.compare("BUTTON_DIFFICULTY4") == 0)
		{
			initialStatePath = "data/difficulty4_patched";
		}
		else if (e->Forms().RaisedBy->Name.compare("BUTTON_DIFFICULTY5") == 0)
		{
			initialStatePath = "data/difficulty5_patched";
		}
		else
		{
			LogWarning("Unknown button pressed: %s", e->Forms().RaisedBy->Name.c_str());
			return;
		}

		stageCmd.cmd = StageCmd::Command::PUSH;
		stageCmd.nextStage = mksp<LoadingScreen>(loadGame(initialStatePath));
		return;
	}
}

void DifficultyMenu::Update(StageCmd *const cmd)
{
	difficultymenuform->Update();
	*cmd = stageCmd;
	stageCmd = StageCmd();
}

void DifficultyMenu::Render() { difficultymenuform->Render(); }

bool DifficultyMenu::IsTransition() { return false; }
}; // namespace OpenApoc

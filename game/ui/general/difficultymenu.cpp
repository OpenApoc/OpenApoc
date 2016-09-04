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

DifficultyMenu::DifficultyMenu() : Stage(), difficultymenuform(ui().getForm("FORM_DIFFICULTYMENU"))
{
	LogAssert(difficultymenuform);
}

DifficultyMenu::~DifficultyMenu() = default;

void DifficultyMenu::begin() {}

void DifficultyMenu::pause() {}

void DifficultyMenu::resume() {}

void DifficultyMenu::finish() {}

std::future<sp<GameState>> loadGame(const UString &path)
{
	auto loadTask = fw().threadPool->enqueue([path]() -> sp<GameState> {

		auto state = mksp<GameState>();
		if (!state->loadGame(path))
		{
			LogError("Failed to load '%s'", path.cStr());
			return nullptr;
		}
		state->startGame();
		state->initState();
		return state;
	});

	return loadTask;
}

void DifficultyMenu::eventOccurred(Event *e)
{
	difficultymenuform->eventOccured(e);

	if (e->type() == EVENT_KEY_DOWN)
	{
		if (e->keyboard().KeyCode == SDLK_ESCAPE)
		{
			fw().stageQueueCommand({StageCmd::Command::POP});
			return;
		}
	}

	if (e->type() == EVENT_FORM_INTERACTION && e->forms().EventFlag == FormEventType::ButtonClick)
	{
		UString initialStatePath;
		if (e->forms().RaisedBy->Name.compare("BUTTON_DIFFICULTY1") == 0)
		{
			initialStatePath = "data/difficulty1_patched";
		}
		else if (e->forms().RaisedBy->Name.compare("BUTTON_DIFFICULTY2") == 0)
		{
			initialStatePath = "data/difficulty2_patched";
		}
		else if (e->forms().RaisedBy->Name.compare("BUTTON_DIFFICULTY3") == 0)
		{
			initialStatePath = "data/difficulty3_patched";
		}
		else if (e->forms().RaisedBy->Name.compare("BUTTON_DIFFICULTY4") == 0)
		{
			initialStatePath = "data/difficulty4_patched";
		}
		else if (e->forms().RaisedBy->Name.compare("BUTTON_DIFFICULTY5") == 0)
		{
			initialStatePath = "data/difficulty5_patched";
		}
		else
		{
			LogWarning("Unknown button pressed: %s", e->forms().RaisedBy->Name.cStr());
			return;
		}

		fw().stageQueueCommand(
		    {StageCmd::Command::PUSH, mksp<LoadingScreen>(loadGame(initialStatePath))});
		return;
	}
}

void DifficultyMenu::update() { difficultymenuform->update(); }

void DifficultyMenu::render() { difficultymenuform->render(); }

bool DifficultyMenu::isTransition() { return false; }
}; // namespace OpenApoc

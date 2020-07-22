#include "game/ui/general/difficultymenu.h"
#include "forms/form.h"
#include "forms/ui.h"
#include "framework/configfile.h"
#include "framework/data.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/keycodes.h"
#include "framework/modinfo.h"
#include "framework/options.h"
#include "game/state/city/city.h"
#include "game/state/gamestate.h"
#include "game/ui/general/loadingscreen.h"
#include "game/ui/tileview/cityview.h"

namespace OpenApoc
{

DifficultyMenu::DifficultyMenu() : Stage(), difficultymenuform(ui().getForm("difficultymenu"))
{
	LogAssert(difficultymenuform);
}

DifficultyMenu::~DifficultyMenu() = default;

void DifficultyMenu::begin() {}

void DifficultyMenu::pause() {}

void DifficultyMenu::resume() {}

void DifficultyMenu::finish() {}

std::shared_future<void> loadGame(sp<GameState> state)
{
	auto loadTask = fw().threadPoolEnqueue([state]() -> void {
		state->loadMods();
		state->startGame();
		state->initState();
		state->fillPlayerStartingProperty();
		state->fillOrgStartingProperty();
		return;
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
		int difficulty;
		if (e->forms().RaisedBy->Name.compare("BUTTON_DIFFICULTY1") == 0)
		{
			difficulty = 0;
		}
		else if (e->forms().RaisedBy->Name.compare("BUTTON_DIFFICULTY2") == 0)
		{
			difficulty = 1;
		}
		else if (e->forms().RaisedBy->Name.compare("BUTTON_DIFFICULTY3") == 0)
		{
			difficulty = 2;
		}
		else if (e->forms().RaisedBy->Name.compare("BUTTON_DIFFICULTY4") == 0)
		{
			difficulty = 3;
		}
		else if (e->forms().RaisedBy->Name.compare("BUTTON_DIFFICULTY5") == 0)
		{
			difficulty = 4;
		}
		else
		{
			LogWarning("Unknown button pressed: %s", e->forms().RaisedBy->Name);
			return;
		}

		auto loadedState = mksp<GameState>();
		loadedState->difficulty = difficulty;

		fw().stageQueueCommand(
		    {StageCmd::Command::PUSH,
		     mksp<LoadingScreen>(nullptr, loadGame(loadedState),
		                         [loadedState]() { return mksp<CityView>(loadedState); })});
		return;
	}
}

void DifficultyMenu::update() { difficultymenuform->update(); }

void DifficultyMenu::render() { difficultymenuform->render(); }

bool DifficultyMenu::isTransition() { return false; }
}; // namespace OpenApoc

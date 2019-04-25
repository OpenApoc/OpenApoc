#include "game/ui/general/difficultymenu.h"
#include "forms/form.h"
#include "forms/ui.h"
#include "framework/configfile.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/keycodes.h"
#include "game/state/city/city.h"
#include "game/state/gamestate.h"
#include "game/ui/general/loadingscreen.h"
#include "game/ui/tileview/cityview.h"

namespace
{
OpenApoc::ConfigOptionString
    modList("Game", "Mods", "A colon-separated list of mods to load (relative to data directory)",
            "");

} // anonymous namespace

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

std::shared_future<void> loadGame(const UString &path, sp<GameState> state)
{
	auto loadTask = fw().threadPoolEnqueue([path, state]() -> void {
		if (!state->loadGame(fw().getDataDir() + "/gamestate_common"))
		{
			LogError("Failed to load common gamestate");
			return;
		}
		if (!state->loadGame(fw().getDataDir() + "/" + path))
		{
			LogError("Failed to load '%s'", path);
			return;
		}
		auto mods = modList.get().split(":");
		for (const auto &modString : mods)
		{
			LogWarning("loading mod \"%s\"", modString);
			if (!state->loadGame(fw().getDataDir() + "/" + modString))
			{
				LogError("Failed to load mod \"%s\"", modString);
			}
		}

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
		UString initialStatePath;
		if (e->forms().RaisedBy->Name.compare("BUTTON_DIFFICULTY1") == 0)
		{
			initialStatePath = "difficulty1_patched";
		}
		else if (e->forms().RaisedBy->Name.compare("BUTTON_DIFFICULTY2") == 0)
		{
			initialStatePath = "difficulty2_patched";
		}
		else if (e->forms().RaisedBy->Name.compare("BUTTON_DIFFICULTY3") == 0)
		{
			initialStatePath = "difficulty3_patched";
		}
		else if (e->forms().RaisedBy->Name.compare("BUTTON_DIFFICULTY4") == 0)
		{
			initialStatePath = "difficulty4_patched";
		}
		else if (e->forms().RaisedBy->Name.compare("BUTTON_DIFFICULTY5") == 0)
		{
			initialStatePath = "difficulty5_patched";
		}
		else
		{
			LogWarning("Unknown button pressed: %s", e->forms().RaisedBy->Name);
			return;
		}

		auto loadedState = mksp<GameState>();

		fw().stageQueueCommand(
		    {StageCmd::Command::PUSH,
		     mksp<LoadingScreen>(nullptr, loadGame(initialStatePath, loadedState),
		                         [loadedState]() { return mksp<CityView>(loadedState); })});
		return;
	}
}

void DifficultyMenu::update() { difficultymenuform->update(); }

void DifficultyMenu::render() { difficultymenuform->render(); }

bool DifficultyMenu::isTransition() { return false; }
}; // namespace OpenApoc

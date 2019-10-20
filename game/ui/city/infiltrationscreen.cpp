#include "game/ui/city/infiltrationscreen.h"
#include "forms/form.h"
#include "forms/label.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/keycodes.h"
#include "game/state/gamestate.h"

namespace OpenApoc
{

InfiltrationScreen::InfiltrationScreen(sp<GameState> state)
    : Stage(), menuform(ui().getForm("city/infiltration")), state(state)
{
}

InfiltrationScreen::~InfiltrationScreen() = default;

void InfiltrationScreen::begin()
{
	menuform->findControlTyped<Label>("TEXT_FUNDS")->setText(state->getPlayerBalance());
}

void InfiltrationScreen::pause() {}

void InfiltrationScreen::resume() {}

void InfiltrationScreen::finish() {}

void InfiltrationScreen::eventOccurred(Event *e)
{
	menuform->eventOccured(e);

	if (e->type() == EVENT_KEY_DOWN)
	{
		if (e->keyboard().KeyCode == SDLK_ESCAPE)
		{
			menuform->findControl("BUTTON_QUIT")->click();
			return;
		}
	}

	if (e->type() == EVENT_FORM_INTERACTION && e->forms().EventFlag == FormEventType::ButtonClick)
	{
		if (e->forms().RaisedBy->Name == "BUTTON_QUIT")
		{
			fw().stageQueueCommand({StageCmd::Command::POP});
			return;
		}
	}
}

void InfiltrationScreen::update() { menuform->update(); }

void InfiltrationScreen::render()
{
	fw().stageGetPrevious(this->shared_from_this())->render();
	menuform->render();
}

bool InfiltrationScreen::isTransition() { return false; }

}; // namespace OpenApoc

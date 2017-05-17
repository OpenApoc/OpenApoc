#include "game/ui/city/alertscreen.h"
#include "forms/form.h"
#include "forms/label.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/keycodes.h"
#include "game/state/city/building.h"
#include "game/state/gamestate.h"
#include "game/state/organisation.h"
#include "library/strings_format.h"

namespace OpenApoc
{

AlertScreen::AlertScreen(sp<GameState> state, sp<Building> building)
    : Stage(), menuform(ui().getForm("city/alert")), state(state), building(building)
{
	menuform->findControlTyped<Label>("TEXT_FUNDS")->setText(state->getPlayerBalance());
	menuform->findControlTyped<Label>("TEXT_OWNER_NAME")->setText(tr(building->owner->name));
	menuform->findControlTyped<Label>("TEXT_BUILDING_FUNCTION")
	    ->setText(tr(building->function->name));
}

AlertScreen::~AlertScreen() = default;

void AlertScreen::begin() {}

void AlertScreen::pause() {}

void AlertScreen::resume() {}

void AlertScreen::finish() {}

void AlertScreen::eventOccurred(Event *e)
{
	menuform->eventOccured(e);

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
		if (e->forms().RaisedBy->Name == "BUTTON_QUIT")
		{
			fw().stageQueueCommand({StageCmd::Command::POP});
			return;
		}
	}
}

void AlertScreen::update() { menuform->update(); }

void AlertScreen::render()
{
	fw().stageGetPrevious(this->shared_from_this())->render();
	menuform->render();
}

bool AlertScreen::isTransition() { return false; }

}; // namespace OpenApoc

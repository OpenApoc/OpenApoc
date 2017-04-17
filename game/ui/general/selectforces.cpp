#include "game/ui/general/selectforces.h"
#include "game/ui/general/skirmish.h"
#include "forms/form.h"
#include "forms/checkbox.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/keycodes.h"
#include "game/state/gamestate.h"
#include "forms/scrollbar.h"
#include "forms/label.h"
namespace OpenApoc
{
	
SelectForces::SelectForces(sp<GameState> state, Skirmish &skirmish)
: Stage(),skirmish(skirmish), state(*state), menuform(ui().getForm("selectforces"))
{
	menuform->findControlTyped<Label>("TEXT_FUNDS")->setText(state->getPlayerBalance());
	menuform->findControlTyped<Label>("LOCATION")->setText(skirmish.getLocationText());


}

SelectForces::~SelectForces() = default;

void SelectForces::begin(){}

void SelectForces::pause(){}

void SelectForces::resume(){}

void SelectForces::finish(){}

void SelectForces::eventOccurred(Event * e)
{
	menuform->eventOccured(e);

	if (e->type() == EVENT_KEY_DOWN)
	{
		if (e->keyboard().KeyCode == SDLK_ESCAPE)
		{
			fw().stageQueueCommand({ StageCmd::Command::POP });
			return;
		}
	}

	if (e->type() == EVENT_FORM_INTERACTION && e->forms().EventFlag == FormEventType::ButtonClick)
	{
		if (e->forms().RaisedBy->Name == "BUTTON_OK")
		{
			skirmish.goToBattle();
			fw().stageQueueCommand({ StageCmd::Command::POP });
			return;
		}
	}

}

void SelectForces::update()
{
	menuform->update();
}

void SelectForces::render()
{
	fw().stageGetPrevious(this->shared_from_this())->render();
	menuform->render();
}

bool SelectForces::isTransition()
{
	return false;
}

}
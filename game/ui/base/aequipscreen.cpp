#include "game/ui/base/aequipscreen.h"
#include "forms/forms.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "game/state/gamestate.h"

namespace OpenApoc
{
AEquipScreen::AEquipScreen(sp<GameState> state)
    : Stage(), form(ui().getForm("FORM_AEQUIPSCREEN")),
      pal(fw().data->loadPalette("xcom3/ufodata/agenteqp.pcx")), state(state)

{
}

AEquipScreen::~AEquipScreen() = default;

void AEquipScreen::begin()
{
	form->findControlTyped<Label>("TEXT_FUNDS")->setText(state->getPlayerBalance());
}

void AEquipScreen::pause() {}

void AEquipScreen::resume() {}

void AEquipScreen::finish() {}

void AEquipScreen::eventOccurred(Event *e)
{
	form->eventOccured(e);

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

		if (e->forms().RaisedBy->Name == "BUTTON_OK")
		{
			fw().stageQueueCommand({StageCmd::Command::POP});
			return;
		}
	}
}

void AEquipScreen::update() { form->update(); }

void AEquipScreen::render()
{
	fw().stageGetPrevious(this->shared_from_this())->render();
	fw().renderer->setPalette(this->pal);
	form->render();
}

bool AEquipScreen::isTransition() { return false; }

} // namespace OpenApoc

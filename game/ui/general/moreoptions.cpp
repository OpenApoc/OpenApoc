#include "forms/checkbox.h"
#include "forms/form.h"
#include "forms/label.h"
#include "forms/listbox.h"
#include "forms/scrollbar.h"
#include "forms/textbutton.h"
#include "forms/ui.h"
#include "framework/configfile.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/keycodes.h"
#include "game/state/gamestate.h"
#include "game/state/shared/organisation.h"
#include "game/ui/general/moreoptions.h"
#include <limits>

namespace OpenApoc
{

MoreOptions::MoreOptions(sp<GameState> state)
    : Stage(), menuform(ui().getForm("moreoptions")), state(state)
{
}
MoreOptions::~MoreOptions() {}

bool MoreOptions::isTransition() { return false; }

void MoreOptions::begin() {}

void MoreOptions::pause() {}

void MoreOptions::resume() {}

void MoreOptions::finish() {}

void MoreOptions::eventOccurred(Event *e)
{
	menuform->eventOccured(e);

	if (e->type() == EVENT_KEY_DOWN)
	{
		if (e->keyboard().KeyCode == SDLK_ESCAPE || e->keyboard().KeyCode == SDLK_RETURN ||
		    e->keyboard().KeyCode == SDLK_KP_ENTER)
		{
			menuform->findControl("BUTTON_OK")->click();
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

void MoreOptions::update() { menuform->update(); }

void MoreOptions::render()
{
	fw().stageGetPrevious(this->shared_from_this())->render();
	menuform->render();
}
} // namespace OpenApoc

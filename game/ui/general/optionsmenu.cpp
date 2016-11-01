#include "game/ui/general/optionsmenu.h"
#include "forms/form.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/keycodes.h"
#include "game/ui/debugtools/debugmenu.h"

namespace OpenApoc
{

OptionsMenu::OptionsMenu() : Stage(), menuform(ui().getForm("options")) {}

OptionsMenu::~OptionsMenu() = default;

void OptionsMenu::begin() {}

void OptionsMenu::pause() {}

void OptionsMenu::resume() {}

void OptionsMenu::finish() {}

void OptionsMenu::eventOccurred(Event *e)
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
		if (e->forms().RaisedBy->Name == "BUTTON_TEST_XCOMBASE")
		{
			return;
		}
		if (e->forms().RaisedBy->Name == "BUTTON_DEBUGGING")
		{
			fw().stageQueueCommand({StageCmd::Command::PUSH, mksp<DebugMenu>()});
			return;
		}
		if (e->forms().RaisedBy->Name == "BUTTON_QUIT")
		{
			fw().stageQueueCommand({StageCmd::Command::POP});
			return;
		}
	}
}

void OptionsMenu::update() { menuform->update(); }

void OptionsMenu::render()
{
	fw().stageGetPrevious(this->shared_from_this())->render();
	menuform->render();
}

bool OptionsMenu::isTransition() { return false; }

}; // namespace OpenApoc

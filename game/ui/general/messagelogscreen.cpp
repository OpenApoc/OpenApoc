#include "game/ui/general/messagelogscreen.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "game/state/gamestate.h"

namespace OpenApoc
{

MessageLogScreen::MessageLogScreen(sp<GameState> state)
    : Stage(), menuform(ui().getForm("FORM_MESSAGE_LOG_SCREEN")), state(state)
{
}

MessageLogScreen::~MessageLogScreen() = default;

void MessageLogScreen::begin()
{
	menuform->findControlTyped<Label>("TEXT_FUNDS")->setText(state->getPlayerBalance());
}

void MessageLogScreen::pause() {}

void MessageLogScreen::resume() {}

void MessageLogScreen::finish() {}

void MessageLogScreen::eventOccurred(Event *e)
{
	menuform->eventOccured(e);

	if (e->type() == EVENT_KEY_DOWN)
	{
		if (e->keyboard().KeyCode == SDLK_ESCAPE)
		{
			stageCmd.cmd = StageCmd::Command::POP;
			return;
		}
	}

	if (e->type() == EVENT_FORM_INTERACTION && e->forms().EventFlag == FormEventType::ButtonClick)
	{
		if (e->forms().RaisedBy->Name == "BUTTON_OK")
		{
			stageCmd.cmd = StageCmd::Command::POP;
			return;
		}
	}
}

void MessageLogScreen::update(StageCmd *const cmd)
{
	menuform->update();
	*cmd = this->stageCmd;
	// Reset the command to default
	this->stageCmd = StageCmd();
}

void MessageLogScreen::render()
{
	fw().stageGetPrevious(this->shared_from_this())->render();
	fw().renderer->drawFilledRect({0, 0}, fw().displayGetSize(), Colour{0, 0, 0, 128});
	menuform->render();
}

bool MessageLogScreen::isTransition() { return false; }

}; // namespace OpenApoc

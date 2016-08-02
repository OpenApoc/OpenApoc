#include "game/ui/general/messagelogscreen.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "game/state/gamestate.h"

namespace OpenApoc
{

MessageLogScreen::MessageLogScreen(sp<GameState> state)
    : Stage(), menuform(ui().GetForm("FORM_MESSAGE_LOG_SCREEN")), state(state)
{
}

MessageLogScreen::~MessageLogScreen() {}

void MessageLogScreen::Begin()
{
	menuform->FindControlTyped<Label>("TEXT_FUNDS")->SetText(state->getPlayerBalance());
}

void MessageLogScreen::Pause() {}

void MessageLogScreen::Resume() {}

void MessageLogScreen::Finish() {}

void MessageLogScreen::EventOccurred(Event *e)
{
	menuform->EventOccured(e);

	if (e->Type() == EVENT_KEY_DOWN)
	{
		if (e->Keyboard().KeyCode == SDLK_ESCAPE)
		{
			stageCmd.cmd = StageCmd::Command::POP;
			return;
		}
	}

	if (e->Type() == EVENT_FORM_INTERACTION && e->Forms().EventFlag == FormEventType::ButtonClick)
	{
		if (e->Forms().RaisedBy->Name == "BUTTON_OK")
		{
			stageCmd.cmd = StageCmd::Command::POP;
			return;
		}
	}
}

void MessageLogScreen::Update(StageCmd *const cmd)
{
	menuform->Update();
	*cmd = this->stageCmd;
	// Reset the command to default
	this->stageCmd = StageCmd();
}

void MessageLogScreen::Render()
{
	fw().Stage_GetPrevious(this->shared_from_this())->Render();
	fw().renderer->drawFilledRect({0, 0}, fw().Display_GetSize(), Colour{0, 0, 0, 128});
	menuform->Render();
}

bool MessageLogScreen::IsTransition() { return false; }

}; // namespace OpenApoc

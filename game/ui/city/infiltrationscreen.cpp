#include "game/ui/city/infiltrationscreen.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"

namespace OpenApoc
{

InfiltrationScreen::InfiltrationScreen()
    : Stage(), menuform(ui().GetForm("FORM_INFILTRATION_SCREEN"))
{
}

InfiltrationScreen::~InfiltrationScreen() {}

void InfiltrationScreen::Begin() {}

void InfiltrationScreen::Pause() {}

void InfiltrationScreen::Resume() {}

void InfiltrationScreen::Finish() {}

void InfiltrationScreen::EventOccurred(Event *e)
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
		if (e->Forms().RaisedBy->Name == "BUTTON_QUIT")
		{
			stageCmd.cmd = StageCmd::Command::POP;
			return;
		}
	}
}

void InfiltrationScreen::Update(StageCmd *const cmd)
{
	menuform->Update();
	*cmd = this->stageCmd;
	// Reset the command to default
	this->stageCmd = StageCmd();
}

void InfiltrationScreen::Render()
{
	fw().Stage_GetPrevious(this->shared_from_this())->Render();
	fw().renderer->drawFilledRect({0, 0}, fw().Display_GetSize(), Colour{0, 0, 0, 128});
	menuform->Render();
}

bool InfiltrationScreen::IsTransition() { return false; }

}; // namespace OpenApoc

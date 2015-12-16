
#include "game/city/infiltrationscreen.h"
#include "framework/framework.h"

namespace OpenApoc
{

InfiltrationScreen::InfiltrationScreen(Framework &fw)
    : Stage(fw), menuform(fw.gamecore->GetForm("FORM_INFILTRATION_SCREEN"))
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
	fw.gamecore->MouseCursor->EventOccured(e);

	if (e->Type == EVENT_KEY_DOWN)
	{
		if (e->Data.Keyboard.KeyCode == SDLK_ESCAPE)
		{
			stageCmd.cmd = StageCmd::Command::POP;
			return;
		}
	}

	if (e->Type == EVENT_FORM_INTERACTION && e->Data.Forms.EventFlag == FormEventType::ButtonClick)
	{
		if (e->Data.Forms.RaisedBy->Name == "BUTTON_QUIT")
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
	fw.Stage_GetPrevious(this->shared_from_this())->Render();
	fw.renderer->drawFilledRect({0, 0}, fw.Display_GetSize(), Colour{0, 0, 0, 128});
	menuform->Render();
	fw.gamecore->MouseCursor->Render();
}

bool InfiltrationScreen::IsTransition() { return false; }

}; // namespace OpenApoc

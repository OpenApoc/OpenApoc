#include "game/ui/city/baseselectscreen.h"
#include "forms/ui.h"
#include "framework/framework.h"

namespace OpenApoc
{

BaseSelectScreen::BaseSelectScreen() : Stage(), menuform(ui().GetForm("FORM_SELECT_BASE_SCREEN")) {}

BaseSelectScreen::~BaseSelectScreen() {}

void BaseSelectScreen::Begin() {}

void BaseSelectScreen::Pause() {}

void BaseSelectScreen::Resume() {}

void BaseSelectScreen::Finish() {}

void BaseSelectScreen::EventOccurred(Event *e)
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

void BaseSelectScreen::Update(StageCmd *const cmd)
{
	menuform->Update();
	*cmd = this->stageCmd;
	// Reset the command to default
	this->stageCmd = StageCmd();
}

void BaseSelectScreen::Render()
{
	fw().Stage_GetPrevious(this->shared_from_this())->Render();
	fw().renderer->drawFilledRect({0, 0}, fw().Display_GetSize(), Colour{0, 0, 0, 128});
	menuform->Render();
}

bool BaseSelectScreen::IsTransition() { return false; }

}; // namespace OpenApoc


#include "game/base/basescreen.h"
#include "framework/framework.h"

namespace OpenApoc
{

BaseScreen::BaseScreen(Framework &fw)
    : Stage(fw), basescreenform(fw.gamecore->GetForm("FORM_BASESCREEN"))
{
}

BaseScreen::~BaseScreen() {}

void BaseScreen::Begin() {}

void BaseScreen::Pause() {}

void BaseScreen::Resume() {}

void BaseScreen::Finish() {}

void BaseScreen::EventOccurred(Event *e)
{
	basescreenform->EventOccured(e);
	fw.gamecore->MouseCursor->EventOccured(e);

	if (e->Type == EVENT_KEY_DOWN)
	{
		if (e->Data.Keyboard.KeyCode == ALLEGRO_KEY_ESCAPE)
		{
			stageCmd.cmd = StageCmd::Command::POP;
			return;
		}
	}

	if (e->Type == EVENT_FORM_INTERACTION && e->Data.Forms.EventFlag == FormEventType::ButtonClick)
	{
		if (e->Data.Forms.RaisedBy->Name == "BUTTON_OK")
		{
			stageCmd.cmd = StageCmd::Command::POP;
			return;
		}
	}
}

void BaseScreen::Update(StageCmd *const cmd)
{
	basescreenform->Update();
	*cmd = stageCmd;
	stageCmd = StageCmd();
}

void BaseScreen::Render()
{
	fw.Stage_GetPrevious(this->shared_from_this())->Render();
	fw.renderer->drawFilledRect({0, 0}, fw.Display_GetSize(), Colour{0, 0, 0, 128});
	basescreenform->Render();
	fw.gamecore->MouseCursor->Render();
}

bool BaseScreen::IsTransition() { return false; }

}; // namespace OpenApoc

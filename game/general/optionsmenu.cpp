
#include "game/general/optionsmenu.h"
#include "game/ufopaedia/ufopaedia.h"
#include "game/debugtools/debugmenu.h"
#include "framework/framework.h"

namespace OpenApoc
{

OptionsMenu::OptionsMenu(Framework &fw)
    : Stage(fw), menuform(fw.gamecore->GetForm("FORM_OPTIONSMENU"))
{
}

OptionsMenu::~OptionsMenu() {}

void OptionsMenu::Begin() {}

void OptionsMenu::Pause() {}

void OptionsMenu::Resume() {}

void OptionsMenu::Finish() {}

void OptionsMenu::EventOccurred(Event *e)
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
		if (e->Data.Forms.RaisedBy->Name == "BUTTON_TEST_XCOMBASE")
		{
			return;
		}
		if (e->Data.Forms.RaisedBy->Name == "BUTTON_TEST_UFOPAEDIA")
		{
			stageCmd.cmd = StageCmd::Command::PUSH;
			stageCmd.nextStage = std::make_shared<Ufopaedia>(fw);
			return;
		}
		if (e->Data.Forms.RaisedBy->Name == "BUTTON_DEBUGGING")
		{
			stageCmd.cmd = StageCmd::Command::PUSH;
			stageCmd.nextStage = std::make_shared<DebugMenu>(fw);
			return;
		}
		if (e->Data.Forms.RaisedBy->Name == "BUTTON_QUIT")
		{
			stageCmd.cmd = StageCmd::Command::POP;
			return;
		}
	}
}

void OptionsMenu::Update(StageCmd *const cmd)
{
	menuform->Update();
	*cmd = this->stageCmd;
	// Reset the command to default
	this->stageCmd = StageCmd();
}

void OptionsMenu::Render()
{
	fw.Stage_GetPrevious(this->shared_from_this())->Render();
	fw.renderer->drawFilledRect({0, 0}, fw.Display_GetSize(), Colour{0, 0, 0, 128});
	menuform->Render();
	fw.gamecore->MouseCursor->Render();
}

bool OptionsMenu::IsTransition() { return false; }

}; // namespace OpenApoc

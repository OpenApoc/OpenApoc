#include "game/general/optionsmenu.h"
#include "framework/framework.h"
#include "game/debugtools/debugmenu.h"
#include "game/resources/gamecore.h"

namespace OpenApoc
{

OptionsMenu::OptionsMenu() : Stage(), menuform(fw().gamecore->GetForm("FORM_OPTIONSMENU")) {}

OptionsMenu::~OptionsMenu() {}

void OptionsMenu::Begin() {}

void OptionsMenu::Pause() {}

void OptionsMenu::Resume() {}

void OptionsMenu::Finish() {}

void OptionsMenu::EventOccurred(Event *e)
{
	menuform->EventOccured(e);
	fw().gamecore->MouseCursor->EventOccured(e);

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
		if (e->Forms().RaisedBy->Name == "BUTTON_TEST_XCOMBASE")
		{
			return;
		}
		if (e->Forms().RaisedBy->Name == "BUTTON_DEBUGGING")
		{
			stageCmd.cmd = StageCmd::Command::PUSH;
			stageCmd.nextStage = mksp<DebugMenu>();
			return;
		}
		if (e->Forms().RaisedBy->Name == "BUTTON_QUIT")
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
	fw().Stage_GetPrevious(this->shared_from_this())->Render();
	fw().renderer->drawFilledRect({0, 0}, fw().Display_GetSize(), Colour{0, 0, 0, 128});
	menuform->Render();
	fw().gamecore->MouseCursor->Render();
}

bool OptionsMenu::IsTransition() { return false; }

}; // namespace OpenApoc

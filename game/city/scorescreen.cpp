
#include "game/city/scorescreen.h"
#include "framework/framework.h"

namespace OpenApoc
{

ScoreScreen::ScoreScreen(Framework &fw)
    : Stage(fw), menuform(fw.gamecore->GetForm("FORM_SCORE_SCREEN"))
{
}

ScoreScreen::~ScoreScreen() {}

void ScoreScreen::Begin() {}

void ScoreScreen::Pause() {}

void ScoreScreen::Resume() {}

void ScoreScreen::Finish() {}

void ScoreScreen::EventOccurred(Event *e)
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

void ScoreScreen::Update(StageCmd *const cmd)
{
	menuform->Update();
	*cmd = this->stageCmd;
	// Reset the command to default
	this->stageCmd = StageCmd();
}

void ScoreScreen::Render()
{
	fw.Stage_GetPrevious(this->shared_from_this())->Render();
	fw.renderer->drawFilledRect({0, 0}, fw.Display_GetSize(), Colour{0, 0, 0, 128});
	menuform->Render();
	fw.gamecore->MouseCursor->Render();
}

bool ScoreScreen::IsTransition() { return false; }

}; // namespace OpenApoc

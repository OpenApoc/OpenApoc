#include "game/ui/city/scorescreen.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "game/state/gamestate.h"

namespace OpenApoc
{

ScoreScreen::ScoreScreen(sp<GameState> state)
    : Stage(), menuform(ui().GetForm("FORM_SCORE_SCREEN")), state(state)
{
}

ScoreScreen::~ScoreScreen() {}

void ScoreScreen::Begin()
{
	menuform->FindControlTyped<Label>("TEXT_FUNDS")->SetText(state->getPlayerBalance());
}

void ScoreScreen::Pause() {}

void ScoreScreen::Resume() {}

void ScoreScreen::Finish() {}

void ScoreScreen::EventOccurred(Event *e)
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

void ScoreScreen::Update(StageCmd *const cmd)
{
	menuform->Update();
	*cmd = this->stageCmd;
	// Reset the command to default
	this->stageCmd = StageCmd();
}

void ScoreScreen::Render()
{
	fw().Stage_GetPrevious(this->shared_from_this())->Render();
	fw().renderer->drawFilledRect({0, 0}, fw().Display_GetSize(), Colour{0, 0, 0, 128});
	menuform->Render();
}

bool ScoreScreen::IsTransition() { return false; }

}; // namespace OpenApoc

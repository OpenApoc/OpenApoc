#include "game/ui/base/researchselect.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "game/state/base/facility.h"
#include "game/state/gamestate.h"

namespace OpenApoc
{

ResearchSelect::ResearchSelect(sp<GameState> state, sp<Lab> lab)
    : Stage(), form(ui().GetForm("FORM_RESEARCHSELECT")), state(state), lab(lab)
{
}

ResearchSelect::~ResearchSelect() {}

void ResearchSelect::Begin()
{
	form->FindControlTyped<Label>("TEXT_FUNDS")->SetText(state->getPlayerBalance());
}

void ResearchSelect::Pause() {}

void ResearchSelect::Resume() {}

void ResearchSelect::Finish() {}

void ResearchSelect::EventOccurred(Event *e)
{
	form->EventOccured(e);

	if (e->Type() == EVENT_KEY_DOWN)
	{
		if (e->Keyboard().KeyCode == SDLK_ESCAPE)
		{
			stageCmd.cmd = StageCmd::Command::POP;
			return;
		}
	}

	if (e->Type() == EVENT_FORM_INTERACTION)
	{
		if (e->Forms().EventFlag == FormEventType::ButtonClick)
		{
			if (e->Forms().RaisedBy->Name == "BUTTON_OK")
			{
				stageCmd.cmd = StageCmd::Command::POP;
				return;
			}
		}
	}
}

void ResearchSelect::Update(StageCmd *const cmd)
{
	form->Update();
	*cmd = stageCmd;
	stageCmd = StageCmd();
}

void ResearchSelect::Render()
{
	fw().Stage_GetPrevious(this->shared_from_this())->Render();
	// fw().renderer->drawFilledRect({0, 0}, fw().Display_GetSize(), Colour{0, 0, 0, 128});
	form->Render();
}

bool ResearchSelect::IsTransition() { return false; }

}; // namespace OpenApoc

#include "game/base/researchscreen.h"
#include "framework/framework.h"
#include "game/resources/gamecore.h"

namespace OpenApoc
{

ResearchScreen::ResearchScreen(sp<GameState> state, StateRef<Base> base,
                               StateRef<Facility> selected_lab)
    : Stage(), form(fw().gamecore->GetForm("FORM_RESEARCHSCREEN")), base(base),
      selected_lab(selected_lab), state(state)
{
}

ResearchScreen::~ResearchScreen() {}

void ResearchScreen::Begin()
{
	form->FindControlTyped<Label>("TEXT_FUNDS")->SetText(state->getPlayerBalance());
}

void ResearchScreen::Pause() {}

void ResearchScreen::Resume() {}

void ResearchScreen::Finish() {}

void ResearchScreen::EventOccurred(Event *e)
{
	form->EventOccured(e);
	fw().gamecore->MouseCursor->EventOccured(e);

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

void ResearchScreen::Update(StageCmd *const cmd)
{
	form->Update();
	*cmd = stageCmd;
	stageCmd = StageCmd();
}

void ResearchScreen::Render()
{
	fw().Stage_GetPrevious(this->shared_from_this())->Render();
	// fw().renderer->drawFilledRect({0, 0}, fw().Display_GetSize(), Colour{0, 0, 0, 128});
	form->Render();
	fw().gamecore->MouseCursor->Render();
}

bool ResearchScreen::IsTransition() { return false; }

}; // namespace OpenApoc

#include "game/ufopaedia/ufopaediaview.h"
#include "framework/framework.h"
#include "game/resources/gamecore.h"
#include "game/ufopaedia/ufopaediacategoryview.h"
#include "library/sp.h"

namespace OpenApoc
{

UfopaediaView::UfopaediaView(GameState &state)
    : Stage(), menuform(fw().gamecore->GetForm("FORM_UFOPAEDIA_TITLE")), state(state)
{
}

UfopaediaView::~UfopaediaView() {}

void UfopaediaView::Begin() {}

void UfopaediaView::Pause() {}

void UfopaediaView::Resume() {}

void UfopaediaView::Finish() {}

void UfopaediaView::EventOccurred(Event *e)
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
		if (e->Forms().RaisedBy->Name == "BUTTON_QUIT")
		{
			stageCmd.cmd = StageCmd::Command::POP;
			return;
		}

		if (e->Forms().RaisedBy->Name.substr(0, 7) == "BUTTON_")
		{
			for (auto &cat : state.ufopaedia)
			{
				auto catName = cat.first;
				UString butName = "BUTTON_" + catName;
				if (butName == e->Forms().RaisedBy->Name)
				{
					stageCmd.cmd = StageCmd::Command::PUSH;
					stageCmd.nextStage = mksp<UfopaediaCategoryView>(state, cat.second);
					LogInfo("Clicked category \"%s\"", catName.c_str());
					return;
				}
			}
		}
	}
}

void UfopaediaView::Update(StageCmd *const cmd)
{
	menuform->Update();
	*cmd = this->stageCmd;
	// Reset the command to default
	this->stageCmd = StageCmd();
}

void UfopaediaView::Render()
{
	fw().Stage_GetPrevious(this->shared_from_this())->Render();
	fw().renderer->drawFilledRect({0, 0}, fw().Display_GetSize(), Colour{0, 0, 0, 128});
	menuform->Render();
	fw().gamecore->MouseCursor->Render();
}

bool UfopaediaView::IsTransition() { return false; }

}; // namespace OpenApoc

#include "game/ui/city/basebuyscreen.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "game/state/city/building.h"
#include "game/state/gamestate.h"
#include "game/ui/general/messagebox.h"

namespace OpenApoc
{

BaseBuyScreen::BaseBuyScreen(sp<GameState> state, sp<Building> building)
    : Stage(), menuform(ui().GetForm("FORM_BUY_BASE_SCREEN")), state(state), building(building)
{
	price = building->bounds.size().x * building->bounds.size().y * 2000;
}

BaseBuyScreen::~BaseBuyScreen() {}

void BaseBuyScreen::Begin()
{
	menuform->FindControlTyped<Label>("TEXT_FUNDS")->SetText(state->getPlayerBalance());

	auto text = menuform->FindControlTyped<Label>("TEXT_PRICE");
	text->SetText(UString::format(tr("This Building will cost $%d"), price));
}

void BaseBuyScreen::Pause() {}

void BaseBuyScreen::Resume() {}

void BaseBuyScreen::Finish() {}

void BaseBuyScreen::EventOccurred(Event *e)
{
	menuform->EventOccured(e);

	if (e->Type() == EVENT_KEY_DOWN)
	{
		if (e->Keyboard().KeyCode == SDLK_ESCAPE)
		{
			stageCmd.cmd = StageCmd::Command::POP;
		}
	}

	else if (e->Type() == EVENT_FORM_INTERACTION &&
	         e->Forms().EventFlag == FormEventType::ButtonClick)
	{
		if (e->Forms().RaisedBy->Name == "BUTTON_OK")
		{
			stageCmd.cmd = StageCmd::Command::POP;
		}
		else if (e->Forms().RaisedBy->Name == "BUTTON_BUY_BASE")
		{
			if (state->getPlayer()->balance >= price)
			{
				state->getPlayer()->balance -= price;

				auto base = mksp<Base>(*state, StateRef<Building>{state.get(), building});
				state->player_bases[Base::getPrefix() +
				                    Strings::FromInteger(state->player_bases.size() + 1)] = base;
				building->owner = state->getPlayer();

				stageCmd.cmd = StageCmd::Command::POP;
			}
			else
			{
				stageCmd.cmd = StageCmd::Command::PUSH;
				stageCmd.nextStage =
				    mksp<MessageBox>(tr("No Sale"), tr("Not enough money to buy this building."),
				                     MessageBox::ButtonOptions::Ok);
			}
		}
	}
}

void BaseBuyScreen::Update(StageCmd *const cmd)
{
	menuform->Update();
	*cmd = this->stageCmd;
	// Reset the command to default
	this->stageCmd = StageCmd();
}

void BaseBuyScreen::Render()
{
	fw().Stage_GetPrevious(this->shared_from_this())->Render();
	fw().renderer->drawFilledRect({0, 0}, fw().Display_GetSize(), Colour{0, 0, 0, 128});
	menuform->Render();
}

bool BaseBuyScreen::IsTransition() { return false; }

}; // namespace OpenApoc

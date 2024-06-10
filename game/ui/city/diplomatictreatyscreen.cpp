#include "game/ui/city/diplomatictreatyscreen.h"
#include "forms/form.h"
#include "forms/label.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/keycodes.h"
#include "game/state/gamestate.h"
#include "game/state/shared/organisation.h"

namespace OpenApoc
{

DiplomaticTreatyScreen::DiplomaticTreatyScreen(sp<GameState> state, StateRef<Organisation> org)
    : Stage(), menuform(ui().getForm("city/diplomatic_treaty")), state(state), organisation(org)
{
}

DiplomaticTreatyScreen::~DiplomaticTreatyScreen() = default;

void DiplomaticTreatyScreen::begin()
{
	menuform->findControlTyped<Label>("TEXT_DATE")->setText(state->gameTime.getLongDateString());
	menuform->findControlTyped<Label>("TEXT_FUNDS")->setText(state->getPlayerBalance());
	menuform->findControlTyped<Label>("TEXT_ORGANISATION")->setText(tr(organisation->name));
	labelOffer = menuform->findControlTyped<Label>("TEXT_OFFER");
	labelBribe = menuform->findControlTyped<Label>("TEXT_RIFT_BRIBE");

	bribeAmount = organisation->diplomaticRiftOffer(*state, state->getPlayer());

	labelOffer->setText(tr("We are unhappy with the recent activity of your organization and "
	                       "request compensation to restore normal diplomatic relations. If you do "
	                       "not comply your craft and Agents may be subject to hostile actions."));
	labelBribe->setText(format("Pay: $ %s ?", Strings::fromInteger(bribeAmount, true)));
}

void DiplomaticTreatyScreen::pause() {}

void DiplomaticTreatyScreen::resume() {}

void DiplomaticTreatyScreen::finish() {}

void DiplomaticTreatyScreen::eventOccurred(Event *e)
{
	menuform->eventOccured(e);

	if (e->type() == EVENT_KEY_DOWN)
	{
		switch (e->keyboard().KeyCode)
		{
			case SDLK_ESCAPE:
			case SDLK_RETURN:
			case SDLK_KP_ENTER:
				menuform->findControl("BUTTON_QUIT")->click();
				return;
		}
	}

	if (e->type() == EVENT_FORM_INTERACTION && e->forms().EventFlag == FormEventType::ButtonClick)
	{
		if (e->forms().RaisedBy->Name == "BUTTON_BRIBE")
		{
			if (bribeAmount > 0 && state->getPlayer()->balance > bribeAmount)
			{
				organisation->signTreatyWith(*state, state->getPlayer(), bribeAmount, false);
				fw().stageQueueCommand({StageCmd::Command::POP});
			}
			return;
		}
		if (e->forms().RaisedBy->Name == "BUTTON_QUIT")
		{
			fw().stageQueueCommand({StageCmd::Command::POP});
			return;
		}
	}
}

void DiplomaticTreatyScreen::update() { menuform->update(); }

void DiplomaticTreatyScreen::render()
{
	fw().stageGetPrevious(this->shared_from_this())->render();
	menuform->render();
}

bool DiplomaticTreatyScreen::isTransition() { return false; }

}; // namespace OpenApoc

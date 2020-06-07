#include "game/ui/city/bribescreen.h"
#include "forms/form.h"
#include "forms/label.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/keycodes.h"
#include "game/state/city/city.h"
#include "game/state/gamestate.h"
#include "game/state/shared/organisation.h"

namespace OpenApoc
{

BribeScreen::BribeScreen(sp<GameState> state)
    : Stage(), menuform(ui().getForm("city/bribe")), state(state),
      organisation(state->current_city->cityViewSelectedOrganisation)
{
}

BribeScreen::~BribeScreen() = default;

/**
 * Update info about deal.
 */
void BribeScreen::updateInfo()
{
	UString relationship;
	UString offer;
	bribe = organisation->costOfBribeBy(state->getPlayer());

	switch (organisation->isRelatedTo(state->getPlayer()))
	{
		case Organisation::Relation::Allied:
			relationship = ": allied with:";
			offer =
			    tr("X-COM is ALLIED with this organization. The relationship cannot be improved.");
			bribe = 0;
			break;

		case Organisation::Relation::Friendly:
			relationship = ": friendly with:";
			offer = getOfferString(bribe, tr("ALLIED"));
			break;

		case Organisation::Relation::Neutral:
			relationship = ": neutral towards:";
			offer = getOfferString(bribe, tr("FRIENDLY"));
			break;

		case Organisation::Relation::Unfriendly:
			relationship = ": unfriendly towards:";
			offer = getOfferString(bribe, tr("NEUTRAL"));
			break;

		case Organisation::Relation::Hostile:
			relationship = ": hostile towards:";
			if (organisation->isRelatedTo(state->getAliens()) == Organisation::Relation::Allied)
			{
				offer = tr("Whilst X-COM continue to oppose our Alien friends we will remain "
				           "hostile. Negotiations are impossible.");
				bribe = 0;
			}
			else
			{
				offer = getOfferString(bribe, tr("UNFRIENDLY"));
			}
			break;

		default:
			relationship = ": Attitude unknown towards:";
			offer = "Unconventional relations";
			bribe = 0;
			LogError(offer);
	}

	if (organisation->takenOver)
	{
		offer = tr("This organization is under Alien control.The Alien race will not enter "
		           "negotiations with X-COM.");
		bribe = 0;
	}

	labelFunds->setText(state->getPlayerBalance());
	labelRelation->setText(format("%s%s X-COM", tr(organisation->name), tr(relationship)));
	labelOffer->setText(offer);
}

/**
 * Get the offer of a bribe.
 * @param itWillCost - the sum of the bribe
 * @param newRelation - text of better attitude
 * @return - a text of the offer
 */
UString BribeScreen::getOfferString(int itWillCost, const UString &newAttitude) const
{
	return format("%s %d  %s  %s", tr("It will cost: $"), itWillCost,
	              tr("to improve relations to:"), newAttitude);
}

void BribeScreen::begin()
{
	menuform->findControlTyped<Label>("TEXT_DATE")->setText(state->gameTime.getLongDateString());
	labelFunds = menuform->findControlTyped<Label>("TEXT_FUNDS");
	labelRelation = menuform->findControlTyped<Label>("TEXT_RELATION");
	labelOffer = menuform->findControlTyped<Label>("TEXT_OFFER");

	updateInfo();
}

void BribeScreen::pause() {}

void BribeScreen::resume() {}

void BribeScreen::finish() {}

void BribeScreen::eventOccurred(Event *e)
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
			if (bribe > 0)
			{
				organisation->bribedBy(*state, state->getPlayer(), bribe);
				updateInfo();
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

void BribeScreen::update() { menuform->update(); }

void BribeScreen::render()
{
	fw().stageGetPrevious(this->shared_from_this())->render();
	menuform->render();
}

bool BribeScreen::isTransition() { return false; }

}; // namespace OpenApoc

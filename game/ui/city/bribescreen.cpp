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
#include "library/strings_translate.h"

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
	const auto &playername = state->getPlayer()->name;

	switch (organisation->isRelatedTo(state->getPlayer()))
	{
		case Organisation::Relation::Allied:
			relationship = tformat("{1}: allied with: {2}", organisation->name, playername);
			offer = tformat(
			    "{1} is ALLIED with this organization. The relationship cannot be improved.",
			    playername);
			bribe = 0;
			break;

		case Organisation::Relation::Friendly:
			relationship = tformat("{1}: friendly with: {2}", organisation->name, playername);
			offer = getOfferString(bribe, tformat("ALLIED"));
			break;

		case Organisation::Relation::Neutral:
			relationship = tformat("{1}: neutral towards: {2}", organisation->name, playername);
			offer = getOfferString(bribe, tformat("FRIENDLY"));
			break;

		case Organisation::Relation::Unfriendly:
			relationship = tformat("{1}: unfriendly towards: {2}", organisation->name, playername);
			offer = getOfferString(bribe, tformat("NEUTRAL"));
			break;

		case Organisation::Relation::Hostile:
			relationship = tformat("{1}: hostile towards: {2}", organisation->name, playername);
			if (organisation->isRelatedTo(state->getAliens()) == Organisation::Relation::Allied)
			{
				offer = tformat("Whilst {1} continue to oppose our Alien friends we will remain "
				                "hostile. Negotiations are impossible.",
				                playername);
				bribe = 0;
			}
			else
			{
				offer = getOfferString(bribe, tformat("UNFRIENDLY"));
			}
			break;

		default:
			relationship =
			    tformat("{1}: Attitude unknown towards: {2}", organisation->name, playername);
			offer = tformat("Unconventional relations");
			bribe = 0;
			LogError(offer);
	}

	if (organisation->takenOver)
	{
		offer = tformat("This organization is under Alien control.The Alien race will not enter "
		                "negotiations with {1}.",
		                playername);
		bribe = 0;
	}

	labelFunds->setText(state->getPlayerBalance());
	labelRelation->setText(relationship);
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
	return tformat("It will cost: ${1} to improve relations to: {2}", itWillCost, newAttitude);
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

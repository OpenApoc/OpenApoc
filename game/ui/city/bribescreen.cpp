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

/**
 * Get the offer of a bribe.
 * @param itWillCost - the sum of the bribe
 * @param newRelation - text of better attitude
 * @return - a text of the offer
 */
static UString getOfferString(int itWillCost, const Organisation::Relation newAttitude)
{
	switch (newAttitude)
	{
		case Organisation::Relation::Allied:
			return tformat("It will cost ${1} to improve relations to Allied") % itWillCost;
		case Organisation::Relation::Friendly:
			return tformat("It will cost ${1} to improve relations to Friendly") % itWillCost;
		case Organisation::Relation::Neutral:
			return tformat("It will cost ${1} to improve relations to Neutral") % itWillCost;
		case Organisation::Relation::Unfriendly:
			return tformat("It will cost ${1} to improve relations to Unfriendly") % itWillCost;
		case Organisation::Relation::Hostile:
			LogWarning("Trying to improve relations to hostile, but can't be lower than hostile to "
			           "improve from");
			return tformat("It will cost ${1} to improve relations to Hostile") % itWillCost;
	}
	LogError("getOfferString() called on unknown relation value");
	return "Invalid offer";
}

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
			relationship = tformat("{1}: allied with: {2}") % translate(organisation->name) %
			               translate(state->getPlayer()->name);
			offer = translate(
			    "X-COM is ALLIED with this organization. The relationship cannot be improved.");
			bribe = 0;
			break;

		case Organisation::Relation::Friendly:
			relationship = tformat("{1}: friendly towards: {2}") % translate(organisation->name) %
			               translate(state->getPlayer()->name);
			offer = getOfferString(bribe, Organisation::Relation::Allied);
			break;

		case Organisation::Relation::Neutral:
			relationship = tformat("{1}: neutral towards: {2}") % translate(organisation->name) %
			               translate(state->getPlayer()->name);
			offer = getOfferString(bribe, Organisation::Relation::Friendly);
			break;

		case Organisation::Relation::Unfriendly:
			relationship = tformat("{1}: unfriendly towards: {2}") % translate(organisation->name) %
			               translate(state->getPlayer()->name);
			offer = getOfferString(bribe, Organisation::Relation::Neutral);
			break;

		case Organisation::Relation::Hostile:
			relationship = tformat("{1}: hostile towards: {2}") % translate(organisation->name) %
			               translate(state->getPlayer()->name);
			if (organisation->isRelatedTo(state->getAliens()) == Organisation::Relation::Allied)
			{
				offer =
				    translate("Whilst X-COM continue to oppose our Alien friends we will remain "
				              "hostile. Negotiations are impossible.");
				bribe = 0;
			}
			else
			{
				offer = getOfferString(bribe, Organisation::Relation::Unfriendly);
			}
			break;

		default:
			LogError("Invalid relation");
	}

	if (organisation->takenOver)
	{
		offer = translate("This organization is under Alien control.The Alien race will not enter "
		                  "negotiations with X-COM.");
		bribe = 0;
	}

	labelFunds->setText(state->getPlayerBalance());
	labelRelation->setText(relationship);
	labelOffer->setText(offer);
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
				fw().stageQueueCommand({StageCmd::Command::POP});
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

#include "game/ui/city/weeklyfundingscreen.h"
#include "forms/form.h"
#include "forms/graphicbutton.h"
#include "forms/label.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/keycodes.h"
#include "game/state/gamestate.h"
#include "game/state/shared/organisation.h"
#include "game/ui/city/scorescreen.h"

namespace OpenApoc
{

WeeklyFundingScreen::WeeklyFundingScreen(sp<GameState> state)
    : Stage(), menuform(ui().getForm("city/weekly_funding")), state(state)
{
	labelCurrentIncome = menuform->findControlTyped<Label>("FUNDING_CURRENT");
	labelRatingDescription = menuform->findControlTyped<Label>("SENATE_RATING");
	labelAdjustment = menuform->findControlTyped<Label>("FUNDING_ADJUSTMENT");
	labelNextWeekIncome = menuform->findControlTyped<Label>("FUNDING_NEW");

	auto buttonOK = menuform->findControlTyped<GraphicButton>("BUTTON_OK");
	buttonOK->addCallback(FormEventType::ButtonClick,
	                      [](Event *) { fw().stageQueueCommand({StageCmd::Command::POP}); });
}

WeeklyFundingScreen::~WeeklyFundingScreen() = default;

void WeeklyFundingScreen::begin()
{
	// Validate that we can recieve funding
	if (state->fundingTerminated)
	{
		fw().stageQueueCommand({StageCmd::Command::POP});
		return;
	}

	menuform->findControlTyped<Label>("TEXT_FUNDS")->setText(state->getPlayerBalance());
	menuform->findControlTyped<Label>("TEXT_DATE")->setText(state->gameTime.getLongDateString());
	menuform->findControlTyped<Label>("TEXT_WEEK")->setText(state->gameTime.getWeekString());

	menuform->findControlTyped<Label>("TITLE")->setText(tr("WEEKLY FUNDING ASSESSMENT"));

	UString ratingDescription;

	const auto player = state->getPlayer();
	const auto government = state->getGovernment();
	int currentIncome = player->income;

	if (government->isRelatedTo(player) == Organisation::Relation::Hostile)
	{
		ratingDescription =
		    tr("The Senate has declared total hostility to X-COM and there will be no further "
		       "funding. Furthermore, the Senate will take any steps necessary to destroy the "
		       "X-COM organization if it refuses to cease operation.");

		currentIncome = 0;
	}
	else if (state->totalScore.getTotal() < -2400)
	{
		ratingDescription = tr("The Senate considers the performance of X-COM to be so abysmal "
		                       "that it will cease funding from now on.");

		currentIncome = 0;
	}
	else
	{
		const int rating = state->weekScore.getTotal();
		const int modifier = state->calculateFundingModifier();

		int neutralRatingThreshold = 0;
		if (!state->weekly_rating_rules.empty())
		{
			// last entry should be smallest positive value that gives funding increase
			neutralRatingThreshold = state->weekly_rating_rules.back().first;
		}

		const int availableGovFunds = government->balance / 2;

		if (availableGovFunds < currentIncome)
		{
			// Reduce this week's income if government doesn't have enough funds
			currentIncome = (availableGovFunds < 0) ? 0 : availableGovFunds;

			ratingDescription = tr("Unfortunately the Senate has to limit X-COM funding due to the "
			                       "poor state of government finances.");
		}
		else if (rating < 0)
		{
			ratingDescription = tr("The Senate is not pleased with the performance of X-COM and "
			                       "has reduced funding accordingly.");
		}
		else if (rating > neutralRatingThreshold)
		{
			ratingDescription = tr("The Senate has a favorable attitude to X-COM and has increased "
			                       "funding accordingly.");
		}
		else
		{
			// Neutral rating, no rating message. Move up the labels.
			labelAdjustment->Location.y -= 44;
			labelNextWeekIncome->Location.y -= 44;
		}

		// Income adjustment is still based on base player funding, not current one
		const int adjustment = (modifier == 0) ? 0 : player->income / modifier;

		labelAdjustment->setText(
		    format("%s $%s", tr("Funding adjustment>"), Strings::fromInteger(adjustment, true)));
		labelNextWeekIncome->setText(
		    format("%s $%s", tr("Income for next week>"),
		           Strings::fromInteger(currentIncome + adjustment, true)));
	}

	labelCurrentIncome->setText(
	    format("%s $%s", tr("Current income>"), Strings::fromInteger(currentIncome, true)));
	labelRatingDescription->setText(ratingDescription);
}

void WeeklyFundingScreen::pause() {}

void WeeklyFundingScreen::resume() {}

void WeeklyFundingScreen::finish()
{
	fw().stageQueueCommand({StageCmd::Command::PUSH, mksp<ScoreScreen>(this->state, true)});
}

void WeeklyFundingScreen::eventOccurred(Event *e)
{
	menuform->eventOccured(e);

	if (e->type() == EVENT_KEY_DOWN)
	{
		switch (e->keyboard().KeyCode)
		{
			case SDLK_ESCAPE:
			case SDLK_RETURN:
			case SDLK_KP_ENTER:
				menuform->findControl("BUTTON_OK")->click();
		}
		return;
	}
}

void WeeklyFundingScreen::update() { menuform->update(); }

void WeeklyFundingScreen::render()
{
	fw().stageGetPrevious(this->shared_from_this())->render();
	menuform->render();
}

bool WeeklyFundingScreen::isTransition() { return false; }

}; // namespace OpenApoc

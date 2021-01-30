#include "game/ui/city/weeklyfundingscreen.h"
#include "forms/form.h"
#include "forms/graphicbutton.h"
#include "forms/label.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/keycodes.h"
#include "game/state/gamestate.h"
#include "game/state/rules/weeklyrating.h"
#include "game/state/shared/organisation.h"

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
	menuform->findControlTyped<Label>("TEXT_FUNDS")->setText(state->getPlayerBalance());
	menuform->findControlTyped<Label>("TEXT_DATE")->setText(state->gameTime.getLongDateString());
	menuform->findControlTyped<Label>("TEXT_WEEK")->setText(state->gameTime.getWeekString());

	menuform->findControlTyped<Label>("TITLE")->setText(tr("WEEKLY FUNDING ASSESSMENT"));

	const UString emptyString;
	UString ratingDescription;

	const auto player = state->getPlayer();
	const auto government = state->getGovernment();

	if (government->isRelatedTo(player) == Organisation::Relation::Hostile)
	{
		ratingDescription =
		    tr("The Senate has declared total hostility to X-COM and there will be no further "
		       "funding. Furthermore, the Senate will take any steps necessary to destroy the "
		       "X-COM organization if it refuses to cease operation.");

		labelAdjustment->setText(emptyString);
		labelNextWeekIncome->setText(emptyString);
	}
	else if (state->totalScore.getTotal() < -2400)
	{
		ratingDescription = tr("The Senate considers the performance of X-COM to be so abysmal "
		                       "that it will cease funding from now on.");
		labelAdjustment->setText(emptyString);
		labelNextWeekIncome->setText(emptyString);
	}
	else
	{
		const auto rating = WeeklyRating::getRating(state->weekScore.getTotal());
		const int mod = WeeklyRating::getRatingModifier(rating);
		int adjustment = (mod == 0) ? 0 : player->income / mod;

		if (government->balance / 2 < player->income)
		{
			ratingDescription = tr("Unfortunately the Senate has to limit X-COM funding due to the "
			                       "poor state of government finances.");
		}
		else if (rating < WeeklyRating::Type::Neutral)
		{
			ratingDescription = tr("The Senate is not pleased with the performance of X-COM and "
			                       "has reduced funding accordingly.");
		}
		else if (rating > WeeklyRating::Type::Neutral)
		{
			ratingDescription = tr("The Senate has a favorable attitude to X-COM and has increased "
			                       "funding accordingly.");
		}
		else
		{
			// Neutral rating, no rating message. Move up next week income line.
			labelAdjustment->Location.y -= 44;
			labelNextWeekIncome->Location.y -= 44;
		}

		labelAdjustment->setText(format("%s $%d", tr("Funding adjustment>"), adjustment));
		labelNextWeekIncome->setText(
		    format("%s $%d", tr("Income for next week>"), player->income + adjustment));
	}

	labelCurrentIncome->setText(format("%s $%d", tr("Current income>"), player->income));
	labelRatingDescription->setText(ratingDescription);
}

void WeeklyFundingScreen::pause() {}

void WeeklyFundingScreen::resume() {}

void WeeklyFundingScreen::finish() {}

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

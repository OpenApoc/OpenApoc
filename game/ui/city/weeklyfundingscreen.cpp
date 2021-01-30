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

	auto player = state->getPlayer();
	labelCurrentIncome->setText(format("%s $%d", tr("Current income>"), player->income));
	labelAdjustment->setText(format("%s $%d", tr("Funding adjustment>"), 7500));
	labelNextWeekIncome->setText(
	    format("%s $%d", tr("Income for next week>"), player->income + 7500));

	UString ratingDescription;
	ratingDescription =
	    tr("The Senate has a favorable attitude to X-COM and has increased funding accordingly.");
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

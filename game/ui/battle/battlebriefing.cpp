#include "game/ui/battle/battlebriefing.h"
#include "forms/form.h"
#include "forms/graphicbutton.h"
#include "forms/label.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "game/state/battle/battlecommonimagelist.h"
#include "game/state/gamestate.h"
#include "game/ui/battle/battleprestart.h"
#include "game/ui/battle/battleview.h"
#include "game/ui/city/cityview.h"
#include <cmath>

namespace OpenApoc
{

BattleBriefing::BattleBriefing(sp<GameState> state, std::future<void> gameStateTask)
    : Stage(), menuform(ui().getForm("battle/briefing")), loading_task(std::move(gameStateTask)),
      state(state)
{
	menuform->findControlTyped<Label>("TEXT_DATE")->setText("Friday, 14th  July, 2084      17:35");
	menuform->findControlTyped<Label>("TEXT_BRIEFING")->setText("You must lorem ipisum etc.");
	menuform->findControlTyped<GraphicButton>("BUTTON_REAL_TIME")->setVisible(false);
	menuform->findControlTyped<GraphicButton>("BUTTON_TURN_BASED")->setVisible(false);

	menuform->findControlTyped<GraphicButton>("BUTTON_REAL_TIME")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    this->state->current_battle->setMode(Battle::Mode::RealTime);
		    fw().stageQueueCommand(
		        {StageCmd::Command::REPLACEALL, mksp<BattlePreStart>(this->state)});
		});

	menuform->findControlTyped<GraphicButton>("BUTTON_TURN_BASED")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    this->state->current_battle->setMode(Battle::Mode::TurnBased);
		    fw().stageQueueCommand(
		        {StageCmd::Command::REPLACEALL, mksp<BattlePreStart>(this->state)});
		});
}

void BattleBriefing::begin() {}

void BattleBriefing::pause() {}

void BattleBriefing::resume() {}

void BattleBriefing::finish() {}

void BattleBriefing::eventOccurred(Event *e) { menuform->eventOccured(e); }

void BattleBriefing::update()
{
	menuform->update();

	auto status = this->loading_task.wait_for(std::chrono::seconds(0));
	switch (status)
	{
		case std::future_status::ready:
		{
			menuform->findControlTyped<GraphicButton>("BUTTON_REAL_TIME")->setVisible(true);
			menuform->findControlTyped<GraphicButton>("BUTTON_TURN_BASED")->setVisible(true);
		}
			return;
		default:
			// Not yet finished
			return;
	}
}

void BattleBriefing::render()
{
	menuform->render();
	// FIXME: Draw "loading" in the bottom ?
}

bool BattleBriefing::isTransition() { return false; }

}; // namespace OpenApoc

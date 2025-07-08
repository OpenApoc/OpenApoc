#include "game/ui/battle/battledebriefing.h"
#include "forms/form.h"
#include "forms/graphicbutton.h"
#include "forms/label.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/keycodes.h"
#include "game/state/battle/battle.h"
#include "game/state/battle/battleunit.h"
#include "game/state/gamestate.h"
#include "game/ui/tileview/cityview.h"
#include "library/strings_format.h"
#include <cmath>

namespace OpenApoc
{
BattleDebriefing::BattleDebriefing(sp<GameState> state)
    : Stage(), menuform(ui().getForm("battle/debriefing")), state(state)
{
	menuform->findControlTyped<GraphicButton>("BUTTON_OK")
	    ->addCallback(FormEventType::ButtonClick,
	                  [this](Event *)
	                  {
		                  Battle::exitBattle(*this->state);
		                  fw().stageQueueCommand(
		                      {StageCmd::Command::REPLACEALL, mksp<CityView>(this->state)});
	                  });

	menuform->findControlTyped<Label>("TEXT_SCORE_COMBAT_RATING")
	    ->setText(fmt::format("{}", state->current_battle->score.combatRating));
	menuform->findControlTyped<Label>("TEXT_SCORE_CASUALTY_PENALTY")
	    ->setText(fmt::format("{}", state->current_battle->score.casualtyPenalty));
	menuform->findControlTyped<Label>("TEXT_SCORE_LEADERSHIP_BONUS")
	    ->setText(fmt::format("{}", state->current_battle->score.getLeadershipBonus()));
	menuform->findControlTyped<Label>("TEXT_SCORE_LIVE_ALIENS_CAPTURED")
	    ->setText(fmt::format("{}", state->current_battle->score.liveAlienCaptured));
	menuform->findControlTyped<Label>("TEXT_SCORE_EQUIPMENT_CAPTURED")
	    ->setText(fmt::format("{}", state->current_battle->score.equipmentCaptured));
	menuform->findControlTyped<Label>("TEXT_SCORE_EQUIPMENT_LOST")
	    ->setText(fmt::format("{}", state->current_battle->score.equipmentLost));
	menuform->findControlTyped<Label>("TEXT_SCORE_TOTAL")
	    ->setText(fmt::format("{}", state->current_battle->score.getTotal()));
	menuform->findControlTyped<Label>("TEXT_MISSION_PERFORMANCE")
	    ->setText(fmt::format("{}", state->current_battle->score.getText()));

	int idx = 1;
	for (auto &u : state->current_battle->unitsPromoted)
	{
		menuform->findControlTyped<Label>(fmt::format("PROMOTION_{}", idx++))
		    ->setText(fmt::format("{} {} {}", u->agent->name, tr("promoted to"),
		                          u->agent->getRankName()));
	}
}

void BattleDebriefing::begin() {}

void BattleDebriefing::pause() {}

void BattleDebriefing::resume() {}

void BattleDebriefing::finish() {}

void BattleDebriefing::eventOccurred(Event *e)
{
	menuform->eventOccured(e);
	if (e->type() == EVENT_KEY_DOWN)
	{
		if (e->keyboard().KeyCode == SDLK_RETURN || e->keyboard().KeyCode == SDLK_ESCAPE ||
		    e->keyboard().KeyCode == SDLK_SPACE || e->keyboard().KeyCode == SDLK_KP_ENTER)
		{
			menuform->findControl("BUTTON_OK")->click();
			return;
		}
	}
}

void BattleDebriefing::update() { menuform->update(); }

void BattleDebriefing::render() { menuform->render(); }

bool BattleDebriefing::isTransition() { return false; }

}; // namespace OpenApoc

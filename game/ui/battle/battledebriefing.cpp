#include "game/ui/battle/battledebriefing.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "game/state/battle/battle.h"
#include "game/state/battle/battlecommonimagelist.h"
#include "game/ui/city/cityview.h"
#include <cmath>

namespace OpenApoc
{
BattleDebriefing::BattleDebriefing(sp<GameState> state)
    : Stage(), menuform(ui().getForm("FORM_BATTLE_DEBRIEFING")), state(state)
{
	menuform->findControlTyped<GraphicButton>("BUTTON_OK")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    Battle::exitBattle(*this->state);

		    fw().stageQueueCommand({StageCmd::Command::PUSH, mksp<CityView>(this->state)});
		});
}

void BattleDebriefing::begin() {}

void BattleDebriefing::pause() {}

void BattleDebriefing::resume() {}

void BattleDebriefing::finish() {}

void BattleDebriefing::eventOccurred(Event *e) { menuform->eventOccured(e); }

void BattleDebriefing::update() { menuform->update(); }

void BattleDebriefing::render() { menuform->render(); }

bool BattleDebriefing::isTransition() { return false; }

}; // namespace OpenApoc

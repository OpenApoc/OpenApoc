#include "game/ui/battle/battleprestart.h"
#include "forms/form.h"
#include "forms/graphicbutton.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "game/state/battle/battlecommonimagelist.h"
#include "game/state/gamestate.h"
#include "game/ui/battle/battleview.h"
#include "game/ui/general/loadingscreen.h"
#include <cmath>

namespace OpenApoc
{

std::future<void> enterBattle(sp<GameState> state)
{
	auto loadTask =
	    fw().threadPoolEnqueue([state]() -> void { Battle::enterBattle(*state.get()); });

	return loadTask;
}

BattlePreStart::BattlePreStart(sp<GameState> state)
    : Stage(), menuform(ui().getForm("battle/prestart")), state(state)
{
	menuform->findControlTyped<GraphicButton>("BUTTON_OK")
	    ->addCallback(FormEventType::ButtonClick, [this, state](Event *) {

		    auto gameState = this->state;

		    fw().stageQueueCommand(
		        {StageCmd::Command::PUSH,
		         mksp<LoadingScreen>(gameState, enterBattle(gameState),
		                             [gameState]() { return mksp<BattleView>(gameState); },
		                             this->state->battle_common_image_list->loadingImage, 1)});
		});
}

void BattlePreStart::begin() {}

void BattlePreStart::pause() {}

void BattlePreStart::resume() {}

void BattlePreStart::finish() {}

void BattlePreStart::eventOccurred(Event *e) { menuform->eventOccured(e); }

void BattlePreStart::update() { menuform->update(); }

void BattlePreStart::render()
{
	menuform->render();
	// FIXME: Draw "loading" in the bottom ?
}

bool BattlePreStart::isTransition() { return false; }

}; // namespace OpenApoc

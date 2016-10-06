#include "game/ui/battle/battleprestart.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "game/state/battle/battlecommonimagelist.h"
#include "game/ui/battle/battleview.h"
#include "game/ui/general/loadingscreen.h"
#include <cmath>

namespace OpenApoc
{

std::future<sp<GameState>> enterBattle(sp<GameState> state)
{

	auto loadTask = fw().threadPoolEnqueue([state]() -> sp<GameState> {

		Battle::enterBattle(*state.get());

		return state;
	});

	return loadTask;
}

BattlePreStart::BattlePreStart(sp<GameState> state)
    : Stage(), menuform(ui().getForm("FORM_BATTLE_PRESTART")), state(state)
{
	menuform->findControlTyped<GraphicButton>("BUTTON_OK")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {

		    fw().stageQueueCommand(
		        {StageCmd::Command::PUSH,
		         mksp<LoadingScreen>(enterBattle(this->state),
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

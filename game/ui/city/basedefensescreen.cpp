#include "game/ui/city/basedefensescreen.h"
#include "forms/form.h"
#include "forms/label.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/keycodes.h"
#include "game/state/battle/battle.h"
#include "game/state/city/base.h"
#include "game/state/gamestate.h"
#include "game/ui/battle/battlebriefing.h"
#include "game/ui/general/aequipscreen.h"

namespace OpenApoc
{

namespace
{
std::shared_future<void> loadBattleBase(sp<GameState> state, StateRef<Base> base,
                                        StateRef<Organisation> attacker)
{
	auto loadTask = fw().threadPoolEnqueue([base, state, attacker]() -> void {
		std::list<StateRef<Agent>> agents;
		StateRef<Vehicle> veh = {};

		bool hotseat = false;
		const std::map<StateRef<AgentType>, int> *aliens = nullptr;
		const int *guards = nullptr;
		const int *civilians = nullptr;

		Battle::beginBattle(*state, hotseat, attacker, agents, aliens, guards, civilians, veh,
		                    base->building);
	});

	return loadTask;
}
} // namespace

BaseDefenseScreen::BaseDefenseScreen(sp<GameState> state, StateRef<Base> base,
                                     StateRef<Organisation> attacker)
    : Stage(), menuform(ui().getForm("city/basedefense")), state(state), base(base),
      attacker(attacker)
{
}

BaseDefenseScreen::~BaseDefenseScreen() = default;

void BaseDefenseScreen::initiateDefenseMission(StateRef<Base> base, StateRef<Organisation> attacker)
{
	bool isBuilding = true;
	bool isRaid = false;
	fw().stageQueueCommand({StageCmd::Command::REPLACEALL,
	                        mksp<BattleBriefing>(state, attacker, base->building.id, isBuilding,
	                                             isRaid, loadBattleBase(state, base, attacker))});
}
void BaseDefenseScreen::begin()
{
	menuform->findControlTyped<Label>("BASE_NAME")->setText(base->name);
}

void BaseDefenseScreen::pause() {}

void BaseDefenseScreen::resume() {}

void BaseDefenseScreen::finish() {}

void BaseDefenseScreen::eventOccurred(Event *e)
{
	menuform->eventOccured(e);

	if (e->type() == EVENT_KEY_DOWN)
	{
		if (e->keyboard().KeyCode == SDLK_ESCAPE || e->keyboard().KeyCode == SDLK_RETURN ||
		    e->keyboard().KeyCode == SDLK_KP_ENTER)
		{
			menuform->findControl("BUTTON_QUIT")->click();
			return;
		}
	}

	if (e->type() == EVENT_FORM_INTERACTION && e->forms().EventFlag == FormEventType::ButtonClick)
	{
		if (e->forms().RaisedBy->Name == "BUTTON_EQUIPAGENT")
		{
			fw().stageQueueCommand({StageCmd::Command::PUSH, mksp<AEquipScreen>(this->state)});
			return;
		}
		if (e->forms().RaisedBy->Name == "BUTTON_QUIT")
		{
			initiateDefenseMission(base, attacker);
		}
	}
}

void BaseDefenseScreen::update() { menuform->update(); }

void BaseDefenseScreen::render()
{
	fw().stageGetPrevious(this->shared_from_this())->render();
	menuform->render();
}

bool BaseDefenseScreen::isTransition() { return false; }

}; // namespace OpenApoc

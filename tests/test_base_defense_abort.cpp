#include "framework/configfile.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "game/state/battle/battle.h"
#include "game/state/battle/battleunit.h"
#include "game/state/city/base.h"
#include "game/state/city/building.h"
#include "game/state/city/city.h"
#include "game/state/gameevent.h"
#include "game/state/gamestate.h"
#include "game/state/message.h"
#include "game/state/shared/agent.h"
#include "game/state/shared/organisation.h"
#include "game/ui/tileview/cityview.h"
#include <iostream>

using namespace OpenApoc;

namespace
{

int failures = 0;

bool check(bool cond, const std::string &what)
{
	if (cond)
	{
		std::cout << "  PASS: " << what << "\n";
	}
	else
	{
		std::cout << "  FAIL: " << what << "\n";
		failures++;
	}
	return cond;
}

// fillPlayerStartingProperty() creates exactly one base and its building
// already, so "reduce to exactly one base" is a no-op verification rather
// than an active trim.
bool haveExactlyOneBase(GameState &state)
{
	return state.player_bases.size() == 1 && !state.current_base.id.empty();
}

std::list<StateRef<BattleUnit>> playerUnits(GameState &state)
{
	std::list<StateRef<BattleUnit>> result;
	auto player = state.getPlayer();
	for (auto &u : state.current_battle->units)
	{
		if (u.second->owner == player)
		{
			result.emplace_back(&state, u.second);
		}
	}
	return result;
}

// Starts a BaseDefense battle against the player's (only) base, the same way
// BaseDefenseScreen does when a base comes under attack.
bool startBaseDefense(sp<GameState> state)
{
	if (!check(haveExactlyOneBase(*state),
	           "exactly one player base after fillPlayerStartingProperty()"))
	{
		return false;
	}

	StateRef<Building> baseBuilding = state->current_base->building;
	std::list<StateRef<Agent>> agents;
	const std::map<StateRef<AgentType>, int> *aliens = nullptr;
	const int *guards = nullptr;
	const int *civilians = nullptr;

	Battle::beginBattle(*state, false, state->getAliens(), agents, aliens, guards, civilians, {},
	                    baseBuilding);
	if (!check((bool)state->current_battle, "Battle::beginBattle created a battle"))
	{
		return false;
	}
	if (!check(state->current_battle->mission_type == Battle::MissionType::BaseDefense,
	           "battle came up as BaseDefense"))
	{
		return false;
	}

	Battle::enterBattle(*state);

	auto units = playerUnits(*state);
	if (!check(!units.empty(), "player units spawned for the base defense battle"))
	{
		return false;
	}
	for (auto &u : units)
	{
		if (!check((bool)u->tileObject, "spawned player unit has a tile position"))
		{
			return false;
		}
	}
	return true;
}

// Asserts the state left behind by Battle::exitBattle() once the base is lost:
// this is the exact state CityView's constructor has to survive.
bool checkBaseLostState(GameState &state)
{
	bool ok = check(state.player_bases.empty(), "player_bases is empty after the base is lost");
	ok = check(state.current_base.id.empty(), "current_base.id is empty after the base is lost") &&
	     ok;
	ok = check(state.eventFromBattle == GameEventType::GameLost,
	           "eventFromBattle == GameLost after the base is lost") &&
	     ok;
	return ok;
}

// StageStack::push() calls begin() exactly once, synchronously, while applying the debriefing
// screen's REPLACEALL - before the framework's next processEvents() dispatches anything to the
// new current stage. CityView::begin() is the only place that may realize a pending after-battle
// GameLost: constructing CityView alone (which the test already did) must leave it untouched, and
// realizing it any earlier would queue the event while some other stage is still current, where it
// is silently discarded. The framework's own event queue is private, so this feeds the event
// CityView::begin() would have queued straight into the same handler that queue eventually
// dispatches to, and checks the one side effect of that handler that is reachable headlessly: the
// message it logs onto GameState.
bool checkGameLostDelivery(sp<GameState> state, sp<CityView> cityView)
{
	bool ok = check(state->gameTimeBeforeBattle.getTicks() != 0,
	                "after-battle update still pending once CityView is merely constructed");

	cityView->begin();

	ok = check(state->gameTimeBeforeBattle.getTicks() == 0,
	           "CityView::begin() realized the pending after-battle update") &&
	     ok;

	GameEvent gameLostEvent(GameEventType::GameLost);
	bool handled = cityView->handleGameStateEvent(&gameLostEvent);
	ok = check(handled, "CityView::handleGameStateEvent() accepts a GameLost event") && ok;
	ok = check(!state->messages.empty() && state->messages.back().text == gameLostEvent.message(),
	           "the GameLost message is logged, matching what begin()'s queued event would produce "
	           "once CityView is the current stage") &&
	     ok;
	return ok;
}

// Mirrors BUTTON_EXIT_BATTLE's Yes callback in ingameoptions.cpp: every
// conscious player unit retreats without being killed, which still costs the
// last base because BaseDefense treats any non-win as the base being lost.
bool runAbortCase()
{
	std::cout << "Case: abort with zero losses\n";

	auto state = mksp<GameState>();
	if (!check(state->loadGame(config().getString("common")) &&
	               state->loadGame(config().getString("gamestate")),
	           "loaded common and gamestate for the abort case"))
	{
		return false;
	}
	state->startGame();
	state->initState();
	state->fillPlayerStartingProperty();

	if (!startBaseDefense(state))
	{
		return false;
	}

	auto units = playerUnits(*state);
	check(!units.empty(), "player units present to retreat");
	for (auto &u : units)
	{
		if (u->isConscious())
		{
			u->retreat(*state);
		}
	}
	check(state->current_battle->playerWon == false,
	      "playerWon == false once every unit has retreated");

	Battle::finishBattle(*state);
	Battle::exitBattle(*state);

	bool ok = checkBaseLostState(*state);

	// CityView must come up on a null current_base without throwing; see checkGameLostDelivery()
	// below for how the pending GameLost is actually delivered from here.
	sp<CityView> cityView;
	try
	{
		cityView = mksp<CityView>(state);
	}
	catch (...)
	{
		check(false, "CityView construction with an empty current_base does not throw");
		return ok && false;
	}
	check(cityView != nullptr, "CityView constructed with an empty current_base");

	ok = checkGameLostDelivery(state, cityView) && ok;

	state->current_battle = nullptr;
	return ok && cityView != nullptr;
}

// The same Base::die() route is taken when every player unit is genuinely
// killed rather than retreated, so the crash is reachable without an abort.
bool runWipeoutCase()
{
	std::cout << "Case: genuine wipeout (no abort)\n";

	auto state = mksp<GameState>();
	if (!check(state->loadGame(config().getString("common")) &&
	               state->loadGame(config().getString("gamestate")),
	           "loaded common and gamestate for the wipeout case"))
	{
		return false;
	}
	state->startGame();
	state->initState();
	state->fillPlayerStartingProperty();

	if (!startBaseDefense(state))
	{
		return false;
	}

	auto units = playerUnits(*state);
	check(!units.empty(), "player units present to kill");
	for (auto &u : units)
	{
		u->agent->modified_stats.health = 0;
		u->die(*state, nullptr, false);
	}
	check(state->current_battle->playerWon == false, "playerWon == false once every unit is dead");

	Battle::finishBattle(*state);
	Battle::exitBattle(*state);

	bool ok = checkBaseLostState(*state);

	sp<CityView> cityView;
	try
	{
		cityView = mksp<CityView>(state);
	}
	catch (...)
	{
		check(false, "CityView construction with an empty current_base does not throw");
		return ok && false;
	}
	check(cityView != nullptr, "CityView constructed with an empty current_base");

	ok = checkGameLostDelivery(state, cityView) && ok;

	state->current_battle = nullptr;
	return ok && cityView != nullptr;
}

} // namespace

int main(int argc, char **argv)
{
	config().addPositionalArgument("common", "Common gamestate to load");
	config().addPositionalArgument("gamestate", "Gamestate to load");

	if (config().parseOptions(argc, argv))
	{
		return EXIT_FAILURE;
	}

	auto common_name = config().getString("common");
	auto gamestate_name = config().getString("gamestate");
	if (common_name.empty() || gamestate_name.empty())
	{
		std::cerr << "Must provide common and gamestate\n";
		config().showHelp();
		return EXIT_FAILURE;
	}

	Framework fw("OpenApoc", false);

	// Each case's individual assertions already feed the shared failures counter via check();
	// these just report which case a failure came from.
	if (!runAbortCase())
	{
		LogError("Abort case failed");
	}
	if (!runWipeoutCase())
	{
		LogError("Wipeout case failed");
	}

	std::cout << (failures == 0 ? "ALL CHECKS PASSED\n" : "SOME CHECKS FAILED\n");
	return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

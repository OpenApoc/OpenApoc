#include "framework/configfile.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "game/state/battle/battle.h"
#include "game/state/battle/battledoor.h"
#include "game/state/battle/battlescanner.h"
#include "game/state/battle/battleunit.h"
#include "game/state/city/base.h"
#include "game/state/city/building.h"
#include "game/state/city/city.h"
#include "game/state/gamestate.h"
#include "game/state/tilemap/tilemap.h"
#include "game/ui/skirmish/skirmish.h"
#include "library/sp.h"
#include <algorithm>
#include <iostream>

using namespace OpenApoc;

// Skirmish::goToBattle() does the whole battle setup synchronously (temporary building, temporary
// base, freshly generated agents) and only then arms the asynchronous load. Pressing OK a second
// time before that load finishes used to re-enter it, minting a second BUILDING_SKIRMISH /
// BASE_SKIRMISH and a second batch of agents over the top of the first while a background task was
// still reading them.

// The city keeps a StateRef per building, so a repeated goToBattle() leaves duplicate
// BUILDING_SKIRMISH references behind even though the map entry is overwritten.
static int countSkirmishBuildingRefs(const sp<City> &city)
{
	int count = 0;
	for (auto &b : city->buildings)
	{
		if (b.id == "BUILDING_SKIRMISH")
		{
			count++;
		}
	}
	return count;
}

static StateRef<Building> pickSkirmishTarget(sp<GameState> state)
{
	auto city = state->cities["CITYMAP_HUMAN"];
	for (auto &b : city->buildings)
	{
		if (b->owner != state->getPlayer() && !b->battle_map.id.empty())
		{
			return b;
		}
	}
	return {};
}

static bool test_second_gotobattle_is_refused(sp<GameState> state)
{
	LogInfo("Testing that a second goToBattle() while a load is pending is refused...");

	auto target = pickSkirmishTarget(state);
	if (!target)
	{
		LogError("No human-city building with a battle map to use as a skirmish target");
		return false;
	}

	auto skirmish = mksp<Skirmish>(state);
	skirmish->setLocation(target);

	skirmish->goToBattle();

	auto city = state->cities["CITYMAP_HUMAN"];
	// The first call must have run to completion - it is what arms the pending load, and the
	// guard is only meaningful if the state below actually got built.
	if (state->buildings.find("BUILDING_SKIRMISH") == state->buildings.end() ||
	    state->player_bases.find("BASE_SKIRMISH") == state->player_bases.end())
	{
		LogError("First goToBattle() did not create the temporary skirmish building/base");
		return false;
	}
	auto firstBuilding = state->buildings["BUILDING_SKIRMISH"];
	auto firstBase = state->player_bases["BASE_SKIRMISH"];
	auto agentsAfterFirst = state->agents.size();
	auto refsAfterFirst = countSkirmishBuildingRefs(city);
	if (refsAfterFirst != 1)
	{
		LogError(
		    "Expected exactly 1 BUILDING_SKIRMISH reference in the city after one call, got {0}",
		    refsAfterFirst);
		return false;
	}

	skirmish->goToBattle();

	if (state->buildings["BUILDING_SKIRMISH"] != firstBuilding)
	{
		LogError("Second goToBattle() replaced BUILDING_SKIRMISH while a load was pending");
		return false;
	}
	if (state->player_bases["BASE_SKIRMISH"] != firstBase)
	{
		LogError("Second goToBattle() replaced BASE_SKIRMISH while a load was pending");
		return false;
	}
	if (state->agents.size() != agentsAfterFirst)
	{
		LogError("Second goToBattle() generated more agents: {0} -> {1}", agentsAfterFirst,
		         state->agents.size());
		return false;
	}
	if (countSkirmishBuildingRefs(city) != 1)
	{
		LogError(
		    "Second goToBattle() added another BUILDING_SKIRMISH reference to the city, now {0}",
		    countSkirmishBuildingRefs(city));
		return false;
	}

	LogInfo("Second goToBattle() was refused as expected");
	return true;
}

// Hotseat only changes a flag carried into the battle build, so the first call must still go
// through on a fresh stage.
static bool test_first_gotobattle_still_works(sp<GameState> state)
{
	LogInfo("Testing that a fresh Skirmish stage still starts a battle setup...");

	state->buildings.erase("BUILDING_SKIRMISH");
	state->player_bases.erase("BASE_SKIRMISH");
	auto city = state->cities["CITYMAP_HUMAN"];
	auto &cityBuildings = city->buildings;
	cityBuildings.erase(std::remove_if(cityBuildings.begin(), cityBuildings.end(),
	                                   [](const StateRef<Building> &b)
	                                   { return b.id == "BUILDING_SKIRMISH"; }),
	                    cityBuildings.end());

	auto target = pickSkirmishTarget(state);
	auto skirmish = mksp<Skirmish>(state);
	skirmish->setLocation(target);
	skirmish->goToBattle();

	if (state->buildings.find("BUILDING_SKIRMISH") == state->buildings.end())
	{
		LogError("A fresh Skirmish stage failed to set up a battle");
		return false;
	}

	LogInfo("Fresh Skirmish stage still sets up a battle");
	return true;
}

// A rejected Battle::beginBattle() used to be silent, so the losing skirmish loader carried on and
// overwrote the winning battle's skirmish bookkeeping. It now reports failure instead.
static bool test_beginbattle_refuses_while_a_battle_is_in_progress(sp<GameState> state)
{
	LogInfo("Testing that beginBattle reports failure while another battle is in progress...");

	auto target = pickSkirmishTarget(state);
	if (!target)
	{
		LogError("No human-city building with a battle map to use as a battle target");
		return false;
	}

	auto existing = mksp<Battle>();
	state->current_battle = existing;

	std::list<StateRef<Agent>> agents;
	StateRef<Vehicle> noVehicle;
	bool started = Battle::beginBattle(*state, false, state->getAliens(), agents, nullptr, nullptr,
	                                   nullptr, noVehicle, target);

	// Mirrors what the skirmish loaders do once a battle really has started. Before beginBattle
	// reported failure this block ran unconditionally, so a rejected load wrote its own
	// bookkeeping over the battle that actually won the race.
	if (started)
	{
		state->current_battle->skirmish = true;
		state->current_battle->scoreBeforeSkirmish = state->totalScore.tacticalMissions;
	}

	bool passed = true;
	if (started)
	{
		LogError("beginBattle reported success while another battle was in progress");
		passed = false;
	}
	if (state->current_battle != existing)
	{
		LogError("beginBattle replaced the in-progress battle after refusing to start");
		passed = false;
	}
	if (existing->skirmish)
	{
		LogError("A rejected battle load clobbered the in-progress battle's skirmish bookkeeping");
		passed = false;
	}

	state->current_battle = nullptr;

	if (passed)
	{
		LogInfo("beginBattle refused the second battle and left current_battle alone");
	}
	return passed;
}

int main(int argc, char **argv)
{
	OpenApoc::config().addPositionalArgument("common", "Common gamestate to load");
	OpenApoc::config().addPositionalArgument("gamestate", "Gamestate to load");

	if (OpenApoc::config().parseOptions(argc, argv))
	{
		return EXIT_FAILURE;
	}

	auto gamestate_name = OpenApoc::config().getString("gamestate");
	auto common_name = OpenApoc::config().getString("common");
	if (common_name.empty() || gamestate_name.empty())
	{
		std::cerr << "Must provide common and gamestate\n";
		OpenApoc::config().showHelp();
		return EXIT_FAILURE;
	}

	OpenApoc::Framework fw("OpenApoc", false);

	LogInfo("Loading common gamestate \"{0}\"", common_name);
	auto state = OpenApoc::mksp<OpenApoc::GameState>();
	if (!state->loadGame(common_name))
	{
		LogError("Failed to load common gamestate");
		return EXIT_FAILURE;
	}

	LogInfo("Loading gamestate \"{0}\"", gamestate_name);
	if (!state->loadGame(gamestate_name))
	{
		LogError("Failed to load supplied gamestate");
		return EXIT_FAILURE;
	}

	state->startGame();
	state->initState();
	state->fillPlayerStartingProperty();

	int failures = 0;
	if (!test_second_gotobattle_is_refused(state))
	{
		failures++;
	}
	if (!test_first_gotobattle_still_works(state))
	{
		failures++;
	}
	if (!test_beginbattle_refuses_while_a_battle_is_in_progress(state))
	{
		failures++;
	}

	if (failures > 0)
	{
		LogError("test_skirmish_reentrancy failed - {0} test(s) failed", failures);
		return EXIT_FAILURE;
	}

	LogInfo("test_skirmish_reentrancy success - all tests passed");
	return EXIT_SUCCESS;
}

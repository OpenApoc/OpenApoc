// Standalone headless harness for OpenApoc issue #647
// ("Mothership Mission Won't Auto-end").
//
// Reproduces the checkMissionEnd() early-return guard swallowing the
// recompute that should happen when the last hostile unit dies, in the case
// where missionEndTimer is already running from an earlier, unrelated
// recheck (e.g. a different unit having been stunned and later fully
// killed). Not wired into ctest: it is a diagnostic tool for this issue,
// run manually against the savegame attached to the issue.
#include "framework/configfile.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "game/state/battle/battle.h"
#include "game/state/battle/battleunit.h"
#include "game/state/gamestate.h"
#include "game/state/gamestate_serialize.h"
#include "game/state/rules/agenttype.h"
#include "game/state/shared/agent.h"
#include "game/state/shared/organisation.h"
#include <iostream>

using namespace OpenApoc;

namespace
{

// Force a unit into a fully-conscious, undamaged state, standing in for
// "the last hostile still alive" before its final kill.
void reviveUnit(BattleUnit &unit)
{
	unit.destroyed = false;
	unit.retreated = false;
	unit.stunDamage = 0;
	unit.agent->modified_stats.health = unit.agent->current_stats.health;
	unit.current_body_state = BodyState::Standing;
	unit.target_body_state = BodyState::Standing;
}

StateRef<BattleUnit> findAlienUnit(GameState &state)
{
	auto aliens = state.getAliens();
	for (auto &u : state.current_battle->units)
	{
		if (u.second->owner == aliens)
		{
			return {&state, u.second};
		}
	}
	return {};
}

int failures = 0;

void check(bool cond, const std::string &what)
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
}

// The scenario from the prompt: missionEndTimer is already nonzero (as if a
// prior, unrelated recheck left it running) and playerWon is stale (false),
// then the true last hostile dies. Correct behaviour must recompute both.
void runGuardedTimerCase(GameState &state)
{
	std::cout << "Case: last kill with pre-seeded missionEndTimer\n";
	auto &battle = *state.current_battle;
	auto unit = findAlienUnit(state);
	check((bool)unit, "found an alien unit in the save to use as the last hostile");
	if (!unit)
	{
		return;
	}

	reviveUnit(*unit);
	battle.missionEndTimer = 7; // simulate a prior, unrelated recheck already running
	battle.playerWon = false;   // stale conclusion from that prior recheck

	unit->die(state, nullptr, false); // non-violent kill, matches a Toxin C hit

	check(battle.missionEndTimer > 0, "missionEndTimer > 0 after the kill");
	check(battle.playerWon == true, "playerWon == true after the kill");
}

// Plain case: timer starts at 0 (no interfering prior recheck). Must show no
// regression: the normal, unguarded path still works.
void runPlainCase(GameState &state)
{
	std::cout << "Case: last kill with missionEndTimer starting at 0\n";
	auto &battle = *state.current_battle;
	auto unit = findAlienUnit(state);
	check((bool)unit, "found an alien unit in the save to use as the last hostile");
	if (!unit)
	{
		return;
	}

	reviveUnit(*unit);
	battle.missionEndTimer = 0;
	battle.playerWon = false;

	unit->die(state, nullptr, false);

	check(battle.missionEndTimer > 0, "missionEndTimer > 0 after the kill");
	check(battle.playerWon == true, "playerWon == true after the kill");
}

// Loads the ruleset first, then the save on top, matching the pattern used
// by every other test in this repo that loads a save (see
// test_lab_assignment.cpp and test_serialize.cpp): without the ruleset
// loaded first, StateRefs the save doesn't itself carry full definitions for
// fail to resolve.
sp<GameState> loadSaveOnRuleset(const UString &common_name, const UString &gamestate_name)
{
	auto state = mksp<GameState>();
	if (!state->loadGame(common_name))
	{
		LogError("Failed to load common gamestate \"{0}\"", common_name);
		return nullptr;
	}
	if (!state->loadGame(gamestate_name))
	{
		LogError("Failed to load gamestate \"{0}\"", gamestate_name);
		return nullptr;
	}
	if (!state->current_battle)
	{
		LogError("Loaded gamestate has no current_battle");
		return nullptr;
	}
	// Not state->initState(): that also runs City::initCity() over
	// state->cities, and this savegame's city data references scenery tile
	// types (e.g. CITYTILE_ALIENMAP_5) that no longer exist in either the
	// save or the current ruleset, which segfaults there. The battle is all
	// this harness needs, so initialise only that.
	state->current_battle->initBattle(*state);
	return state;
}

} // namespace

int main(int argc, char **argv)
{
	config().addPositionalArgument("common", "Common ruleset gamestate to load first");
	config().addPositionalArgument("gamestate", "Issue #647 savegame to load on top of it");

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

	auto state = loadSaveOnRuleset(common_name, gamestate_name);
	if (!state)
	{
		return EXIT_FAILURE;
	}

	std::cout << "Loaded save. missionEndTimer=" << state->current_battle->missionEndTimer
	          << " playerWon=" << state->current_battle->playerWon << "\n";
	// GameState::~GameState() runs Battle::finishBattle() whenever
	// current_battle is set, which assumes a battle exited the normal way
	// (via the city/vehicle bookkeeping this harness never set up) and
	// segfaults on teardown. We are done with the battle by the time each
	// GameState goes out of scope, so drop the reference to skip that.
	state->current_battle = nullptr;

	// Each case gets a completely fresh GameState so they can't interfere.
	{
		auto s1 = loadSaveOnRuleset(common_name, gamestate_name);
		if (!s1)
		{
			return EXIT_FAILURE;
		}
		runGuardedTimerCase(*s1);
		s1->current_battle = nullptr;
	}
	{
		auto s2 = loadSaveOnRuleset(common_name, gamestate_name);
		if (!s2)
		{
			return EXIT_FAILURE;
		}
		runPlainCase(*s2);
		s2->current_battle = nullptr;
	}

	std::cout << (failures == 0 ? "ALL CHECKS PASSED\n" : "SOME CHECKS FAILED\n");
	return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

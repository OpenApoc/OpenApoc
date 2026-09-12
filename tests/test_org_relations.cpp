#include "framework/configfile.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "game/state/gamestate.h"
#include "game/state/gamestate_serialize.h"
#include "game/state/shared/organisation.h"
#include "library/strings.h"
#include <cmath>
#include <iostream>

using namespace OpenApoc;

static bool nearlyEqual(float a, float b, float epsilon = 0.001f)
{
	return std::fabs(a - b) < epsilon;
}

// long_term_relations should stay put across ordinary adjustRelationTo() calls and daily
// updates, and only move when a qualifying narrative event (here, a forced treaty) happens.
static bool test_long_term_event_gated(sp<GameState> state)
{
	LogInfo("Testing long_term_relations is event-gated...");

	auto org = state->getOrganisation("ORG_MEGAPOL");
	auto player = state->getPlayer();

	org->long_term_relations[player] = 0.0f;
	org->current_relations[player] = 0.0f;

	// Routine friction: several ordinary relation adjustments
	for (int i = 0; i < 5; i++)
	{
		org->adjustRelationTo(*state, player, -6.0f);
	}

	if (!nearlyEqual(org->long_term_relations[player], 0.0f))
	{
		LogError("Expected long_term_relations untouched by routine adjustRelationTo calls, "
		         "got {0}",
		         org->long_term_relations[player]);
		return false;
	}

	// A daily update should not re-baseline long_term_relations either
	org->updateRelations(player);

	if (!nearlyEqual(org->long_term_relations[player], 0.0f))
	{
		LogError("Expected long_term_relations untouched by updateRelations(), got {0}",
		         org->long_term_relations[player]);
		return false;
	}

	// A qualifying narrative event (forced alliance treaty) should re-baseline it
	org->signTreatyWith(*state, player, 0, true);

	if (!nearlyEqual(org->long_term_relations[player], 100.0f) ||
	    !nearlyEqual(org->current_relations[player], 100.0f))
	{
		LogError("Expected forced treaty to snap both current and long_term to 100, got "
		         "current={0} long_term={1}",
		         org->current_relations[player], org->long_term_relations[player]);
		return false;
	}

	LogInfo("Long-term event-gating test passed");
	return true;
}

// updateRelations()'s day-over-day delta (used for the bribe-request diplomacy trigger) must
// keep working exactly as before, now that it is decoupled from long_term_relations.
static bool test_daily_delta_unaffected(sp<GameState> state)
{
	LogInfo("Testing day-over-day relationship delta is unaffected by event-gating...");

	auto org = state->getOrganisation("ORG_MEGAPOL");
	auto player = state->getPlayer();

	org->current_relations[player] = 0.0f;
	org->previous_relations[player] = 0.0f;
	org->long_term_relations[player] = 0.0f;

	org->current_relations[player] = -20.0f;
	float delta = org->updateRelations(player);

	if (!nearlyEqual(delta, -20.0f))
	{
		LogError("Expected a -20 day-over-day delta, got {0}", delta);
		return false;
	}

	// Next day, nothing changes: delta should now be 0, since previous_relations tracks yesterday
	delta = org->updateRelations(player);
	if (!nearlyEqual(delta, 0.0f))
	{
		LogError("Expected a 0 day-over-day delta on an unchanged day, got {0}", delta);
		return false;
	}

	LogInfo("Day-over-day delta test passed");
	return true;
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
	if (gamestate_name.empty() || common_name.empty())
	{
		std::cerr << "Must provide common and gamestate\n";
		OpenApoc::config().showHelp();
		return EXIT_FAILURE;
	}

	OpenApoc::Framework fw("OpenApoc", false);

	auto state = OpenApoc::mksp<OpenApoc::GameState>();
	if (!state->loadGame(common_name) || !state->loadGame(gamestate_name))
	{
		LogError("Failed to load gamestate");
		return EXIT_FAILURE;
	}

	// Loading twice (as test_lab_assignment.cpp does) ensures cross-references between the base
	// ruleset and the difficulty overlay are fully resolved before StateRefs are used below.
	state->loadGame(common_name);
	state->loadGame(gamestate_name);
	state->startGame();
	state->initState();
	state->fillPlayerStartingProperty();

	bool ok = true;
	ok &= test_long_term_event_gated(state);
	ok &= test_daily_delta_unaffected(state);

	if (!ok)
	{
		LogError("test_org_relations FAILED");
		return EXIT_FAILURE;
	}

	LogInfo("test_org_relations success - all tests passed");
	return EXIT_SUCCESS;
}

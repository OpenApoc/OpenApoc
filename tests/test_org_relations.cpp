#include "framework/configfile.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "game/state/battle/battle.h"
#include "game/state/city/vehicle.h"
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

// Ally ripple: a third-party org that likes 'row' should react in the same direction as the
// requested delta, and by more when it likes 'row' more (with extra damping when it dislikes it).
static bool test_ripple_sign_and_magnitude(sp<GameState> state)
{
	LogInfo("Testing ripple sign and magnitude...");

	auto row = state->getOrganisation("ORG_GOVERNMENT");
	auto col = state->getOrganisation("ORG_MEGAPOL");
	auto ally = state->getOrganisation("ORG_MARSEC");
	auto hostile = state->getOrganisation("ORG_CYBERWEB");

	state->current_battle = nullptr;

	// Ally likes 'row' a lot -> positive ripple in the same direction as delta.
	ally->current_relations[row] = 60.0f;
	ally->current_relations[col] = 0.0f;
	// A hostile third party dislikes 'row' -> extra-damped ripple, still same sign as delta.
	hostile->current_relations[row] = -40.0f;
	hostile->current_relations[col] = 0.0f;

	row->current_relations[col] = 0.0f;
	const float delta = 40.0f;
	row->adjustRelationTo(*state, col, delta, true);

	// allyFactor = 60/2 = 30, ripple = 30*40/100 = 12
	if (!nearlyEqual(ally->current_relations[col], 12.0f))
	{
		LogError("Expected allied third party's relation to col to move by +12, got {0}",
		         ally->current_relations[col]);
		return false;
	}

	// allyFactor = -40/2 = -20, damped further (since negative) to -10, ripple = -10*40/100 = -4
	if (!nearlyEqual(hostile->current_relations[col], -4.0f))
	{
		LogError("Expected hostile third party's relation to col to move by -4, got {0}",
		         hostile->current_relations[col]);
		return false;
	}

	if (!nearlyEqual(row->current_relations[col], 40.0f))
	{
		LogError("Expected row's own relation to col to become 40, got {0}",
		         row->current_relations[col]);
		return false;
	}

	LogInfo("Ripple sign and magnitude test passed");
	return true;
}

// The original silently drops a ripple that would cross -100/100, rather than clamping to it.
static bool test_ripple_dropped_not_clamped(sp<GameState> state)
{
	LogInfo("Testing ripple is dropped rather than clamped at the boundary...");

	auto row = state->getOrganisation("ORG_GOVERNMENT");
	auto col = state->getOrganisation("ORG_MEGAPOL");
	auto ally = state->getOrganisation("ORG_MARSEC");

	state->current_battle = nullptr;

	ally->current_relations[row] = 100.0f; // allyFactor = 50
	ally->current_relations[col] = 98.0f;  // 98 + (50*40/100 = 20) = 118, out of range

	row->current_relations[col] = 0.0f;
	row->adjustRelationTo(*state, col, 40.0f, true);

	if (!nearlyEqual(ally->current_relations[col], 98.0f))
	{
		LogError("Expected out-of-range ripple to be dropped, leaving 98, got {0}",
		         ally->current_relations[col]);
		return false;
	}

	// Same check at the negative boundary
	ally->current_relations[row] = -100.0f; // allyFactor = -50, damped to -25
	ally->current_relations[col] = -97.0f;  // -97 + (-25*40/100 = -10) = -107, out of range
	row->current_relations[col] = 0.0f;
	row->adjustRelationTo(*state, col, 40.0f, true);

	if (!nearlyEqual(ally->current_relations[col], -97.0f))
	{
		LogError("Expected out-of-range negative ripple to be dropped, leaving -97, got {0}",
		         ally->current_relations[col]);
		return false;
	}

	LogInfo("Ripple boundary-drop test passed");
	return true;
}

// The ripple loop excludes 'other' (the victim of the adjustment), the same way the original
// excludes its own row - otherwise a victim with getRelationTo(self) == 100 would take a
// spurious extra hit as its own "ally".
static bool test_victim_excluded_from_ripple(sp<GameState> state)
{
	LogInfo("Testing victim is excluded from its own ripple...");

	auto row = state->getOrganisation("ORG_GOVERNMENT");
	auto col = state->getOrganisation("ORG_MEGAPOL");

	state->current_battle = nullptr;

	// If col were not excluded from the loop, it would be processed as its own "third party",
	// computing an ally factor against itself and writing a spurious self-entry.
	col->current_relations.erase(col);
	col->current_relations[row] = 80.0f;
	row->current_relations[col] = 0.0f;

	row->adjustRelationTo(*state, col, 40.0f, true);

	if (col->current_relations.find(col) != col->current_relations.end())
	{
		LogError("Expected victim to be excluded from its own ripple, but a self-entry of {0} "
		         "was written",
		         col->current_relations[col]);
		return false;
	}

	LogInfo("Victim-exclusion test passed");
	return true;
}

// X-COM and the Aliens are always at war, and X-COM's relation to every org mirrors what that
// org thinks of X-COM, whenever any relation touching the player changes.
static bool test_player_mirror_and_hard_war(sp<GameState> state)
{
	LogInfo("Testing X-COM/Alien hard war and player mirror...");

	auto player = state->getPlayer();
	auto aliens = state->getAliens();
	auto other = state->getOrganisation("ORG_MEGAPOL");

	state->current_battle = nullptr;

	// Deliberately break the invariant, then trigger a player-involving change to see it restored
	player->current_relations[aliens] = 3.0f;
	aliens->current_relations[player] = -7.0f;
	other->current_relations[player] = 55.0f;

	other->adjustRelationTo(*state, player, 1.0f);

	if (!nearlyEqual(player->current_relations[aliens], -100.0f) ||
	    !nearlyEqual(aliens->current_relations[player], -100.0f))
	{
		LogError("Expected X-COM/Alien relation pinned at -100 both ways, got {0} / {1}",
		         player->current_relations[aliens], aliens->current_relations[player]);
		return false;
	}

	// other's own relation to player became 55 + 1 = 56; X-COM's mirrored view of it must match
	// that concrete value, not just be self-consistent with whatever getRelationTo() returns.
	if (!nearlyEqual(other->current_relations[player], 56.0f) ||
	    !nearlyEqual(player->current_relations[other], 56.0f))
	{
		LogError("Expected X-COM's relation to org to mirror the org's 56 relation to X-COM, "
		         "got other->player={0} player->other={1}",
		         other->current_relations[player], player->current_relations[other]);
		return false;
	}

	LogInfo("Player mirror and hard war test passed");
	return true;
}

// The X-COM mirror runs unconditionally now, so it must still fire while a battle is in progress
// (the ripple itself stays suppressed mid-battle; this only checks the mirror half).
static bool test_mirror_fires_during_battle(sp<GameState> state)
{
	LogInfo("Testing X-COM mirror still fires mid-battle...");

	auto player = state->getPlayer();
	auto aliens = state->getAliens();
	auto other = state->getOrganisation("ORG_MEGAPOL");

	state->current_battle = mksp<Battle>();

	player->current_relations[aliens] = 3.0f;
	aliens->current_relations[player] = -7.0f;
	other->current_relations[player] = 20.0f;

	other->adjustRelationTo(*state, player, 1.0f, true);

	state->current_battle = nullptr;

	if (!nearlyEqual(player->current_relations[aliens], -100.0f) ||
	    !nearlyEqual(aliens->current_relations[player], -100.0f))
	{
		LogError("Expected X-COM/Alien relation pinned at -100 both ways mid-battle, got {0} / {1}",
		         player->current_relations[aliens], aliens->current_relations[player]);
		return false;
	}

	if (!nearlyEqual(other->current_relations[player], 21.0f) ||
	    !nearlyEqual(player->current_relations[other], 21.0f))
	{
		LogError("Expected X-COM's relation to org to mirror mid-battle, got other->player={0} "
		         "player->other={1}",
		         other->current_relations[player], player->current_relations[other]);
		return false;
	}

	LogInfo("Mid-battle mirror test passed");
	return true;
}

// Without opting in, adjustRelationTo() must move only the direct pair - no third party should
// see any change at all.
static bool test_no_ripple_by_default(sp<GameState> state)
{
	LogInfo("Testing default adjustRelationTo() produces no third-party movement...");

	auto row = state->getOrganisation("ORG_GOVERNMENT");
	auto col = state->getOrganisation("ORG_MEGAPOL");
	auto third = state->getOrganisation("ORG_MARSEC");

	state->current_battle = nullptr;

	third->current_relations[row] = 80.0f;
	third->current_relations[col] = 0.0f;
	row->current_relations[col] = 0.0f;

	row->adjustRelationTo(*state, col, 40.0f);

	if (!nearlyEqual(row->current_relations[col], 40.0f))
	{
		LogError("Expected row's own relation to col to become 40, got {0}",
		         row->current_relations[col]);
		return false;
	}

	if (!nearlyEqual(third->current_relations[col], 0.0f))
	{
		LogError("Expected no ripple to a third party without opting in, got {0}",
		         third->current_relations[col]);
		return false;
	}

	LogInfo("Default no-ripple test passed");
	return true;
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

// Vehicle::adjustRelationshipOnDowned() is the real call site that drives the ripple in play:
// a downed craft's owner takes the direct hit, third parties ripple off it, and (this is the
// player-visible fix) the victim must not double-count itself as its own "ally" the way the
// pre-refactor hand-rolled loop did.
static bool test_vehicle_downed_relation_adjustment(sp<GameState> state)
{
	LogInfo("Testing Vehicle::adjustRelationshipOnDowned drives the shared ripple...");

	auto victimOrg = state->getOrganisation("ORG_GOVERNMENT");
	auto attackerOrg = state->getOrganisation("ORG_MEGAPOL");
	auto thirdHigh = state->getOrganisation("ORG_MARSEC");     // relation 74 to victim
	auto thirdMid = state->getOrganisation("ORG_CYBERWEB");    // relation 50 to victim
	auto thirdZero = state->getOrganisation("ORG_DIABLO");     // relation 0 to victim
	auto thirdNeg = state->getOrganisation("ORG_TRANSTELLAR"); // relation -100 to victim

	state->current_battle = nullptr;

	// Not hostile (isRelatedTo < -50 would be Hostile and take the -5 branch instead); this
	// keeps us on the "lose 30 points" branch that matches the -30 delta the measured numbers
	// below assume.
	victimOrg->current_relations[attackerOrg] = 20.0f;
	attackerOrg->current_relations[victimOrg] = 20.0f;

	thirdHigh->current_relations[victimOrg] = 74.0f;
	thirdHigh->current_relations[attackerOrg] = 0.0f;
	thirdMid->current_relations[victimOrg] = 50.0f;
	thirdMid->current_relations[attackerOrg] = 0.0f;
	thirdZero->current_relations[victimOrg] = 0.0f;
	thirdZero->current_relations[attackerOrg] = 0.0f;
	thirdNeg->current_relations[victimOrg] = -100.0f;
	thirdNeg->current_relations[attackerOrg] = 0.0f;

	// Register real Vehicle objects in state->vehicles so StateRef<Vehicle> resolution (used
	// internally by adjustRelationshipOnDowned() via the `attacker` parameter) finds them
	// normally, instead of relying on a pointer that getId() cannot look up.
	auto victimVehicle = mksp<Vehicle>();
	victimVehicle->owner = victimOrg;
	auto victimVehicleId = Vehicle::generateObjectID(*state);
	state->vehicles[victimVehicleId] = victimVehicle;

	auto attackerVehicle = mksp<Vehicle>();
	attackerVehicle->owner = attackerOrg;
	auto attackerVehicleId = Vehicle::generateObjectID(*state);
	state->vehicles[attackerVehicleId] = attackerVehicle;

	StateRef<Vehicle> attackerRef(state.get(), attackerVehicleId);

	victimVehicle->adjustRelationshipOnDowned(*state, attackerRef);

	state->vehicles.erase(victimVehicleId);
	state->vehicles.erase(attackerVehicleId);

	if (!nearlyEqual(thirdHigh->current_relations[attackerOrg], -11.1f))
	{
		LogError("Expected a third party at relation 74 to the victim to move -11.1 toward the "
		         "attacker, got {0}",
		         thirdHigh->current_relations[attackerOrg]);
		return false;
	}

	if (!nearlyEqual(thirdMid->current_relations[attackerOrg], -7.5f))
	{
		LogError("Expected a third party at relation 50 to the victim to move -7.5 toward the "
		         "attacker, got {0}",
		         thirdMid->current_relations[attackerOrg]);
		return false;
	}

	if (!nearlyEqual(thirdZero->current_relations[attackerOrg], 0.0f))
	{
		LogError("Expected a third party at relation 0 to the victim to stay put, got {0}",
		         thirdZero->current_relations[attackerOrg]);
		return false;
	}

	if (!nearlyEqual(thirdNeg->current_relations[attackerOrg], 7.5f))
	{
		LogError("Expected a third party at relation -100 to the victim to move +7.5 toward "
		         "the attacker, got {0}",
		         thirdNeg->current_relations[attackerOrg]);
		return false;
	}

	// The single most player-visible part of the fix: the victim's own relation to the
	// attacker must move by exactly the requested -30, not -45. Before f13df229 the victim
	// was not excluded from its own ripple loop, and getRelationTo(self) returns 100, so it
	// took an extra -15 hit as its own "ally".
	if (!nearlyEqual(victimOrg->current_relations[attackerOrg], -10.0f))
	{
		LogError("Expected victim's own relation to attacker to move by exactly -30 (20 -> "
		         "-10), got {0}",
		         victimOrg->current_relations[attackerOrg]);
		return false;
	}

	LogInfo("Vehicle downed relation adjustment test passed");
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
	ok &= test_ripple_sign_and_magnitude(state);
	ok &= test_ripple_dropped_not_clamped(state);
	ok &= test_victim_excluded_from_ripple(state);
	ok &= test_player_mirror_and_hard_war(state);
	ok &= test_mirror_fires_during_battle(state);
	ok &= test_no_ripple_by_default(state);
	ok &= test_long_term_event_gated(state);
	ok &= test_daily_delta_unaffected(state);
	ok &= test_vehicle_downed_relation_adjustment(state);

	if (!ok)
	{
		LogError("test_org_relations FAILED");
		return EXIT_FAILURE;
	}

	LogInfo("test_org_relations success - all tests passed");
	return EXIT_SUCCESS;
}

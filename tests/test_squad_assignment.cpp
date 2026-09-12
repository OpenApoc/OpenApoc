#include "framework/configfile.h"
#include "framework/logger.h"
#include "game/state/battle/battleforces.h"
#include "game/state/battle/battleunit.h"
#include "library/sp.h"
#include <vector>

using namespace OpenApoc;

// This exercises BattleForces::insert/insertAt/removeAt directly rather than going through
// BattleUnit::assignToSquad(Battle&, ...): that wrapper just forwards to these same calls
// (battleunit.cpp:169-199), and a real Battle can't be default-constructed outside battle.cpp -
// its defaulted constructor requires several forward-declared member types (BattleScanner,
// BattleDoor, ...) to be complete for exception-safe member unwinding.

// Build a BattleUnit that is not yet assigned to any squad, matching the convention used when
// units first join a battle (see Battle::enterBattle, battle.cpp:1217).
static sp<BattleUnit> makeUnit()
{
	auto unit = mksp<BattleUnit>();
	unit->squadNumber = -1;
	return unit;
}

// Fill squad 0 to capacity and confirm assigning a 7th unit to it is a clean no-op: the new
// unit stays unassigned and the full squad is left untouched.
static bool test_full_squad_is_noop(BattleForces &forces)
{
	LogInfo("Testing that assigning to a full squad is a no-op...");

	std::vector<sp<BattleUnit>> units;
	for (int i = 0; i < 6; i++)
	{
		auto unit = makeUnit();
		if (!forces.insert(0, unit))
		{
			LogError("Unit {0} failed to join squad 0 while it had free space", i);
			return false;
		}
		units.push_back(unit);
	}

	auto &squad0 = forces.squads[0];
	if (squad0.getNumUnits() != 6)
	{
		LogError("Squad 0 should have 6 units, has {0}", squad0.getNumUnits());
		return false;
	}

	auto overflow = makeUnit();
	bool assigned = forces.insert(0, overflow);

	if (assigned)
	{
		LogError("Assigning a 7th unit to a full squad should fail, but succeeded");
		return false;
	}
	if (overflow->squadNumber != -1)
	{
		LogError("Rejected unit's squadNumber should remain -1, is {0}", overflow->squadNumber);
		return false;
	}
	if (squad0.getNumUnits() != 6)
	{
		LogError("Full squad's unit count should be unchanged after a rejected assignment, is {0}",
		         squad0.getNumUnits());
		return false;
	}
	for (int i = 0; i < 6; i++)
	{
		if (units[i]->squadNumber != 0 || (int)units[i]->squadPosition != i)
		{
			LogError("Existing squad member {0} was disturbed by the rejected assignment", i);
			return false;
		}
	}

	LogInfo("Full squad no-op test passed");
	return true;
}

// Reassigning a unit to a different squad must remove it from its previous squad, closing the
// gap left behind, rather than leaving it listed in both.
static bool test_reassignment_removes_from_previous_squad(BattleForces &forces)
{
	LogInfo("Testing that reassignment removes the unit from its previous squad...");

	auto &squad0 = forces.squads[0];
	auto &squad1 = forces.squads[1];
	if (squad0.getNumUnits() != 6)
	{
		LogError("Expected squad 0 to still hold 6 units from the previous test, has {0}",
		         squad0.getNumUnits());
		return false;
	}

	auto movedUnit = squad0.units[2];
	if (!forces.insert(1, movedUnit))
	{
		LogError("Moving a unit into squad 1 should succeed while it has free space");
		return false;
	}

	if (movedUnit->squadNumber != 1 || movedUnit->squadPosition != 0)
	{
		LogError("Moved unit should be squad 1 position 0, is squad {0} position {1}",
		         movedUnit->squadNumber, movedUnit->squadPosition);
		return false;
	}
	if (squad1.getNumUnits() != 1)
	{
		LogError("Squad 1 should now have 1 unit, has {0}", squad1.getNumUnits());
		return false;
	}
	if (squad0.getNumUnits() != 5)
	{
		LogError("Squad 0 should have shrunk to 5 units, has {0}", squad0.getNumUnits());
		return false;
	}
	for (auto &u : squad0.units)
	{
		if (u == movedUnit)
		{
			LogError("Moved unit should no longer be listed in squad 0");
			return false;
		}
	}
	for (unsigned int i = 0; i < squad0.units.size(); i++)
	{
		if (squad0.units[i]->squadPosition != i)
		{
			LogError("Squad 0 position {0} was not renumbered after the removal, has {1}", i,
			         squad0.units[i]->squadPosition);
			return false;
		}
	}

	LogInfo("Reassignment removes from previous squad test passed");
	return true;
}

int main(int argc, char **argv)
{
	if (config().parseOptions(argc, argv))
	{
		return EXIT_FAILURE;
	}

	BattleForces forces;

	if (!test_full_squad_is_noop(forces))
	{
		LogError("Full squad no-op test failed");
		return EXIT_FAILURE;
	}

	if (!test_reassignment_removes_from_previous_squad(forces))
	{
		LogError("Reassignment removes from previous squad test failed");
		return EXIT_FAILURE;
	}

	LogInfo("test_squad_assignment success - all tests passed");
	return EXIT_SUCCESS;
}

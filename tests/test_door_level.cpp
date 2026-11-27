// Manual diagnostic harness for issue #1724: run by hand against a savegame;
// deliberately not add_test'd (it needs the fully extracted data).
//
// Loads a battle save, runs the real-time simulation and reports every door that
// starts opening while no unit is near it on its own level. Units on the level
// below must not open doors above them.
//
// Usage: test_door_level <common gamestate> <battle save> [seconds]
//   e.g. test_door_level data/mods/base/base_gamestate "saves/Civilians Opening Doors Lvl 1.save"
//   120
#include "framework/configfile.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "game/state/battle/battle.h"
#include "game/state/battle/battledoor.h"
#include "game/state/battle/battlemappart.h"
#include "game/state/battle/battleunit.h"
#include "game/state/gamestate.h"
#include "game/state/gametime.h"
#include <cmath>
#include <glm/glm.hpp>
#include <iostream>
#include <limits>
#include <map>
#include <set>

using namespace OpenApoc;

namespace
{

// A unit standing within this horizontal distance of a door tile on the door's
// own level is a legitimate reason for the door to open.
constexpr float NEAR_DISTANCE = 2.0f;

bool unitOccupiesLevel(const BattleUnit &unit, int z)
{
	int unitZ = static_cast<int>(std::floor(unit.position.z));
	return unitZ == z || (unit.isLarge() && unitZ + 1 == z);
}

// Smallest horizontal distance from any live unit to any tile of the door,
// counting only units on the given level (sameLevel) or only units on others.
float nearestUnitDistance(const Battle &battle, const BattleDoor &door, bool sameLevel,
                          int *nearestUnitZ)
{
	float best = std::numeric_limits<float>::max();
	for (auto &mpWeak : door.mapParts)
	{
		auto mp = mpWeak.lock();
		if (!mp)
		{
			continue;
		}
		int doorZ = static_cast<int>(std::floor(mp->position.z));
		Vec2<float> tileCentre = {std::floor(mp->position.x) + 0.5f,
		                          std::floor(mp->position.y) + 0.5f};
		for (auto &u : battle.units)
		{
			auto &unit = *u.second;
			if (unit.isDead() || unit.retreated)
			{
				continue;
			}
			if (unitOccupiesLevel(unit, doorZ) != sameLevel)
			{
				continue;
			}
			float d = glm::length(Vec2<float>{unit.position.x, unit.position.y} - tileCentre);
			if (d < best)
			{
				best = d;
				if (nearestUnitZ)
				{
					*nearestUnitZ = static_cast<int>(std::floor(unit.position.z));
				}
			}
		}
	}
	return best;
}

} // namespace

int main(int argc, char **argv)
{
	config().addPositionalArgument("common", "Common ruleset gamestate to load first");
	config().addPositionalArgument("gamestate", "Battle savegame to load on top of it");
	config().addPositionalArgument("seconds", "Simulated seconds to run (optional, default 120)");
	if (config().parseOptions(argc, argv))
	{
		return EXIT_FAILURE;
	}
	auto commonName = config().getString("common");
	auto gamestateName = config().getString("gamestate");
	auto secondsArg = config().getString("seconds");
	if (commonName.empty() || gamestateName.empty())
	{
		std::cerr << "Must provide common and gamestate\n";
		config().showHelp();
		return EXIT_FAILURE;
	}
	const int seconds = secondsArg.empty() ? 120 : std::stoi(secondsArg);

	Framework fw("OpenApoc", false);

	auto state = mksp<GameState>();
	if (!state->loadGame(commonName) || !state->loadGame(gamestateName))
	{
		LogError("Failed to load gamestate");
		return EXIT_FAILURE;
	}
	if (!state->current_battle)
	{
		LogError("Loaded gamestate has no current_battle");
		return EXIT_FAILURE;
	}
	state->current_battle->initBattle(*state);
	auto &battle = *state->current_battle;

	std::cout << "Loaded battle: " << battle.units.size() << " units, " << battle.doors.size()
	          << " doors, mode "
	          << (battle.mode == Battle::Mode::RealTime ? "RealTime" : "TurnBased") << "\n";

	// One tick per step, like the battle view at normal speed. Larger steps let
	// walking units skip the tile-centre window that door opening checks for.
	const int steps = seconds * static_cast<int>(TICKS_PER_SECOND);
	std::map<UString, Vec3<float>> startPositions;
	for (auto &u : battle.units)
	{
		startPositions[u.first] = u.second->position;
	}
	std::set<UString> opening;
	int openings = 0;
	int spurious = 0;
	for (int step = 0; step < steps; step++)
	{
		state->update(1);
		for (auto &d : battle.doors)
		{
			auto &door = *d.second;
			bool isOpening = !door.open && door.animationTicksRemaining > 0;
			if (!isOpening)
			{
				opening.erase(d.first);
				continue;
			}
			if (!opening.insert(d.first).second)
			{
				continue; // already reported this opening
			}
			openings++;
			float sameLevel = nearestUnitDistance(battle, door, true, nullptr);
			if (sameLevel <= NEAR_DISTANCE)
			{
				continue;
			}
			int otherZ = -1;
			float otherLevel = nearestUnitDistance(battle, door, false, &otherZ);
			spurious++;
			std::cout << "  t=" << (step + 1) / static_cast<float>(TICKS_PER_SECOND) << "s door "
			          << d.first << " at z=" << static_cast<int>(std::floor(door.position.z))
			          << " opened with no unit on its level within " << NEAR_DISTANCE
			          << " tiles (nearest same level " << sameLevel << ", nearest other level "
			          << otherLevel << " at z=" << otherZ << ")\n";
		}
	}

	int movedUnits = 0;
	for (auto &u : battle.units)
	{
		if (glm::length(u.second->position - startPositions[u.first]) > 1.0f)
		{
			movedUnits++;
		}
	}
	std::cout << movedUnits << " of " << battle.units.size() << " units moved more than one tile\n";
	std::cout << "Simulated " << seconds << "s: " << openings << " door openings, " << spurious
	          << " with no unit on the door's level nearby\n";
	// GameState::~GameState() runs Battle::finishBattle() when current_battle is
	// set, which expects the normal city bookkeeping; drop it to skip that.
	state->current_battle = nullptr;
	std::cout << (spurious == 0 ? "PASS\n" : "FAIL\n");
	return spurious == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

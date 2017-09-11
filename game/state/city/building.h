#pragma once

#include "game/state/gametime.h"
#include "game/state/stateobject.h"
#include "library/rect.h"
#include "library/vec.h"
#include <set>
#include <vector>

namespace OpenApoc
{

// In vanilla, 5 buildings were checked every 5 minutes.
// This meant on different difficulty levels it took different time to check every building.
// We round this to nearest 5 and make it decoupled from the amount of buildings
static const unsigned int TICKS_PER_DETECTION_ATTEMPT[5] = {
    70 * TICKS_PER_MINUTE, 80 * TICKS_PER_MINUTE, 85 * TICKS_PER_MINUTE, 95 * TICKS_PER_MINUTE,
    110 * TICKS_PER_MINUTE};
static const unsigned int TICKS_DETECTION_TIMEOUT = 6 * TICKS_PER_HOUR;

class BuildingDef;
class Organisation;
class Vehicle;
class AgentType;
class Base;
class BattleMap;
class BaseLayout;

class BuildingFunction : public StateObject
{
	STATE_OBJECT(BuildingFunction)
  public:
	UString name;
	int infiltrationSpeed = 0;
	int detectionWeight = 0;
};

class Building : public StateObject, public std::enable_shared_from_this<Building>
{
	STATE_OBJECT(Building)
  public:
	UString name;
	StateRef<BuildingFunction> function;
	StateRef<Organisation> owner;
	Rect<int> bounds;
	StateRef<BaseLayout> base_layout;
	StateRef<BattleMap> battle_map;
	std::map<StateRef<AgentType>, int> preset_crew;
	std::map<StateRef<AgentType>, int> current_crew;

	std::vector<Vec3<int>> landingPadLocations;
	std::set<StateRef<Vehicle>> landed_vehicles;

	unsigned ticksDetectionTimeOut = 0;
	unsigned ticksDetectionAttemptAccumulated = 0;
	bool detected = false;

	bool hasAliens() const;
	void updateDetection(GameState &state, unsigned int ticks);
	void detect(GameState &state, bool forced = false);
	void alienGrowth(GameState &state);
};

}; // namespace OpenApoc

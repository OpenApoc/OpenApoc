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
// How frequently building attack events are emitted
static const unsigned int TICKS_ATTACK_EVENT_TIMEOUT = TICKS_PER_HOUR;

class BuildingDef;
class Agent;
class Organisation;
class Vehicle;
class AgentType;
class Base;
class BattleMap;
class BaseLayout;
class City;
class Cargo;
class UfopaediaEntry;
class ResearchTopic;

class BuildingFunction : public StateObject<BuildingFunction>
{
  public:
	UString name;
	int infiltrationSpeed = 0;
	int detectionWeight = 0;
	StateRef<UfopaediaEntry> ufopaedia_entry;
};

class Building : public StateObject<Building>, public std::enable_shared_from_this<Building>
{
  public:
	UString name;
	StateRef<City> city;
	StateRef<BuildingFunction> function;
	StateRef<Organisation> owner;
	Rect<int> bounds;
	StateRef<BaseLayout> base_layout;
	StateRef<Base> base;
	StateRef<BattleMap> battle_map;
	std::map<StateRef<AgentType>, int> preset_crew;
	std::map<StateRef<AgentType>, int> current_crew;

	std::set<StateRef<Vehicle>> currentVehicles;
	std::set<StateRef<Agent>> currentAgents;
	std::list<Cargo> cargo;

	uint64_t timeOfLastAttackEvent = 0;
	unsigned ticksDetectionTimeOut = 0;
	unsigned ticksDetectionAttemptAccumulated = 0;
	bool detected = false;
	int pendingInvestigatorCount = 0;
	// Unlocks when successful at raiding this
	std::list<StateRef<ResearchTopic>> researchUnlock;
	// Access to building
	StateRef<ResearchTopic> accessTopic;
	// Victory when successful at raiding this
	bool victory = false;

	// may fire a 'commence investigation' event
	void decreasePendingInvestigatorCount(GameState &state);
	bool hasAliens() const;
	void updateDetection(GameState &state, unsigned int ticks);
	void updateCargo(GameState &state);
	void detect(GameState &state, bool forced = false);
	void alienGrowth(GameState &state);
	void alienMovement(GameState &state);

	void underAttack(GameState &state, StateRef<Organisation> attacker);

	void collapse(GameState &state);
	void buildingPartChange(GameState &state, Vec3<int> part, bool intact);
	bool isAlive(GameState &state) const;

	// Following members are not serialized, but rather are set in City::initMap method

	Vec3<int> crewQuarters = {-1, -1, -1};
	Vec3<int> carEntranceLocation = {-1, -1, -1};
	std::set<Vec3<int>> landingPadLocations;
	std::set<Vec3<int>> buildingParts;
};

}; // namespace OpenApoc

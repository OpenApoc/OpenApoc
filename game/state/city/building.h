#pragma once

#include "game/state/stateobject.h"
#include "library/rect.h"
#include "library/vec.h"
#include <set>
#include <vector>

namespace OpenApoc
{

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
};

class Building : public StateObject
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
};

}; // namespace OpenApoc

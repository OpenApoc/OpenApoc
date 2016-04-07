#pragma once
#include "game/state/city/baselayout.h"
#include "game/state/city/building.h"
#include "game/state/organisation.h"
#include "library/sp.h"
#include "library/vec.h"

#include <memory>
#include <set>
#include <vector>

namespace OpenApoc
{

class BuildingDef;
class Organisation;
class Vehicle;
class Base;

class Building : public StateObject<Building>
{
  public:
	UString name;
	StateRef<Organisation> owner;
	Rect<int> bounds;
	StateRef<BaseLayout> base_layout;

	std::vector<Vec3<int>> landingPadLocations;
	std::set<StateRef<Vehicle>> landed_vehicles;
};

}; // namespace OpenApoc

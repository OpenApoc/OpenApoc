#pragma once
#include "game/city/baselayout.h"
#include "game/city/building.h"
#include "game/organisation.h"
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

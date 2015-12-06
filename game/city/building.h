#pragma once
#include "library/sp.h"

#include "library/vec.h"

#include <set>
#include <vector>
#include <memory>

namespace OpenApoc
{

class BuildingDef;
class Organisation;
class Framework;
class Vehicle;
class Base;

class Building
{
  public:
	Building(const BuildingDef &def, sp<Organisation> owner);
	const BuildingDef &def;
	sp<Organisation> owner;
	std::vector<Vec3<int>> landingPadLocations;
	std::set<sp<Vehicle>> landed_vehicles;
	sp<Base> base;
};

}; // namespace OpenApoc

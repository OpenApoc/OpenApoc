#pragma once

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
	Building(BuildingDef &def, Organisation &owner);
	BuildingDef &def;
	Organisation &owner;
	std::vector<Vec3<int>> landingPadLocations;
	std::set<std::shared_ptr<Vehicle>> landed_vehicles;
	std::shared_ptr<Base> base;
};

}; // namespace OpenApoc

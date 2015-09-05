#pragma once

#include "library/vec.h"
#include <list>

namespace OpenApoc
{

class Vehicle;
class Tile;

class VehicleMission
{
  public:
	Vehicle &vehicle;
	VehicleMission(Vehicle &vehicle);
	virtual Vec3<float> getNextDestination() = 0;
	virtual ~VehicleMission();
	virtual const std::list<Tile *> &getCurrentPlannedPath() = 0;

	static VehicleMission *randomDestination(Vehicle &v);
};
} // namespace OpenApoc

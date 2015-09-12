#pragma once

#include "library/vec.h"
#include <list>

namespace OpenApoc
{

class Vehicle;
class Tile;
class TileMap;
class Building;

class VehicleMission
{
  public:
	Vehicle &vehicle;
	VehicleMission(Vehicle &vehicle);
	virtual Vec3<float> getNextDestination() = 0;
	virtual ~VehicleMission();
	virtual const std::list<Tile *> &getCurrentPlannedPath() = 0;

	virtual void update(unsigned int ticks) = 0;
	virtual bool isFinished() = 0;
	virtual void start() = 0;

	static VehicleMission *randomDestination(Vehicle &v);
	static VehicleMission *gotoLocation(Vehicle &v, TileMap &map, Vec3<int> target);
	static VehicleMission *gotoBuilding(Vehicle &v, TileMap &map, Building &target);
};
} // namespace OpenApoc

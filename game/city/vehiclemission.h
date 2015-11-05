#pragma once

#include "library/sp.h"
#include "library/vec.h"
#include <list>

namespace OpenApoc
{

class Vehicle;
class Tile;
class TileMap;
class Building;
class UString;

class VehicleMission
{
  public:
	Vehicle &vehicle;
	VehicleMission(Vehicle &vehicle);
	virtual bool getNextDestination(Vec3<float> &dest) = 0;
	virtual ~VehicleMission();
	virtual const std::list<Tile *> &getCurrentPlannedPath() = 0;

	virtual void update(unsigned int ticks) = 0;
	virtual bool isFinished() = 0;
	virtual void start() = 0;

	static VehicleMission *randomDestination(Vehicle &v, TileMap &map);
	static VehicleMission *gotoLocation(Vehicle &v, TileMap &map, Vec3<int> target);
	static VehicleMission *gotoBuilding(Vehicle &v, TileMap &map, sp<Building> target);
	static VehicleMission *snooze(Vehicle &v, TileMap &map, unsigned int ticks);

	// INTERNAL: Not to be used directly (Only works when in building)
	static VehicleMission *takeOff(Vehicle &v, TileMap &map);
	// INTERNAL: Not to be used directly (Only works if directly above a pad)
	static VehicleMission *land(Vehicle &v, TileMap &map, sp<Building> b);

	virtual const UString &getName() = 0;
};
} // namespace OpenApoc

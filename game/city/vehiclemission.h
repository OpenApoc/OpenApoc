#pragma once

#include "library/sp.h"
#include "library/vec.h"
#include "game/city/building.h"
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
	VehicleMission();
	bool getNextDestination(Vec3<float> &dest);

	void update(unsigned int ticks);
	bool isFinished();
	void start();

	static VehicleMission *gotoLocation(Vehicle &v, Vec3<int> target);
	static VehicleMission *gotoBuilding(Vehicle &v, StateRef<Building> target);
	static VehicleMission *snooze(Vehicle &v, unsigned int ticks);

	// INTERNAL: Not to be used directly (Only works when in building)
	static VehicleMission *takeOff(Vehicle &v);
	// INTERNAL: Not to be used directly (Only works if directly above a pad)
	static VehicleMission *land(Vehicle &v, StateRef<Building> b);

	UString getName();

	enum MissionType
	{
		GotoLocation,
		GotoBuilding,
		FollowVehicle,
		AttackVehicle,
		AttackBuilding,
		Snooze,
		TakeOff,
		Land,
	};
	static const std::map<MissionType, UString> TypeMap;

	MissionType type;

	Vec3<int> targetLocation;
	StateRef<Building> targetBuilding;
	StateRef<Vehicle> targetVehicle;
	unsigned int snoozeTime;

	std::list<Vec3<int>> currentPlannedPath;
};
} // namespace OpenApoc

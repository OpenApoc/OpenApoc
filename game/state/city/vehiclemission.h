#pragma once

#include "game/state/city/building.h"
#include "library/strings.h"
#include "library/vec.h"
#include <list>
#include <map>

namespace OpenApoc
{

class Vehicle;
class Tile;
class TileMap;
class Building;
class UString;

class VehicleMission
{
  private:
	// INTERNAL: Not to be used directly (Only works when in building)
	static VehicleMission *takeOff(Vehicle &v);
	// INTERNAL: Not to be used directly (Only works if directly above a pad)
	static VehicleMission *land(Vehicle &v, StateRef<Building> b);

	bool takeOffCheck(GameState &state, Vehicle &v, UString mission);

  public:
	VehicleMission();

	// Methods used in pathfinding etc.
	bool getNextDestination(GameState &state, Vehicle &v, Vec3<float> &dest);
	void update(GameState &state, Vehicle &v, unsigned int ticks);
	bool isFinished(GameState &state, Vehicle &v);
	void start(GameState &state, Vehicle &v);
	void setPathTo(Vehicle &v, Vec3<int> target, int maxIterations = 500);
	bool advanceAlongPath(Vec3<float> &dest);

	// Methods to create new missions
	static VehicleMission *gotoLocation(Vehicle &v, Vec3<int> target);
	static VehicleMission *gotoPortal(Vehicle &v);
	static VehicleMission *gotoPortal(Vehicle &v, Vec3<int> target);
	static VehicleMission *gotoBuilding(Vehicle &v, StateRef<Building> target);
	static VehicleMission *infiltrateBuilding(Vehicle &v, StateRef<Building> target);
	static VehicleMission *attackVehicle(Vehicle &v, StateRef<Vehicle> target);
	static VehicleMission *followVehicle(Vehicle &v, StateRef<Vehicle> target);
	static VehicleMission *snooze(Vehicle &v, unsigned int ticks);
	static VehicleMission *crashLand(Vehicle &v);
	static VehicleMission *patrol(Vehicle &v);

	UString getName();

	enum class MissionType
	{
		GotoLocation,
		GotoBuilding,
		FollowVehicle,
		AttackVehicle,
		AttackBuilding,
		Snooze,
		TakeOff,
		Land,
		Crash,
		Patrol,
		GotoPortal,
		Infiltrate,
		Subvert
	};
	static const std::map<MissionType, UString> TypeMap;

	MissionType type;

	// GotoLocation TakeOff GotoPortal Patrol
	Vec3<int> targetLocation;
	// GotoBuilding AttackBuilding Land Infiltrate
	StateRef<Building> targetBuilding;
	// FollowVehicle AttackVehicle
	StateRef<Vehicle> targetVehicle;
	// Snooze
	unsigned int timeToSnooze;
	// Patrol: waypoints
	unsigned int missionCounter;

	std::list<Vec3<int>> currentPlannedPath;
};
} // namespace OpenApoc

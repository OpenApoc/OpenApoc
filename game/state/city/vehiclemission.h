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
	// INTERNAL: This checks if mission is actually finished. Called by isFinished.
	// If it is finished, update() is called by isFinished so that any remaining work could be done
	bool isFinishedInternal(GameState &state, Vehicle &v);

	bool takeOffCheck(GameState &state, Vehicle &v, UString mission);

  public:
	VehicleMission() = default;

	// Methods used in pathfinding etc.
	bool getNextDestination(GameState &state, Vehicle &v, Vec3<float> &dest);
	void update(GameState &state, Vehicle &v, unsigned int ticks, bool finished = false);
	bool isFinished(GameState &state, Vehicle &v, bool callUpdateIfFinished = true);
	void start(GameState &state, Vehicle &v);
	void setPathTo(GameState &state, Vehicle &v, Vec3<int> target, int maxIterations = 500,
	               bool checkValidity = true);
	bool advanceAlongPath(GameState &state, Vec3<float> &dest, Vehicle &v);
	bool isTakingOff(Vehicle &v);

	// Methods to create new missions
	static VehicleMission *gotoLocation(GameState &state, Vehicle &v, Vec3<int> target,
	                                    bool pickNearest = false);
	static VehicleMission *gotoPortal(GameState &state, Vehicle &v);
	static VehicleMission *gotoPortal(GameState &state, Vehicle &v, Vec3<int> target);
	static VehicleMission *gotoBuilding(GameState &state, Vehicle &v, StateRef<Building> target);
	static VehicleMission *infiltrateOrSubvertBuilding(GameState &state, Vehicle &v,
	                                                   StateRef<Building> target,
	                                                   bool subvert = false);
	static VehicleMission *attackVehicle(GameState &state, Vehicle &v, StateRef<Vehicle> target);
	static VehicleMission *followVehicle(GameState &state, Vehicle &v, StateRef<Vehicle> target);
	static VehicleMission *snooze(GameState &state, Vehicle &v, unsigned int ticks);
	static VehicleMission *restartNextMission(GameState &state, Vehicle &v);
	static VehicleMission *crashLand(GameState &state, Vehicle &v);
	static VehicleMission *patrol(GameState &state, Vehicle &v, unsigned int counter = 10);

	UString getName();

	enum class MissionType
	{
		GotoLocation,
		GotoBuilding,
		FollowVehicle,
		AttackVehicle,
		AttackBuilding,
		RestartNextMission,
		Snooze,
		TakeOff,
		Land,
		Crash,
		Patrol,
		GotoPortal,
		InfiltrateSubvert,
	};
	static const std::map<MissionType, UString> TypeMap;

	MissionType type = MissionType::GotoLocation;

	// GotoLocation InfiltrateSubvert TakeOff GotoPortal Patrol
	Vec3<int> targetLocation = {0, 0, 0};
	// GotoLocation - should it pick nearest point or random point if destination unreachable
	bool pickNearest = false;
	// GotoBuilding AttackBuilding Land Infiltrate
	StateRef<Building> targetBuilding;
	// FollowVehicle AttackVehicle
	StateRef<Vehicle> targetVehicle;
	// Snooze
	unsigned int timeToSnooze = 0;
	// InfiltrateSubvert, Patrol: waypoints
	unsigned int missionCounter = 0;
	// InfiltrateSubvert: mode
	bool subvert = false;

	std::list<Vec3<int>> currentPlannedPath;
};
} // namespace OpenApoc

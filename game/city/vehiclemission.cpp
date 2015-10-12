#include "game/city/vehiclemission.h"
#include "game/tileview/tile.h"
#include "game/city/vehicle.h"
#include "game/city/building.h"
#include "game/rules/buildingdef.h"
#include "framework/logger.h"
#include "game/tileview/tile.h"
#include "game/city/buildingtile.h"

#include <random>

namespace
{
std::default_random_engine rng;

} // anonymous namespace

namespace OpenApoc
{

static bool vehicleCanEnterTile(const Tile &t, const Vehicle &v)
{
	if (t.collideableObjects.empty())
		return true;
	if (t.collideableObjects.size() == 1)
	{
		for (auto &obj : t.collideableObjects)
		{
			if (obj != v.tileObject.lock())
				return false;
		}
	}
	return false;
}

static bool vehicleCanEnterTileAllowLandingPads(const Tile &t, const Vehicle &v)
{
	if (t.collideableObjects.empty())
		return true;
	for (auto &obj : t.collideableObjects)
	{
		if (obj == v.tileObject.lock())
			continue;

		auto buildingTileObject = std::dynamic_pointer_cast<BuildingTile>(obj);
		if (buildingTileObject && buildingTileObject->tileDef.getIsLandingPad())
			continue;

		return false;
	}
	return true;
}

class VehicleRandomDestination : public VehicleMission
{
  public:
	TileMap &map;
	std::uniform_int_distribution<int> xydistribution;
	std::uniform_int_distribution<int> zdistribution;
	std::list<Tile *> path;
	UString name;
	VehicleRandomDestination(Vehicle &v, TileMap &map)
	    : VehicleMission(v), map(map), xydistribution(0, 99), zdistribution(0, 9)
	{
		name = "Random destination";
	}
	virtual bool getNextDestination(Vec3<float> &dest) override
	{
		std::ignore = dest;
		static Vec3<float> invalidDestination{0, 0, 0};
		LogError("Should never be called");
		return false;
	}
	virtual const std::list<Tile *> &getCurrentPlannedPath() override { return path; }
	virtual void start() override
	{
		Vec3<int> newTarget = {xydistribution(rng), xydistribution(rng), zdistribution(rng)};
		while (!map.getTile(newTarget)->ownedObjects.empty())
		{
			newTarget = {xydistribution(rng), xydistribution(rng), zdistribution(rng)};
		}
		auto *gotoMission = VehicleMission::gotoLocation(vehicle, map, newTarget);
		vehicle.missions.emplace_front(gotoMission);
		gotoMission->start();
	}
	virtual bool isFinished() override { return false; }
	virtual void update(unsigned int ticks) override { std::ignore = ticks; }
	virtual const UString &getName() override { return name; }
};

class VehicleIdleMission : public VehicleMission
{
  public:
	UString name;
	static std::list<Tile *> noPath;
	unsigned int idleTicks;
	VehicleIdleMission(Vehicle &v, unsigned int idleTicks) : VehicleMission(v), idleTicks(idleTicks)
	{
		name = "Idle for " + Strings::FromInteger(idleTicks) + "ticks";
	}
	virtual const std::list<Tile *> &getCurrentPlannedPath() override { return noPath; }
	virtual void start() override {}
	virtual bool isFinished() override { return idleTicks == 0; }
	virtual ~VehicleIdleMission() = default;
	virtual void update(unsigned int ticks) override
	{
		if (ticks >= idleTicks)
			idleTicks = 0;
		else
			idleTicks -= ticks;
	}
	virtual bool getNextDestination(Vec3<float> &dest) override
	{
		dest = {0, 0, 0};
		return false;
	}
	virtual const UString &getName() override { return name; }
};

class VehicleTakeOffMission : public VehicleMission
{
  public:
	UString name;
	std::list<Tile *> path;
	TileMap &map;
	Building &b;

	VehicleTakeOffMission(Vehicle &v, TileMap &map, Building &b) : VehicleMission(v), map(map), b(b)
	{
		name = "Take off from building " + b.def.getName();
	}
	virtual const std::list<Tile *> &getCurrentPlannedPath() override { return path; }
	virtual void start() override {}
	virtual bool isFinished() override { return (vehicle.tileObject.lock() && path.empty()); }
	virtual ~VehicleTakeOffMission() = default;
	virtual void update(unsigned int ticks) override
	{
		std::ignore = ticks;
		if (vehicle.tileObject.lock())
		{
			// We're already on our way
			return;
		}
		for (auto padLocation : b.landingPadLocations)
		{
			auto padTile = map.getTile(padLocation);
			auto abovePadLocation = padLocation;
			abovePadLocation.z += 1;
			auto tileAbovePad = map.getTile(abovePadLocation);
			if (!padTile || !tileAbovePad)
			{
				LogError("Invalid landing pad location {%d,%d,%d} - outside map?", padLocation.x,
				         padLocation.y, padLocation.z);
				continue;
			}
			if (!vehicleCanEnterTileAllowLandingPads(*padTile, vehicle))
				continue;
			if (!vehicleCanEnterTileAllowLandingPads(*tileAbovePad, vehicle))
				continue;
			LogInfo("Launching vehicle from building \"%s\" at pad {%d,%d,%d}",
			        b.def.getName().c_str(), padLocation.x, padLocation.y, padLocation.z);
			path = {padTile, tileAbovePad};
			vehicle.launch(map, padLocation);
			return;
		}
		LogWarning("No pad in building \"%s\" free - waiting", b.def.getName().c_str());
	}
	virtual bool getNextDestination(Vec3<float> &dest) override
	{
		if (path.empty())
			return false;
		Tile *nextTile = path.front();
		path.pop_front();
		dest = Vec3<float>{nextTile->position.x, nextTile->position.y, nextTile->position.z}
		       // Add {0.5,0.5,0.5} to make it route to the center of the tile
		       + Vec3<float>{0.5, 0.5, 0.5};
		return true;
	}
	virtual const UString &getName() override { return name; }
};

class VehicleLandMission : public VehicleMission
{
  public:
	UString name;
	std::list<Tile *> path;
	TileMap &map;
	Building &b;

	VehicleLandMission(Vehicle &v, TileMap &map, Building &b) : VehicleMission(v), map(map), b(b)
	{
		name = "Land in building " + b.def.getName();
	}
	virtual const std::list<Tile *> &getCurrentPlannedPath() override { return path; }
	virtual void start() override
	{
		auto vehicleTile = vehicle.tileObject.lock();
		if (!vehicleTile)
		{
			LogError("Trying to land vehicle not in the air?");
			return;
		}
		auto padPosition = vehicleTile->getOwningTile()->position;
		if (padPosition.z < 1)
		{
			LogError("Vehicle trying to land off bottom of map {%d,%d,%d}", padPosition.x,
			         padPosition.y, padPosition.z);
			return;
		}
		padPosition.z -= 1;

		bool padFound = false;

		for (auto &landingPadPos : b.landingPadLocations)
		{
			if (landingPadPos == padPosition)
			{
				padFound = true;
				break;
			}
		}
		if (!padFound)
		{
			LogError("Vehicle at {%d,%d,%d} not directly above a landing pad for building %s",
			         padPosition.x, padPosition.y, padPosition.z + 1, b.def.getName().c_str());
			return;
		}
		path = {map.getTile(padPosition)};
	}
	virtual bool isFinished() override
	{
		if (path.empty())
		{
			/* FIXME: Overloading isFinished() to complete landing action
			 * (Should add a ->end() call to mirror ->start()?*/
			vehicle.land(map, b);
			LogInfo("Vehicle mission %s: Landed", name.c_str());
			return true;
		}
		return false;
	}
	virtual ~VehicleLandMission() = default;
	virtual void update(unsigned int ticks) override { std::ignore = ticks; }
	virtual bool getNextDestination(Vec3<float> &dest) override
	{
		if (path.empty())
			return false;
		Tile *nextTile = path.front();
		path.pop_front();
		dest = Vec3<float>{nextTile->position.x, nextTile->position.y, nextTile->position.z}
		       // Add {0.5,0.5,0.5} to make it route to the center of the tile
		       + Vec3<float>{0.5, 0.5, 0.5};
		return true;
	}
	virtual const UString &getName() override { return name; }
};

class VehicleGotoLocationMission : public VehicleMission
{
  public:
	UString name;
	std::list<Tile *> path;
	TileMap &map;
	Vec3<int> target;

	VehicleGotoLocationMission(Vehicle &v, TileMap &map, Vec3<int> target)
	    : VehicleMission(v), map(map), target(target)
	{
		name = "Goto location {" + Strings::FromInteger(target.x) + "," +
		       Strings::FromInteger(target.y) + "," + Strings::FromInteger(target.z) + "}";
	}
	virtual const std::list<Tile *> &getCurrentPlannedPath() override { return path; }
	virtual void start() override
	{
		auto vehicleTile = vehicle.tileObject.lock();
		if (!vehicleTile)
		{
			LogInfo("Mission %s: Taking off first", this->name.c_str());
			auto *takeoffMission = VehicleMission::takeOff(vehicle, map);
			vehicle.missions.emplace_front(takeoffMission);
			takeoffMission->start();
		}
		else
		{
			path = map.findShortestPath(vehicleTile->getOwningTile()->position, target, 500,
			                            vehicle, vehicleCanEnterTile);
		}
	}
	virtual bool isFinished() override { return (path.empty()); }
	virtual ~VehicleGotoLocationMission() = default;
	virtual void update(unsigned int ticks) override { std::ignore = ticks; }
	virtual bool getNextDestination(Vec3<float> &dest) override
	{
		if (path.empty())
			return false;
		Tile *nextTile = path.front();
		path.pop_front();
		dest = Vec3<float>{nextTile->position.x, nextTile->position.y, nextTile->position.z}
		       // Add {0.5,0.5,0.5} to make it route to the center of the tile
		       + Vec3<float>{0.5, 0.5, 0.5};
		return true;
	}
	virtual const UString &getName() override { return name; }
};

class VehicleGotoBuildingMission : public VehicleMission
{
  public:
	UString name;
	TileMap &map;
	Building &b;

	std::list<Tile> fakePath;

	VehicleGotoBuildingMission(Vehicle &v, TileMap &map, Building &b)
	    : VehicleMission(v), map(map), b(b)
	{
		name = "Goto building " + b.def.getName();
	}
	virtual const std::list<Tile *> &getCurrentPlannedPath() override
	{
		static std::list<Tile *> invalidPath{};
		return invalidPath;
	}
	virtual void start() override
	{
		LogInfo("Vehicle mission %s checking state", name.c_str());
		if (&b == vehicle.building)
		{
			LogInfo("Vehicle mission %s: Already at building", name.c_str());
			return;
		}
		auto vehicleTile = vehicle.tileObject.lock();
		if (!vehicleTile)
		{
			LogInfo("Mission %s: Taking off first", this->name.c_str());
			auto *takeoffMission = VehicleMission::takeOff(vehicle, map);
			vehicle.missions.emplace_front(takeoffMission);
			takeoffMission->start();
			return;
		}
		/* Am I already above a landing pad? If so land */
		auto position = vehicleTile->getOwningTile()->position;
		LogInfo("Vehicle mission %s: at position {%d,%d,%d}", name.c_str(), position.x, position.y,
		        position.z);
		for (auto padLocation : b.landingPadLocations)
		{
			padLocation.z += 1;
			if (padLocation == position)
			{
				LogInfo("Mission %s: Landing on pad {%d,%d,%d}", this->name.c_str(), padLocation.x,
				        padLocation.y, padLocation.z - 1);
				auto *landMission = VehicleMission::land(vehicle, map, b);
				vehicle.missions.emplace_front(landMission);
				landMission->start();
				return;
			}
		}
		/* I must be in the air and not above a pad - try to find the shortest path to a pad
		 * (if no successfull paths then choose the incomplete path with the lowest (cost + distance
		 * to goal)*/
		Vec3<int> shortestPathPad;
		float shortestPathCost = std::numeric_limits<float>::max();
		Vec3<int> closestIncompletePathPad;
		float closestIncompletePathCost = std::numeric_limits<float>::max();

		Vec3<int> target;

		for (auto dest : b.landingPadLocations)
		{
			dest.z += 1; // we want to route to the tile above the pad
			auto currentPath =
			    map.findShortestPath(position, dest, 500, vehicle, vehicleCanEnterTile);
			Vec3<int> pathEnd = currentPath.back()->position;
			if (pathEnd == dest)
			{
				// complete path
				float pathCost = currentPath.size();
				if (shortestPathCost > pathCost)
				{
					shortestPathCost = pathCost;
					shortestPathPad = pathEnd;
				}
			}
			else
			{
				// partial path
				float pathCost = currentPath.size();
				pathCost +=
				    glm::length(Vec3<float>{currentPath.back()->position} - Vec3<float>{dest});
				if (closestIncompletePathCost > pathCost)
				{
					closestIncompletePathCost = pathCost;
					closestIncompletePathPad = pathEnd;
				}
			}
		}

		if (shortestPathCost != std::numeric_limits<float>::max())
		{
			LogInfo("Vehicle mission %s: Found direct path to {%d,%d,%d}", name.c_str(),
			        shortestPathPad.x, shortestPathPad.y, shortestPathPad.z);
			auto *gotoMission = VehicleMission::gotoLocation(vehicle, map, shortestPathPad);
			vehicle.missions.emplace_front(gotoMission);
			gotoMission->start();
		}
		else
		{
			LogInfo("Vehicle mission %s: Found no direct path - closest {%d,%d,%d}", name.c_str(),
			        closestIncompletePathPad.x, closestIncompletePathPad.y,
			        closestIncompletePathPad.z);
			auto *gotoMission =
			    VehicleMission::gotoLocation(vehicle, map, closestIncompletePathPad);
			vehicle.missions.emplace_front(gotoMission);
			gotoMission->start();
		}
	}
	virtual bool isFinished() override
	{
		if (vehicle.building == &b)
		{
			LogInfo("Vehicle mission %s: Finished", name.c_str());
			return true;
		}
		else
		{
			return false;
		}
	}
	virtual ~VehicleGotoBuildingMission() = default;
	virtual void update(unsigned int ticks) override { std::ignore = ticks; }
	virtual bool getNextDestination(Vec3<float> &dest) override
	{
		std::ignore = dest;
		if (vehicle.building != &b)
		{
			LogWarning("Should never be unless already landed");
		}
		dest = {0, 0, 0};
		return true;
	}
	virtual const UString &getName() override { return name; }
};

VehicleMission::VehicleMission(Vehicle &v) : vehicle(v) {}

VehicleMission::~VehicleMission() {}

VehicleMission *VehicleMission::randomDestination(Vehicle &v, TileMap &map)
{
	return new VehicleRandomDestination(v, map);
}

VehicleMission *VehicleMission::gotoLocation(Vehicle &v, TileMap &map, Vec3<int> target)
{
	// TODO
	// Pseudocode:
	// if (in building)
	// 	prepend(TakeOff)
	// routeClosestICanTo(target);
	return new VehicleGotoLocationMission(v, map, target);
}

VehicleMission *VehicleMission::gotoBuilding(Vehicle &v, TileMap &map, Building &target)
{
	// TODO
	// Pseudocode:
	// if (in building)
	// 	queue(TakeOff)
	// while (!above pad) {
	//   foreach(pad at target) {
	//     routes.append(findRouteTo(above pad))
	//   }
	//   if (at least one route ends above pad)
	//     queue(gotoLocation(lowest cost of routes where end == above a pad))
	//   else
	//     queue(gotoLocation(lowest cost of routes + estimated distance to closest pad))
	//  }
	//  queue(Land)
	return new VehicleGotoBuildingMission(v, map, target);
}

VehicleMission *VehicleMission::takeOff(Vehicle &v, TileMap &map)
{
	if (!v.building)
	{
		LogError("Trying to take off while not in a building");
		return nullptr;
	}
	return new VehicleTakeOffMission(v, map, *v.building);
}

VehicleMission *VehicleMission::land(Vehicle &v, TileMap &map, Building &b)
{
	return new VehicleLandMission(v, map, b);
}
} // namespace OpenApoc

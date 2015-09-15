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
			        b.def.getName().str().c_str(), padLocation.x, padLocation.y, padLocation.z);
			path = {padTile, tileAbovePad};
			vehicle.launch(map, padLocation);
			return;
		}
		LogWarning("No pad in building \"%s\" free - waiting", b.def.getName().str().c_str());
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
			LogInfo("Mission %s: Taking off first", this->name.str().c_str());
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
	return nullptr;
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

} // namespace OpenApoc

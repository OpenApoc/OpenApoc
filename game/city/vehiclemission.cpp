#include "game/city/vehiclemission.h"
#include "game/tileview/tile.h"
#include "game/city/vehicle.h"
#include "game/city/building.h"
#include "game/city/scenery.h"
#include "game/rules/buildingdef.h"
#include "framework/logger.h"
#include "game/tileview/tileobject_vehicle.h"
#include "game/tileview/tileobject_scenery.h"
#include "game/rules/scenerytiledef.h"

#include <random>

namespace
{
std::default_random_engine rng;

} // anonymous namespace

namespace OpenApoc
{

class FlyingVehicleCanEnterTileHelper : public CanEnterTileHelper
{
  private:
	TileMap &map;
	Vehicle &v;

  public:
	FlyingVehicleCanEnterTileHelper(TileMap &map, Vehicle &v) : map(map), v(v) {}
	// Support 'from' being nullptr for if a vehicle is being spawned in the map
	bool canEnterTile(Tile *from, Tile *to) const override
	{

		Vec3<int> fromPos = {0, 0, 0};
		if (from)
		{
			fromPos = from->position;
		}
		if (!to)
		{
			LogError("To 'to' position supplied");
			return false;
		}
		Vec3<int> toPos = to->position;
		if (fromPos == toPos)
		{
			LogError("FromPos == ToPos {%d,%d,%d}", toPos.x, toPos.y, toPos.z);
			return false;
		}
		for (auto obj : to->ownedObjects)
		{
			if (obj->getType() == TileObject::Type::Vehicle)
				return false;
			if (obj->getType() == TileObject::Type::Scenery)
			{
				auto sceneryTile = std::static_pointer_cast<TileObjectScenery>(obj);
				if (sceneryTile->scenery.lock()->tileDef.getIsLandingPad())
				{
					continue;
				}
				return false;
			}
		}
		std::ignore = v;
		std::ignore = map;
		// TODO: Try to block diagonal paths clipping past scenery:
		//
		// IE in a 2x2 'flat' case:
		// 'f' = origin tile, 's' = scenery', 't' = target
		//-------
		// +-+
		// |s| t
		// +-+-+
		// f |s|
		//   +-+
		//-------
		// we clearly should disallow moving from v->t despite them being 'adjacent' and empty
		// themselves
		// TODO: Is this then OK for vehicles? as 'most' don't fill the tile?
		// TODO: Can fix the above be fixed by restricting the 'bounds' to the actual voxel map,
		// instead of a while tile? Then comparing against 'intersectingTiles' vehicle objects?

		// FIXME: Handle 'large' vehicles interacting more than with just the 'owned' objects of a
		// single tile?
		return true;
	}
};

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

std::list<Tile *> VehicleIdleMission::noPath;

class VehicleTakeOffMission : public VehicleMission
{
  public:
	UString name;
	std::list<Tile *> path;
	TileMap &map;
	std::weak_ptr<Building> bld;

	VehicleTakeOffMission(Vehicle &v, TileMap &map, sp<Building> b)
	    : VehicleMission(v), map(map), bld(b)
	{
		name = "Take off from building " + b->def.getName();
	}
	virtual const std::list<Tile *> &getCurrentPlannedPath() override { return path; }
	virtual void start() override {}
	virtual bool isFinished() override { return (vehicle.tileObject && path.empty()); }
	virtual ~VehicleTakeOffMission() = default;
	virtual void update(unsigned int ticks) override
	{
		std::ignore = ticks;
		auto b = bld.lock();
		if (!b)
		{
			LogError("Building disappeared");
			return;
		}
		if (vehicle.tileObject)
		{
			// We're already on our way
			return;
		}
		for (auto padLocation : b->landingPadLocations)
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
			FlyingVehicleCanEnterTileHelper canEnterTileHelper(map, vehicle);
			if (!canEnterTileHelper.canEnterTile(nullptr, padTile) ||
			    !canEnterTileHelper.canEnterTile(padTile, tileAbovePad))
				continue;
			LogInfo("Launching vehicle from building \"%s\" at pad {%d,%d,%d}",
			        b->def.getName().c_str(), padLocation.x, padLocation.y, padLocation.z);
			path = {padTile, tileAbovePad};
			vehicle.launch(map, padLocation);
			return;
		}
		LogInfo("No pad in building \"%s\" free - waiting", b->def.getName().c_str());
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
	std::weak_ptr<Building> bld;

	VehicleLandMission(Vehicle &v, TileMap &map, sp<Building> b)
	    : VehicleMission(v), map(map), bld(b)
	{
		name = "Land in building " + b->def.getName();
	}
	virtual const std::list<Tile *> &getCurrentPlannedPath() override { return path; }
	virtual void start() override
	{
		auto b = bld.lock();
		if (!b)
		{
			LogError("Building disappeared");
			return;
		}
		auto vehicleTile = vehicle.tileObject;
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

		for (auto &landingPadPos : b->landingPadLocations)
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
			         padPosition.x, padPosition.y, padPosition.z + 1, b->def.getName().c_str());
			return;
		}
		path = {map.getTile(padPosition)};
	}
	virtual bool isFinished() override
	{
		auto b = bld.lock();
		if (!b)
		{
			LogError("Building disappeared");
			return true;
		}
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
		auto vehicleTile = vehicle.tileObject;
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
			                            FlyingVehicleCanEnterTileHelper{map, vehicle});
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
	std::weak_ptr<Building> bld;

	std::list<Tile> fakePath;

	VehicleGotoBuildingMission(Vehicle &v, TileMap &map, sp<Building> b)
	    : VehicleMission(v), map(map), bld(b)
	{
		name = "Goto building " + b->def.getName();
	}
	virtual const std::list<Tile *> &getCurrentPlannedPath() override
	{
		static std::list<Tile *> invalidPath{};
		return invalidPath;
	}
	virtual void start() override
	{
		LogInfo("Vehicle mission %s checking state", name.c_str());
		auto b = bld.lock();
		if (!b)
		{
			LogError("Building disappeared");
			return;
		}
		if (b == vehicle.building.lock())
		{
			LogInfo("Vehicle mission %s: Already at building", name.c_str());
			return;
		}
		auto vehicleTile = vehicle.tileObject;
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
		for (auto padLocation : b->landingPadLocations)
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

		for (auto dest : b->landingPadLocations)
		{
			dest.z += 1; // we want to route to the tile above the pad
			auto currentPath = map.findShortestPath(position, dest, 500,
			                                        FlyingVehicleCanEnterTileHelper{map, vehicle});

			if (currentPath.size() == 0)
			{
				// If the routing failed to find even a single tile skip it
				continue;
			}
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
		else if (closestIncompletePathCost != std::numeric_limits<float>::max())
		{
			LogInfo("Vehicle mission %s: Found no direct path - closest {%d,%d,%d}", name.c_str(),
			        closestIncompletePathPad.x, closestIncompletePathPad.y,
			        closestIncompletePathPad.z);
			auto *gotoMission =
			    VehicleMission::gotoLocation(vehicle, map, closestIncompletePathPad);
			vehicle.missions.emplace_front(gotoMission);
			gotoMission->start();
		}
		else
		{
			unsigned int snoozeTime = 10;
			LogWarning("Vehicle mission %s: No partial paths found, snoozing for %u", name.c_str(),
			           snoozeTime);
			auto *snoozeMission = VehicleMission::snooze(vehicle, map, snoozeTime);
			vehicle.missions.emplace_front(snoozeMission);
			snoozeMission->start();
		}
	}
	virtual bool isFinished() override
	{
		auto b = bld.lock();
		if (!b)
		{
			LogError("Building disappeared");
			return true;
		}
		if (vehicle.building.lock() == b)
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
		auto b = bld.lock();
		if (!b)
		{
			LogError("Building disappered");
		}
		if (vehicle.building.lock() != b)
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

VehicleMission *VehicleMission::gotoBuilding(Vehicle &v, TileMap &map, sp<Building> target)
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

VehicleMission *VehicleMission::snooze(Vehicle &v, TileMap &map, unsigned int snoozeTicks)
{
	std::ignore = map;
	return new VehicleIdleMission(v, snoozeTicks);
}

VehicleMission *VehicleMission::takeOff(Vehicle &v, TileMap &map)
{
	auto bld = v.building.lock();
	if (!bld)
	{
		LogError("Trying to take off while not in a building");
		return nullptr;
	}
	return new VehicleTakeOffMission(v, map, bld);
}

VehicleMission *VehicleMission::land(Vehicle &v, TileMap &map, sp<Building> b)
{
	return new VehicleLandMission(v, map, b);
}
} // namespace OpenApoc

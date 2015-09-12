#include "game/city/vehiclemission.h"
#include "game/tileview/tile.h"
#include "game/city/vehicle.h"
#include "framework/logger.h"
#include "game/tileview/tile.h"

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

class VehicleRandomDestination : public VehicleMission
{
  public:
	std::uniform_int_distribution<int> xydistribution;
	std::uniform_int_distribution<int> zdistribution;
	VehicleRandomDestination(Vehicle &v)
	    : VehicleMission(v), xydistribution(0, 99), zdistribution(0, 9){};
	std::list<Tile *> path;
	virtual bool getNextDestination(Vec3<float> &dest) override
	{
		auto vehicleTile = this->vehicle.tileObject.lock();
		if (!vehicleTile)
		{
			LogError("Calling on vehicle with no tile object?");
		}
		while (path.empty())
		{
			Vec3<int> newTarget = {xydistribution(rng), xydistribution(rng), zdistribution(rng)};
			while (!vehicleTile->getOwningTile()->map.getTile(newTarget)->ownedObjects.empty())
			{
				newTarget = {xydistribution(rng), xydistribution(rng), zdistribution(rng)};
			}
			path = vehicleTile->getOwningTile()->map.findShortestPath(
			    vehicleTile->getOwningTile()->position, newTarget, 500, vehicle,
			    vehicleCanEnterTile);
			if (path.empty())
			{
				LogInfo("Failed to path - retrying");
				continue;
			}
			// Skip first in the path (as that's current tile)
			path.pop_front();
		}
		if (!path.front()->ownedObjects.empty())
		{
			Vec3<int> target = path.back()->position;
			path = vehicleTile->getOwningTile()->map.findShortestPath(
			    vehicleTile->getOwningTile()->position, target, 500, vehicle, vehicleCanEnterTile);
			if (path.empty())
			{
				LogInfo("Failed to path after obstruction");
				path.clear();
				return this->getNextDestination(dest);
			}
			// Skip first in the path (as that's current tile)
			path.pop_front();
		}
		Tile *nextTile = path.front();
		path.pop_front();
		dest = Vec3<float>{nextTile->position.x, nextTile->position.y, nextTile->position.z}
		       // Add {0.5,0.5,0.5} to make it route to the center of the tile
		       + Vec3<float>{0.5, 0.5, 0.5};
		return true;
	}
	virtual const std::list<Tile *> &getCurrentPlannedPath() override { return path; }
	virtual void start() {}
	virtual bool isFinished() { return false; }
	virtual void update(unsigned int ticks) { std::ignore = ticks; }
};

class VehicleIdleMission : public VehicleMission
{
  public:
	static std::list<Tile *> noPath;
	unsigned int idleTicks;
	VehicleIdleMission(Vehicle &v, unsigned int idleTicks) : VehicleMission(v), idleTicks(idleTicks)
	{
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
	virtual bool getNextDestination(Vec3<float> &dest) override { return false; }
};

VehicleMission::VehicleMission(Vehicle &v) : vehicle(v) {}

VehicleMission::~VehicleMission() {}

VehicleMission *VehicleMission::randomDestination(Vehicle &v)
{
	return new VehicleRandomDestination(v);
}

VehicleMission *VehicleMission::gotoLocation(Vehicle &v, TileMap &map, Vec3<int> target)
{
	return nullptr;
}

VehicleMission *VehicleMission::gotoBuilding(Vehicle &v, TileMap &map, Building &target)
{
	return nullptr;
}

} // namespace OpenApoc

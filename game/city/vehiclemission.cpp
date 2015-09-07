#include "game/city/vehiclemission.h"
#include "game/tileview/tile.h"
#include "game/city/vehicle.h"
#include "framework/logger.h"

#include <random>

namespace
{
std::default_random_engine rng;
} // anonymous namespace

namespace OpenApoc
{

class VehicleRandomDestination : public VehicleMission
{
  public:
	std::uniform_int_distribution<int> xydistribution;
	std::uniform_int_distribution<int> zdistribution;
	VehicleRandomDestination(Vehicle &v)
	    : VehicleMission(v), xydistribution(0, 99), zdistribution(0, 9){};
	std::list<Tile *> path;
	virtual Vec3<float> getNextDestination() override
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
			    vehicleTile->getOwningTile()->position, newTarget);
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
			    vehicleTile->getOwningTile()->position, target);
			if (path.empty())
			{
				LogInfo("Failed to path after obstruction");
				path.clear();
				return this->getNextDestination();
			}
			// Skip first in the path (as that's current tile)
			path.pop_front();
		}
		Tile *nextTile = path.front();
		path.pop_front();
		return Vec3<float>{nextTile->position.x, nextTile->position.y, nextTile->position.z}
		       // Add {0.5,0.5,0.5} to make it route to the center of the tile
		       + Vec3<float>{0.5, 0.5, 0.5};
	}
	virtual const std::list<Tile *> &getCurrentPlannedPath() override { return path; }
	virtual void start() {}
	virtual bool isFinished() { return false; }
	virtual void update(unsigned int ticks) { std::ignore = ticks; }
};

VehicleMission::VehicleMission(Vehicle &v) : vehicle(v) {}

VehicleMission::~VehicleMission() {}

VehicleMission *VehicleMission::randomDestination(Vehicle &v)
{
	return new VehicleRandomDestination(v);
}

} // namespace OpenApoc

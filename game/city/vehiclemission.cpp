#include "game/city/vehiclemission.h"
#include "game/tileview/tile.h"
#include "game/city/vehicle.h"
#include "game/city/building.h"
#include "game/city/scenery.h"
#include "framework/logger.h"
#include "game/tileview/tileobject_vehicle.h"
#include "game/tileview/tileobject_scenery.h"
#include "game/rules/scenery_tile_type.h"

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
	bool canEnterTile(Tile *from, Tile *to) const
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
				if (sceneryTile->scenery.lock()->type->isLandingPad)
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

VehicleMission *VehicleMission::gotoLocation(Vehicle &v, Vec3<int> target)
{
	// TODO
	// Pseudocode:
	// if (in building)
	// 	prepend(TakeOff)
	// routeClosestICanTo(target);
	auto *mission = new VehicleMission();
	mission->type = MissionType::GotoLocation;
	mission->targetLocation = target;
	return mission;
}

VehicleMission *VehicleMission::gotoBuilding(Vehicle &v, StateRef<Building> target)
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
	auto *mission = new VehicleMission();
	mission->type = MissionType::GotoBuilding;
	mission->targetBuilding = target;
	return mission;
}

VehicleMission *VehicleMission::snooze(Vehicle &v, unsigned int snoozeTicks)
{
	auto *mission = new VehicleMission();
	mission->type = MissionType::Snooze;
	mission->snoozeTime = snoozeTicks;
	return mission;
}

VehicleMission *VehicleMission::takeOff(Vehicle &v)
{
	if (!v.currentlyLandedBuilding)
	{
		LogError("Trying to take off while not in a building");
		return nullptr;
	}
	auto *mission = new VehicleMission();
	mission->type = MissionType::TakeOff;
	return mission;
}

VehicleMission *VehicleMission::land(Vehicle &v, StateRef<Building> b)
{
	auto *mission = new VehicleMission();
	mission->type = MissionType::Land;
	mission->targetBuilding = b;
	return mission;
}

VehicleMission::VehicleMission() : targetLocation(0, 0, 0), snoozeTime(0) {}

bool VehicleMission::getNextDestination(Vec3<float> &dest)
{
	switch (this->type)
	{
		default:
			LogWarning("TODO: Implement");
			return false;
	}
}

void VehicleMission::update(unsigned int ticks)
{
	switch (this->type)
	{
		default:
			LogWarning("TODO: Implement");
			return;
	}
}

bool VehicleMission::isFinished()
{
	switch (this->type)
	{
		default:
			LogWarning("TODO: Implement");
			return false;
	}
}

void VehicleMission::start()
{
	switch (this->type)
	{
		default:
			LogWarning("TODO: Implement");
			return;
	}
}

UString VehicleMission::getName()
{
	switch (this->type)
	{
		default:
			LogWarning("TODO: Implement");
			return "";
	}
}

const std::map<VehicleMission::MissionType, UString> VehicleMission::TypeMap = {
    {VehicleMission::MissionType::GotoLocation, "GotoLocation"},
    {VehicleMission::MissionType::GotoBuilding, "GotoBuilding"},
    {VehicleMission::MissionType::FollowVehicle, "FollowVehicle"},
    {VehicleMission::MissionType::AttackVehicle, "AttackVehicle"},
    {VehicleMission::MissionType::AttackBuilding, "AttackBuilding"},
    {VehicleMission::MissionType::Snooze, "Snooze"},
    {VehicleMission::MissionType::TakeOff, "TakeOff"},
    {VehicleMission::MissionType::Land, "Land"},
};

} // namespace OpenApoc

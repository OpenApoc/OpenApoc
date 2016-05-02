#include "game/state/city/vehiclemission.h"
#include "framework/logger.h"
#include "game/state/city/building.h"
#include "game/state/city/scenery.h"
#include "game/state/city/vehicle.h"
#include "game/state/gamestate.h"
#include "game/state/rules/scenery_tile_type.h"
#include "game/state/tileview/tile.h"
#include "game/state/tileview/tileobject_scenery.h"
#include "game/state/tileview/tileobject_vehicle.h"

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
	mission->timeToSnooze = snoozeTicks;
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

VehicleMission::VehicleMission() : targetLocation(0, 0, 0), timeToSnooze(0) {}

bool VehicleMission::getNextDestination(GameState &state, Vehicle &v, Vec3<float> &dest)
{
	switch (this->type)
	{
		case MissionType::TakeOff:      // Fall-through
		case MissionType::GotoLocation: // Fall-through
		case MissionType::Land:
		{
			if (currentPlannedPath.empty())
				return false;
			currentPlannedPath.pop_front();
			if (currentPlannedPath.empty())
				return false;
			auto pos = currentPlannedPath.front();
			dest = Vec3<float>{pos.x, pos.y, pos.z}
			       // Add {0.5,0.5,0.5} to make it route to the center of the tile
			       + Vec3<float>{0.5, 0.5, 0.5};
			return true;
		}
		case MissionType::GotoBuilding:
		{
			if (v.currentlyLandedBuilding != this->targetBuilding)
			{
				auto name = this->getName();
				LogError("Vehicle mission %s: getNextDestination() shouldn't be called unless "
				         "you've reached the target?",
				         name.c_str());
			}
			dest = {0, 0, 9};
			return false;
		}
		case MissionType::Snooze:
		{
			dest = {0, 0, 9};
			return false;
		}
		default:
			LogWarning("TODO: Implement");
			return false;
	}
}

void VehicleMission::update(GameState &state, Vehicle &v, unsigned int ticks)
{
	switch (this->type)
	{
		case MissionType::TakeOff:
		{
			if (v.tileObject)
			{
				// We're already on our way
				return;
			}
			auto b = v.currentlyLandedBuilding;
			if (!b)
			{
				LogError("Building disappeared");
				return;
			}

			// Which city (and therefore map) contains the current building?
			StateRef<City> city;
			for (auto &c : state.cities)
			{
				if (c.second->buildings.find(b.id) != c.second->buildings.end())
				{
					LogInfo("Taking off from building \"%s\" in city \"%s\"", b.id.c_str(),
					        c.first.c_str());
					city = {&state, c.first};
					break;
				}
			}
			if (!city)
			{
				LogError("No city found containing building \"%s\"", b.id.c_str());
				return;
			}
			auto &map = *city->map;
			for (auto padLocation : b->landingPadLocations)
			{
				auto padTile = map.getTile(padLocation);
				auto abovePadLocation = padLocation;
				abovePadLocation.z += 1;
				auto tileAbovePad = map.getTile(abovePadLocation);
				if (!padTile || !tileAbovePad)
				{
					LogError("Invalid landing pad location {%d,%d,%d} - outside map?",
					         padLocation.x, padLocation.y, padLocation.z);
					continue;
				}
				FlyingVehicleCanEnterTileHelper canEnterTileHelper(map, v);
				if (!canEnterTileHelper.canEnterTile(nullptr, padTile) ||
				    !canEnterTileHelper.canEnterTile(padTile, tileAbovePad))
					continue;
				LogInfo("Launching vehicle from building \"%s\" at pad {%d,%d,%d}", b.id.c_str(),
				        padLocation.x, padLocation.y, padLocation.z);
				this->currentPlannedPath = {padLocation, abovePadLocation};
				v.launch(map, state, padLocation);
				return;
			}
			LogInfo("No pad in building \"%s\" free - waiting", b.id.c_str());
			return;
		}
		case MissionType::Land:
			return;
		case MissionType::GotoBuilding:
			return;
		case MissionType::GotoLocation:
			return;
		case MissionType::Snooze:
		{
			if (ticks >= this->timeToSnooze)
				this->timeToSnooze = 0;
			else
				this->timeToSnooze -= ticks;
			return;
		}
		default:
			LogWarning("TODO: Implement");
			return;
	}
}

bool VehicleMission::isFinished(GameState &state, Vehicle &v)
{
	switch (this->type)
	{
		case MissionType::TakeOff:
			return v.tileObject && this->currentPlannedPath.empty();
		case MissionType::Land:
		{
			auto b = this->targetBuilding;
			if (!b)
			{
				LogError("Building disappeared");
				return true;
			}
			if (this->currentPlannedPath.empty())
			{
				/* FIXME: Overloading isFinished() to complete landing action
 * (Should add a ->end() call to mirror ->start()?*/
				v.land(state, b);
				LogInfo("Vehicle mission: Landed in %s", b.id.c_str());
				return true;
			}
			return false;
		}
		case MissionType::GotoLocation:
			return this->currentPlannedPath.empty();
		case MissionType::GotoBuilding:
			return this->targetBuilding == v.currentlyLandedBuilding;
		case MissionType::Snooze:
			return this->timeToSnooze == 0;
		default:
			LogWarning("TODO: Implement");
			return false;
	}
}

void VehicleMission::start(GameState &state, Vehicle &v)
{
	switch (this->type)
	{
		case MissionType::TakeOff: // Fall-through
		case MissionType::Snooze:
			// No setup
			return;
		case MissionType::Land:
		{
			auto b = this->targetBuilding;
			if (!b)
			{
				LogError("Building disappeared");
				return;
			}
			auto vehicleTile = v.tileObject;
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
				         padPosition.x, padPosition.y, padPosition.z + 1, b.id.c_str());
				return;
			}
			this->currentPlannedPath = {padPosition};
			return;
		}
		case MissionType::GotoLocation:
		{

			auto vehicleTile = v.tileObject;
			if (!vehicleTile)
			{
				auto name = this->getName();
				LogInfo("Mission %s: Taking off first", name.c_str());
				auto *takeoffMission = VehicleMission::takeOff(v);
				v.missions.emplace_front(takeoffMission);
				takeoffMission->start(state, v);
			}
			else
			{
				auto &map = vehicleTile->map;
				// FIXME: Change findShortestPath to return Vec3<int> positions?
				auto path = map.findShortestPath(vehicleTile->getOwningTile()->position,
				                                 this->targetLocation, 500,
				                                 FlyingVehicleCanEnterTileHelper{map, v});
				// Always start with the current position
				this->currentPlannedPath.push_back(vehicleTile->getOwningTile()->position);
				for (auto *t : path)
				{
					this->currentPlannedPath.push_back(t->position);
				}
			}
			return;
		}
		case MissionType::GotoBuilding:
		{
			auto name = this->getName();
			LogInfo("Vehicle mission %s checking state", name.c_str());
			auto b = this->targetBuilding;
			if (!b)
			{
				LogError("Building disappeared");
				return;
			}
			if (b == v.currentlyLandedBuilding)
			{
				LogInfo("Vehicle mission %s: Already at building", name.c_str());
				return;
			}
			auto vehicleTile = v.tileObject;
			if (!vehicleTile)
			{
				LogInfo("Mission %s: Taking off first", name.c_str());
				auto *takeoffMission = VehicleMission::takeOff(v);
				v.missions.emplace_front(takeoffMission);
				takeoffMission->start(state, v);
				return;
			}
			/* Am I already above a landing pad? If so land */
			auto position = vehicleTile->getOwningTile()->position;
			LogInfo("Vehicle mission %s: at position {%d,%d,%d}", name.c_str(), position.x,
			        position.y, position.z);
			for (auto padLocation : b->landingPadLocations)
			{
				padLocation.z += 1;
				if (padLocation == position)
				{
					LogInfo("Mission %s: Landing on pad {%d,%d,%d}", name.c_str(), padLocation.x,
					        padLocation.y, padLocation.z - 1);
					auto *landMission = VehicleMission::land(v, b);
					v.missions.emplace_front(landMission);
					landMission->start(state, v);
					return;
				}
			}
			/* I must be in the air and not above a pad - try to find the shortest path to a pad
			 * (if no successfull paths then choose the incomplete path with the lowest (cost +
			 * distance
			 * to goal)*/
			Vec3<int> shortestPathPad = {0, 0, 0};
			float shortestPathCost = std::numeric_limits<float>::max();

			auto &map = vehicleTile->map;

			for (auto dest : b->landingPadLocations)
			{
				// Simply find the nearest landing pad to the current location and route to that
				// Don't pay attention to stuff that blocks us, as things will likely move anyway...

				// We actually want the tile above the pad itself
				dest.z = dest.z + 1;
				if (position == dest)
					continue;
				Vec3<float> currentPosition = position;
				Vec3<float> landingPadPosition = dest;

				float distance = glm::length(currentPosition - landingPadPosition);

				if (distance < shortestPathCost)
					shortestPathPad = dest;
			}

			LogInfo("Vehicle mission %s: Pathing to pad at {%d,%d,%d}", name.c_str(),
			        shortestPathPad.x, shortestPathPad.y, shortestPathPad.z);
			auto *gotoMission = VehicleMission::gotoLocation(v, shortestPathPad);
			v.missions.emplace_front(gotoMission);
			gotoMission->start(state, v);
			return;
		}
		default:
			LogWarning("TODO: Implement");
			return;
	}
}

UString VehicleMission::getName()
{
	UString name = "UNKNOWN";
	auto it = VehicleMission::TypeMap.find(this->type);
	if (it != VehicleMission::TypeMap.end())
		name = it->second;
	switch (this->type)
	{
		case MissionType::GotoLocation:
			name += UString::format(" {%d,%d,%d}", this->targetLocation.x, this->targetLocation.y,
			                        this->targetLocation.z);
			break;
		case MissionType::GotoBuilding:
			break;
		case MissionType::FollowVehicle:
			break;
		case MissionType::AttackBuilding:
			break;
		case MissionType::Snooze:
			break;
		case MissionType::TakeOff:
			break;
		case MissionType::Land:
			name += " in " + this->targetBuilding.id;
			break;
	}
	return name;
}

const std::map<VehicleMission::MissionType, UString> VehicleMission::TypeMap = {
    {MissionType::GotoLocation, "GotoLocation"},
    {MissionType::GotoBuilding, "GotoBuilding"},
    {MissionType::FollowVehicle, "FollowVehicle"},
    {MissionType::AttackVehicle, "AttackVehicle"},
    {MissionType::AttackBuilding, "AttackBuilding"},
    {MissionType::Snooze, "Snooze"},
    {MissionType::TakeOff, "TakeOff"},
    {MissionType::Land, "Land"},
};

} // namespace OpenApoc

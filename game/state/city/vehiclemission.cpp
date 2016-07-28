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

class FlyingVehicleTileHelper : public CanEnterTileHelper
{
  private:
	TileMap &map;
	Vehicle &v;

  public:
	FlyingVehicleTileHelper(TileMap &map, Vehicle &v) : map(map), v(v) {}
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
			LogError("No 'to' position supplied");
			return false;
		}
		Vec3<int> toPos = to->position;
		if (fromPos == toPos)
		{
			LogError("FromPos == ToPos {%d,%d,%d}", toPos.x, toPos.y, toPos.z);
			return false;
		}
		if (!map.tileIsValid(toPos))
		{
			LogError("ToPos {%d,%d,%d} is not on the map", toPos.x, toPos.y, toPos.z);
			return false;
		}

		if (v.type->size.x > 1)
		{
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

	float adjustCost(Vec3<int> nextPosition, int z) const override
	{
		if ((nextPosition.z < (int)v.altitude && z == 1) ||
		    (nextPosition.z > (int)v.altitude && z == -1) || nextPosition.z == (int)v.altitude)
		{
			return -0.5f;
		}
		return 0;
	}

	bool canLandOnTile(Tile *to) const
	{
		if (!to)
		{
			LogError("No 'to' position supplied");
			return false;
		}
		Vec3<int> toPos = to->position;

		for (int y = 0; y < v.type->size.y; y++)
		{
			for (int x = 0; x < v.type->size.x; x++)
			{
				for (auto obj : map.getTile(x + toPos.x, y + toPos.y, toPos.z)->ownedObjects)
				{
					if (obj->getType() == TileObject::Type::Scenery)
					{
						auto scenery =
						    std::static_pointer_cast<TileObjectScenery>(obj)->scenery.lock();
						if (scenery->type->tile_type != SceneryTileType::TileType::General ||
						    scenery->type->walk_mode == SceneryTileType::WalkMode::None ||
						    scenery->type->isLandingPad || scenery->type->isHill)
							return false;
					}
				}
			}
		}
		return true;
	}

	Vec3<int> findTileToLandOn(GameState &state, sp<TileObjectVehicle> vTile) const
	{
		const int lookupRaduis = 10;
		auto startPos = vTile->getOwningTile()->position;

		for (int r = 0; r < lookupRaduis; r++)
		{
			for (int y = startPos.y - r; y <= startPos.y + r; y++)
			{
				bool middle = y > startPos.y - r && y < startPos.y + r;
				for (int x = startPos.x - r; x <= startPos.x + r;)
				{
					for (int z = startPos.z - 1; z >= 0; z--)
					{
						auto tile = map.getTile(x, y, z);
						bool hasScenery = false;
						for (auto obj : tile->ownedObjects)
						{
							if (obj->getType() == TileObject::Type::Scenery)
							{
								hasScenery = true;
								break;
							}
						}

						if (hasScenery && canLandOnTile(tile))
						{
							auto crashTile = tile->position;
							crashTile.z++;
							return crashTile;
						}
					}
					if (middle && x == startPos.x - r)
					{
						x = startPos.x + r;
					}
					else
					{
						x++;
					}
				}
			}
		}
		return startPos;
	}

	Vec3<float> findSidestep(GameState &state, sp<TileObjectVehicle> vTile,
	                         sp<TileObjectVehicle> targetTile, float distancePref) const
	{
		const int maxIterations = 6;
		auto bestPosition = vTile->getPosition();
		float closest = std::numeric_limits<float>::max();
		std::uniform_real_distribution<float> offset(-1.0f, 1.0f);

		for (int i = 0; i < maxIterations; i++)
		{
			float xOffset = offset(state.rng);
			float yOffset = offset(state.rng);
			auto newPosition = vTile->getPosition();
			newPosition.x += xOffset;
			newPosition.y += yOffset;

			if (static_cast<int>(newPosition.z) < static_cast<int>(v.altitude))
			{
				newPosition.z += 1;
			}
			else if (static_cast<int>(newPosition.z) > static_cast<int>(v.altitude))
			{
				newPosition.z -= 1;
			}
			newPosition.z = glm::clamp(newPosition.z, 0.0f, map.size.z - 0.1f);

			if (static_cast<Vec3<int>>(newPosition) != vTile->getOwningTile()->position &&
			    canEnterTile(vTile->getOwningTile(), map.getTile(newPosition)))
			{
				float currentDist = glm::abs(distancePref - targetTile->getDistanceTo(newPosition));
				if (currentDist < closest)
				{
					closest = currentDist;
					bestPosition = newPosition;
				}
			}
		}
		return bestPosition;
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

VehicleMission *VehicleMission::gotoPortal(Vehicle &v, Vec3<int> target)
{
	auto *mission = new VehicleMission();
	mission->type = MissionType::GotoPortal;
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

VehicleMission *VehicleMission::infiltrateBuilding(Vehicle &v, StateRef<Building> target)
{
	auto *mission = new VehicleMission();
	mission->type = MissionType::Infiltrate;
	mission->targetBuilding = target;
	return mission;
}

VehicleMission *VehicleMission::attackVehicle(Vehicle &v, StateRef<Vehicle> target)
{
	auto *mission = new VehicleMission();
	mission->type = MissionType::AttackVehicle;
	mission->targetVehicle = target;
	return mission;
}

VehicleMission *VehicleMission::followVehicle(Vehicle &v, StateRef<Vehicle> target)
{
	auto *mission = new VehicleMission();
	mission->type = MissionType::FollowVehicle;
	mission->targetVehicle = target;
	return mission;
}

VehicleMission *VehicleMission::snooze(Vehicle &v, unsigned int snoozeTicks)
{
	auto *mission = new VehicleMission();
	mission->type = MissionType::Snooze;
	mission->timeToSnooze = snoozeTicks;
	return mission;
}

VehicleMission *VehicleMission::crashLand(Vehicle &v)
{
	auto *mission = new VehicleMission();
	mission->type = MissionType::Crash;
	return mission;
}

VehicleMission *VehicleMission::patrol(Vehicle &v)
{
	auto *mission = new VehicleMission();
	mission->type = MissionType::Patrol;
	mission->missionCounter = 5;
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
		case MissionType::Infiltrate:
		{
			return advanceAlongPath(dest);
		}
		case MissionType::Crash:
		{
			if (currentPlannedPath.empty())
				return false;
			currentPlannedPath.pop_front();
			if (currentPlannedPath.empty())
				return false;
			auto pos = currentPlannedPath.front();
			dest = Vec3<float>{pos.x, pos.y, pos.z}
			       // Add {0.5,0.5,0.5} to make it route to the center of the tile
			       + Vec3<float>{0.5, 0.5, 0.0};
			return true;
		}
		case MissionType::Patrol:
			if (!advanceAlongPath(dest))
			{
				if (missionCounter == 0)
					return false;

				missionCounter--;
				std::uniform_int_distribution<int> xyPos(25, 115);
				setPathTo(v, {xyPos(state.rng), xyPos(state.rng), v.altitude});
			}
			break;
		case MissionType::AttackVehicle:
		case MissionType::FollowVehicle:
		{
			auto vTile = v.tileObject;
			auto targetTile = this->targetVehicle->tileObject;

			if (vTile && targetTile)
			{
				auto &map = vTile->map;
				FlyingVehicleTileHelper tileHelper(map, v);

				float distancePreference = 5 * VELOCITY_SCALE.x;
				if (this->type == MissionType::AttackVehicle && v.getFiringRange())
				{
					distancePreference = v.getFiringRange();

					switch (v.attackMode)
					{
						case Vehicle::AttackMode::Aggressive:
							distancePreference /= 4;
							break;
						case Vehicle::AttackMode::Standard:
							distancePreference /= 3;
							break;
						case Vehicle::AttackMode::Defensive:
							distancePreference /= 2;
							break;
						case Vehicle::AttackMode::Evasive:
							distancePreference /= 1.5f;
							break;
					}
				}

				if (vTile->getDistanceTo(targetTile) < distancePreference)
				{
					// target is in range, we're done with pathing and start maneuvering
					if (!currentPlannedPath.empty())
						currentPlannedPath.clear();

					auto newPosition =
					    tileHelper.findSidestep(state, vTile, targetTile, distancePreference);
					if (newPosition != vTile->getPosition())
					{
						dest = newPosition;
						return true;
					}
					return false;
				}
				else if (targetTile->getOwningTile()->position != this->targetLocation ||
				         currentPlannedPath.empty())
				{
					// adjust the path if target moved
					currentPlannedPath.clear();
					this->targetLocation = targetTile->getOwningTile()->position;
					setPathTo(v, this->targetLocation, 100);
				}

				// continue
				if (!currentPlannedPath.empty())
				{
					currentPlannedPath.pop_front();
					if (currentPlannedPath.empty())
						return false;
					auto pos = currentPlannedPath.front();
					dest = Vec3<float>{pos.x, pos.y, pos.z}
					       // Add {0.5,0.5,0.5} to make it route to the center of the tile
					       + Vec3<float>{0.5, 0.5, 0.5};
					return true;
				}
			}
			return false;
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
				FlyingVehicleTileHelper tileHelper(map, v);
				if (!tileHelper.canEnterTile(nullptr, padTile) ||
				    !tileHelper.canEnterTile(padTile, tileAbovePad))
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
		case MissionType::Patrol:
		{
			float range = v.getFiringRange();
			if (v.tileObject && range > 0)
			{
				auto enemy = v.findClosestEnemy(state, v.tileObject);
				if (enemy)
				{
					StateRef<Vehicle> vehicleRef(&state, enemy->getVehicle());
					auto attackMission = VehicleMission::attackVehicle(v, vehicleRef);
					v.missions.emplace_front(attackMission);
					attackMission->start(state, v);
				}
			}
			return;
		}
		case MissionType::Infiltrate:
		{
			// this should run before mission is checked and destroyed
			if (this->isFinished(state, v))
			{
				auto doodad =
				    v.city->placeDoodad(StateRef<DoodadType>{&state, "DOODAD_INFILTRATION_RAY"},
				                        v.tileObject->getPosition() - Vec3<float>{0, 0, 1});
			}
			return;
		}
		case MissionType::Land:
		case MissionType::Crash:
		case MissionType::GotoBuilding:
		case MissionType::GotoLocation:
		case MissionType::AttackVehicle:
		case MissionType::FollowVehicle:
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
		case MissionType::Crash:
		case MissionType::Infiltrate:
			return this->currentPlannedPath.empty();
		case MissionType::Patrol:
			return this->missionCounter == 0 && this->currentPlannedPath.empty();
		case MissionType::GotoBuilding:
			return this->targetBuilding == v.currentlyLandedBuilding;
		case MissionType::AttackVehicle:
		{
			auto t = this->targetVehicle;
			if (!t)
			{
				LogError("Target disappeared");
				return true;
			}
			auto targetTile = t->tileObject;
			if (!targetTile)
			{
				LogInfo("Vehicle attack mission: Target not on the map");
				return true;
			}
			if (t->type->type == VehicleType::Type::UFO)
			{
				return t->isCrashed();
			}
			return this->targetVehicle->health == 0;
		}
		case MissionType::FollowVehicle:
		{
			auto t = this->targetVehicle;
			if (!t)
			{
				// Target was destroyed
				return true;
			}
			auto targetTile = t->tileObject;
			if (!targetTile)
			{
				// Target not on the map anymore
				return true;
			}
			if (t->type->type == VehicleType::Type::UFO)
			{
				return t->isCrashed();
			}
			return this->targetVehicle->health == 0;
		}
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
				this->setPathTo(v, this->targetLocation);
			}
			return;
		}
		case MissionType::Crash:
		{
			auto vehicleTile = v.tileObject;
			if (!vehicleTile)
			{
				LogError("Trying to crash land vehicle that's not in the air?");
				return;
			}
			auto &map = vehicleTile->map;
			FlyingVehicleTileHelper tileHelper(map, v);

			auto tile = tileHelper.findTileToLandOn(state, vehicleTile);
			if (tile == vehicleTile->getOwningTile()->position)
			{
				// FIXME: Snooze for a while (?)
				auto name = this->getName();
				LogError("Vehicle mission %s failed to find a place to land", name.c_str());
				return;
			}
			this->targetLocation = tile;
			setPathTo(v, tile);
			return;
		}
		case MissionType::Patrol:
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
			return;
		}
		case MissionType::FollowVehicle:
		case MissionType::AttackVehicle:
		{
			auto name = this->getName();
			LogInfo("Vehicle mission %s checking state", name.c_str());
			auto t = this->targetVehicle;
			if (!t)
			{
				LogError("Target disappeared");
				return;
			}
			if (v.shared_from_this() == t.getSp())
			{
				LogError("Vehicle mission %s: Targeting itself", name.c_str());
				return;
			}
			auto targetTile = t->tileObject;
			if (!targetTile)
			{
				LogInfo("Vehicle mission %s: Target not on the map", name.c_str());
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
			this->targetLocation = targetTile->getOwningTile()->position;
			this->setPathTo(v, this->targetLocation);
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
		case MissionType::Infiltrate:
		{
			auto name = this->getName();
			auto b = this->targetBuilding;
			if (!b)
			{
				LogError("Building disappeared");
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

			auto &map = vehicleTile->map;
			Vec3<int> goodPos{0, 0, 0};
			int xPos = (b->bounds.p0.x + b->bounds.p1.x) / 2;
			int yPos = (b->bounds.p0.y + b->bounds.p1.y) / 2;
			for (int z = map.size.z - 1; z > 0; z--)
			{
				auto t = map.getTile(xPos, yPos, z);
				if (t->ownedObjects.empty())
				{
					goodPos = t->position;
				}
				else
				{
					break;
				}
			}
			if (goodPos.z != 0)
				setPathTo(v, goodPos);
			return;
		}
		default:
			LogWarning("TODO: Implement");
			return;
	}
}

void VehicleMission::setPathTo(Vehicle &v, Vec3<int> target, int maxIterations)
{
	auto vehicleTile = v.tileObject;
	if (vehicleTile)
	{
		auto &map = vehicleTile->map;
		// FIXME: Change findShortestPath to return Vec3<int> positions?
		auto path =
		    map.findShortestPath(vehicleTile->getOwningTile()->position, target, maxIterations,
		                         FlyingVehicleTileHelper{map, v}, (float)v.altitude);

		// Always start with the current position
		this->currentPlannedPath.push_back(vehicleTile->getOwningTile()->position);
		for (auto *t : path)
		{
			this->currentPlannedPath.push_back(t->position);
		}
	}
	else
	{
		LogError("Mission %s: Take off before pathfinding!", this->getName().c_str());
	}
}

bool VehicleMission::advanceAlongPath(Vec3<float> &dest)
{
	// Add {0.5,0.5,0.5} to make it route to the center of the tile
	static const Vec3<float> offset{0.5, 0.5, 0.5};

	if (currentPlannedPath.empty())
		return false;
	currentPlannedPath.pop_front();
	if (currentPlannedPath.empty())
		return false;
	auto pos = currentPlannedPath.front();
	dest = Vec3<float>{pos.x, pos.y, pos.z} + offset;
	return true;
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
		case MissionType::Crash:
			name += UString::format(" landing on {%d,%d,%d}", this->targetLocation.x,
			                        this->targetLocation.y, this->targetLocation.z);
			break;
		case MissionType::AttackVehicle:
			name += UString::format(" target \"%s\"", this->targetVehicle.id.c_str());
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
    {MissionType::Crash, "Crash"},
};

} // namespace OpenApoc

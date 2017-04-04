#include "game/state/city/vehiclemission.h"
#include "framework/logger.h"
#include "game/state/city/building.h"
#include "game/state/city/city.h"
#include "game/state/city/doodad.h"
#include "game/state/city/scenery.h"
#include "game/state/city/vehicle.h"
#include "game/state/gamestate.h"
#include "game/state/rules/scenery_tile_type.h"
#include "game/state/rules/vehicle_type.h"
#include "game/state/tileview/tile.h"
#include "game/state/tileview/tileobject_doodad.h"
#include "game/state/tileview/tileobject_scenery.h"
#include "game/state/tileview/tileobject_shadow.h"
#include "game/state/tileview/tileobject_vehicle.h"
#include "library/strings_format.h"
#include <glm/glm.hpp>

namespace OpenApoc
{

class FlyingVehicleTileHelper : public CanEnterTileHelper
{
  private:
	TileMap &map;
	Vehicle &v;

  public:
	FlyingVehicleTileHelper(TileMap &map, Vehicle &v) : map(map), v(v) {}

	bool canEnterTile(Tile *from, Tile *to, bool ignoreStaticUnits = false,
	                  bool ignoreAllUnits = false) const override
	{
		float nothing;
		bool none1;
		bool none2;
		return canEnterTile(from, to, false, none1, nothing, none2, ignoreStaticUnits,
		                    ignoreAllUnits);
	}

	// Support 'from' being nullptr for if a vehicle is being spawned in the map
	bool canEnterTile(Tile *from, Tile *to, bool, bool &, float &cost, bool &, bool,
	                  bool) const override
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
			LogError("FromPos == ToPos %s", toPos.x);
			return false;
		}
		if (!map.tileIsValid(toPos))
		{
			LogError("ToPos %s is not on the map", toPos.x);
			return false;
		}

		// FIXME: Check for big vehicles
		// if (v.type->size.x > 1)
		//{
		//}

		for (auto &obj : to->ownedObjects)
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
		cost = glm::length(Vec3<float>{fromPos} - Vec3<float>{toPos});
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

	float getDistance(Vec3<float> from, Vec3<float> to) const override
	{
		return glm::length(to - from);
	}

	float getDistance(Vec3<float> from, Vec3<float> toStart, Vec3<float> toEnd) const override
	{
		auto diffStart = toStart - from;
		auto diffEnd = toEnd - from - Vec3<float>{1.0f, 1.0f, 1.0f};
		auto xDiff = from.x >= toStart.x && from.x < toEnd.x
		                 ? 0.0f
		                 : std::min(std::abs(diffStart.x), std::abs(diffEnd.x));
		auto yDiff = from.y >= toStart.y && from.y < toEnd.y
		                 ? 0.0f
		                 : std::min(std::abs(diffStart.y), std::abs(diffEnd.y));
		auto zDiff = from.z >= toStart.z && from.z < toEnd.z
		                 ? 0.0f
		                 : std::min(std::abs(diffStart.z), std::abs(diffEnd.z));
		return sqrtf(xDiff * xDiff + yDiff * yDiff + zDiff * zDiff);
	}

	bool canLandOnTile(Tile *to) const
	{
		if (!to)
		{
			LogError("No 'to' position supplied");
			return false;
		}
		Vec3<int> toPos = to->position;

		int xMax = 0;
		int yMax = 0;
		for (auto &s : v.type->size)
		{
			xMax = std::max(xMax, s.second.x);
			yMax = std::max(yMax, s.second.y);
		}

		for (int y = 0; y < yMax; y++)
		{
			for (int x = 0; x < xMax; x++)
			{
				for (auto &obj : map.getTile(x + toPos.x, y + toPos.y, toPos.z)->ownedObjects)
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

	Vec3<int> findTileToLandOn(GameState &, sp<TileObjectVehicle> vTile) const
	{
		const int lookupRaduis = 10;
		auto startPos = vTile->getOwningTile()->position;
		// Make sure we check from high enough altitude
		auto startZ = std::max(startPos.z, 4);

		for (int r = 0; r < lookupRaduis; r++)
		{
			for (int y = startPos.y - r; y <= startPos.y + r; y++)
			{
				bool middle = y > startPos.y - r && y < startPos.y + r;
				for (int x = startPos.x - r; x <= startPos.x + r;)
				{
					// Ensure we're not trying to land into a tile that's blocked
					bool lastHasScenery = true;
					for (int z = startZ; z >= 0; z--)
					{
						auto tile = map.getTile(x, y, z);
						bool hasScenery = false;
						for (auto &obj : tile->ownedObjects)
						{
							if (obj->getType() == TileObject::Type::Scenery)
							{
								hasScenery = true;
								break;
							}
						}
						if (hasScenery && !lastHasScenery && canLandOnTile(tile))
						{
							auto crashTile = tile->position;
							crashTile.z++;
							return crashTile;
						}
						lastHasScenery = hasScenery;
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

VehicleMission *VehicleMission::gotoLocation(GameState &, Vehicle &, Vec3<int> target,
                                             bool pickNearest, int reRouteAttempts)
{
	// TODO
	// Pseudocode:
	// if (in building)
	// 	prepend(TakeOff)
	// routeClosestICanTo(target);
	auto *mission = new VehicleMission();
	mission->type = MissionType::GotoLocation;
	mission->targetLocation = target;
	mission->pickNearest = pickNearest;
	mission->reRouteAttempts = reRouteAttempts;
	return mission;
}

VehicleMission *VehicleMission::gotoPortal(GameState &state, Vehicle &v)
{
	Vec3<int> target = {0, 0, 0};
	auto vTile = v.tileObject;
	if (vTile)
	{
		float closestPortalRange = std::numeric_limits<float>::max();
		for (auto &p : v.city->portals)
		{
			float distance = vTile->getDistanceTo(p->tileObject);
			if (distance < closestPortalRange)
			{
				closestPortalRange = distance;
				target = p->tileObject->getOwningTile()->position;
			}
		}
	}
	return gotoPortal(state, v, target);
}

VehicleMission *VehicleMission::gotoPortal(GameState &, Vehicle &, Vec3<int> target)
{
	auto *mission = new VehicleMission();
	mission->type = MissionType::GotoPortal;
	mission->targetLocation = target;
	return mission;
}

VehicleMission *VehicleMission::gotoBuilding(GameState &, Vehicle &, StateRef<Building> target)
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

VehicleMission *VehicleMission::infiltrateOrSubvertBuilding(GameState &, Vehicle &,
                                                            StateRef<Building> target, bool subvert)
{
	auto *mission = new VehicleMission();
	mission->type = MissionType::InfiltrateSubvert;
	mission->targetBuilding = target;
	mission->subvert = subvert;
	return mission;
}

VehicleMission *VehicleMission::attackVehicle(GameState &, Vehicle &, StateRef<Vehicle> target)
{
	auto *mission = new VehicleMission();
	mission->type = MissionType::AttackVehicle;
	mission->targetVehicle = target;
	return mission;
}

VehicleMission *VehicleMission::followVehicle(GameState &, Vehicle &, StateRef<Vehicle> target)
{
	auto *mission = new VehicleMission();
	mission->type = MissionType::FollowVehicle;
	mission->targetVehicle = target;
	return mission;
}

VehicleMission *VehicleMission::snooze(GameState &, Vehicle &, unsigned int snoozeTicks)
{
	auto *mission = new VehicleMission();
	mission->type = MissionType::Snooze;
	mission->timeToSnooze = snoozeTicks;
	return mission;
}

VehicleMission *VehicleMission::restartNextMission(GameState &, Vehicle &)
{
	auto *mission = new VehicleMission();
	mission->type = MissionType::RestartNextMission;
	return mission;
}

VehicleMission *VehicleMission::crashLand(GameState &, Vehicle &)
{
	auto *mission = new VehicleMission();
	mission->type = MissionType::Crash;
	return mission;
}

VehicleMission *VehicleMission::patrol(GameState &, Vehicle &, unsigned int counter)
{
	auto *mission = new VehicleMission();
	mission->type = MissionType::Patrol;
	mission->missionCounter = counter;
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

VehicleMission *VehicleMission::land(Vehicle &, StateRef<Building> b)
{
	auto *mission = new VehicleMission();
	mission->type = MissionType::Land;
	mission->targetBuilding = b;
	return mission;
}

bool VehicleMission::takeOffCheck(GameState &state, Vehicle &v, UString mission)
{
	if (!v.tileObject)
	{
		if (v.currentlyLandedBuilding)
		{
			LogInfo("Mission %s: Taking off first", mission);
			auto *takeoffMission = VehicleMission::takeOff(v);
			v.missions.emplace_front(takeoffMission);
			takeoffMission->start(state, v);
			return true;
		}
		else
		{
			LogError("Mission %s: Vehicle not in the city and shouldn't recieve orders", mission);
			return false;
		}
	}
	return false;
}

bool VehicleMission::getNextDestination(GameState &state, Vehicle &v, Vec3<float> &dest)
{
	switch (this->type)
	{
		case MissionType::TakeOff:      // Fall-through
		case MissionType::GotoLocation: // Fall-through
		case MissionType::Crash:
		case MissionType::Land:
		{
			return advanceAlongPath(state, dest, v);
		}
		case MissionType::Patrol:
			if (!advanceAlongPath(state, dest, v))
			{
				if (missionCounter == 0)
					return false;

				missionCounter--;
				std::uniform_int_distribution<int> xyPos(25, 115);
				setPathTo(state, v, {xyPos(state.rng), xyPos(state.rng), v.altitude});
			}
			return false;
		case MissionType::AttackVehicle:
		case MissionType::FollowVehicle:
		{
			auto vTile = v.tileObject;
			auto targetTile = this->targetVehicle->tileObject;

			if (vTile && targetTile)
			{
				auto &map = vTile->map;
				FlyingVehicleTileHelper tileHelper(map, v);

				float distancePreference = 5 * VELOCITY_SCALE_CITY.x;
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
					setPathTo(state, v, this->targetLocation, 25, false);
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
				         name);
			}
			dest = {0, 0, 9};
			return false;
		}
		case MissionType::Snooze:
		case MissionType::RestartNextMission:
		case MissionType::GotoPortal:
		case MissionType::InfiltrateSubvert:
		{
			return false;
		}
		default:
			LogWarning("TODO: Implement getNextDestination");
			return false;
	}
	return false;
}

void VehicleMission::update(GameState &state, Vehicle &v, unsigned int ticks, bool finished)
{
	finished = finished || isFinishedInternal(state, v);
	switch (this->type)
	{
		case MissionType::TakeOff:
		{
			if (v.tileObject)
			{
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
					LogInfo("Taking off from building \"%s\" in city \"%s\"", b.id, c.first);
					city = {&state, c.first};
					break;
				}
			}
			if (!city)
			{
				LogError("No city found containing building \"%s\"", b.id);
				return;
			}
			auto &map = *city->map;
			for (auto &padLocation : b->landingPadLocations)
			{
				auto padTile = map.getTile(padLocation);
				auto abovePadLocation = padLocation;
				abovePadLocation.z += 1;
				auto tileAbovePad = map.getTile(abovePadLocation);
				if (!padTile || !tileAbovePad)
				{
					LogError("Invalid landing pad location %s - outside map?", padLocation.x);
					continue;
				}
				FlyingVehicleTileHelper tileHelper(map, v);
				Vec3<float> belowPadLocation = padLocation;
				belowPadLocation += Vec3<float>{0.5f, 0.5f, -1.0f};
				auto belowPadTile = map.getTile(belowPadLocation);
				bool padIsBusy = (!tileHelper.canEnterTile(nullptr, padTile) ||
				                  !tileHelper.canEnterTile(padTile, tileAbovePad));
				if (!padIsBusy)
				{
					for (auto &obj : belowPadTile->ownedObjects)
					{
						if (obj->getType() == TileObject::Type::Vehicle)
						{
							padIsBusy = true;
							break;
						}
					}
				}
				if (padIsBusy)
				{
					continue;
				}
				LogInfo("Launching vehicle from building \"%s\" at pad %s", b.id, padLocation);
				this->currentPlannedPath = {belowPadLocation, belowPadLocation, padLocation,
				                            abovePadLocation};
				v.launch(map, state, belowPadLocation);
				return;
			}
			LogInfo("No pad in building \"%s\" free - waiting", b.id);
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
					auto attackMission = VehicleMission::attackVehicle(state, v, vehicleRef);
					v.missions.emplace_front(attackMission);
					attackMission->start(state, v);
				}
			}
			return;
		}
		case MissionType::GotoPortal:
		{
			auto vTile = v.tileObject;
			if (vTile && finished)
			{
				for (auto &city : state.cities)
				{
					if (city.second != v.city.getSp())
					{
						v.shadowObject->removeFromMap();
						v.tileObject->removeFromMap();
						v.shadowObject.reset();
						v.tileObject.reset();
						v.city = {&state, city.second};
						// FIXME: add GotoBase mission
						return;
					}
				}
			}
			if (vTile && !finished && this->currentPlannedPath.empty())
			{
				// Forever path to portal, eventually it will work
				setPathTo(state, v, targetLocation);
			}
			return;
		}
		case MissionType::Crash:
		{
			auto vTile = v.tileObject;
			if (vTile && !finished && this->currentPlannedPath.empty())
			{
				LogWarning("Crash landing failed, restartng...");
				auto *restartMision = restartNextMission(state, v);
				v.missions.emplace_front(restartMision);
				restartMision->start(state, v);
			}
			return;
		}
		case MissionType::GotoLocation:
		{
			auto vTile = v.tileObject;
			if (vTile && !finished && this->currentPlannedPath.empty())
			{
				if (reRouteAttempts > 0)
				{
					reRouteAttempts--;
					setPathTo(state, v, targetLocation);
				}
				else
				{
					// Finall attempt, give up if fails
					setPathTo(state, v, targetLocation, 500, true, true);
				}
			}
			return;
		}
		case MissionType::Land:
			if (finished && v.tileObject)
			{
				auto b = this->targetBuilding;
				if (!b)
				{
					LogError("Building disappeared");
					return;
				}
				v.land(state, b);
				LogInfo("Vehicle mission: Landed in %s", b.id);
				return;
			}
			return;
		case MissionType::InfiltrateSubvert:
		case MissionType::GotoBuilding:
		case MissionType::AttackVehicle:
		case MissionType::FollowVehicle:
		case MissionType::RestartNextMission:
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
			LogWarning("TODO: Implement update");
			return;
	}
}

bool VehicleMission::isFinished(GameState &state, Vehicle &v, bool callUpdateIfFinished)
{
	if (isFinishedInternal(state, v))
	{
		if (callUpdateIfFinished)
		{
			update(state, v, 0, true);
		}
		return true;
	}
	return false;
}

bool VehicleMission::isFinishedInternal(GameState &, Vehicle &v)
{
	switch (this->type)
	{
		case MissionType::TakeOff:
			return v.tileObject && this->currentPlannedPath.empty();
		case MissionType::Land:
			return currentPlannedPath.empty();
		case MissionType::GotoPortal:
		case MissionType::GotoLocation:
		case MissionType::Crash:
		{
			auto vTile = v.tileObject;
			if (vTile && this->currentPlannedPath.empty() &&
			    (pickNearest || vTile->getOwningTile()->position == this->targetLocation))
				return true;
			return false;
		}
		case MissionType::InfiltrateSubvert:
		{
			return missionCounter > 1;
		}
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
		case MissionType::RestartNextMission:
			return true;
		default:
			LogWarning("TODO: Implement isFinishedInternal");
			return false;
	}
}

void VehicleMission::start(GameState &state, Vehicle &v)
{
	switch (this->type)
	{
		case MissionType::TakeOff: // Fall-through
		case MissionType::RestartNextMission:
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
			auto vehiclePosition = vehicleTile->getOwningTile()->position;
			if (vehiclePosition.z < 2)
			{
				LogError("Vehicle trying to land off bottom of map %s", vehiclePosition);
				return;
			}
			auto padPosition = vehiclePosition - Vec3<int>{0, 0, 1};

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
				LogError("Vehicle at %s not directly above a landing pad for building %s",
				         vehiclePosition, b.id);
				return;
			}
			this->currentPlannedPath = {padPosition, padPosition - Vec3<int>{0, 0, 1}};
			return;
		}
		case MissionType::GotoPortal:
		{
			if (!isFinished(state, v))
			{
				LogInfo("Vehicle mission %s: Pathing to portal at %s", getName(), targetLocation);
				auto *gotoMission = VehicleMission::gotoLocation(state, v, targetLocation);
				v.missions.emplace_front(gotoMission);
				gotoMission->start(state, v);
			}
			return;
		}
		case MissionType::GotoLocation:
		{
			auto vehicleTile = v.tileObject;
			if (takeOffCheck(state, v, this->getName()))
			{
				return;
			}
			else
			{
				this->setPathTo(state, v, this->targetLocation);
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
				Vec3<int> randomNearbyPos = {
				    randBoundsInclusive(state.rng, tile.x - 4, tile.x + 4),
				    randBoundsInclusive(state.rng, tile.y - 4, tile.y + 4),
				    randBoundsInclusive(state.rng, tile.z - 2, tile.z + 2)};
				randomNearbyPos.x = clamp(randomNearbyPos.x, 0, map.size.x - 1);
				randomNearbyPos.y = clamp(randomNearbyPos.y, 0, map.size.y - 1);
				randomNearbyPos.z = clamp(randomNearbyPos.z, 0, map.size.z - 1);
				LogInfo("Vehicle mission %s: Can't find place to land, moving to random location "
				        "at %s",
				        getName(), randomNearbyPos);
				auto *gotoMission =
				    VehicleMission::gotoLocation(state, v, randomNearbyPos, true, 0);
				v.missions.emplace_front(gotoMission);
				gotoMission->start(state, v);
				return;
			}
			this->targetLocation = tile;
			setPathTo(state, v, tile);
			return;
		}
		case MissionType::Patrol:
		{
			takeOffCheck(state, v, this->getName());
			return;
		}
		case MissionType::FollowVehicle:
		case MissionType::AttackVehicle:
		{
			auto name = this->getName();
			LogInfo("Vehicle mission %s checking state", name);
			auto t = this->targetVehicle;
			if (!t)
			{
				LogError("Target disappeared");
				return;
			}
			if (v.shared_from_this() == t.getSp())
			{
				LogError("Vehicle mission %s: Targeting itself", name);
				return;
			}
			auto targetTile = t->tileObject;
			if (!targetTile)
			{
				LogInfo("Vehicle mission %s: Target not on the map", name);
				return;
			}
			auto vehicleTile = v.tileObject;
			if (takeOffCheck(state, v, name))
			{
				return;
			}
			this->targetLocation = targetTile->getOwningTile()->position;
			this->setPathTo(state, v, this->targetLocation, 25, false);
			return;
		}
		case MissionType::GotoBuilding:
		{
			auto name = this->getName();
			LogInfo("Vehicle mission %s checking state", name);
			auto b = this->targetBuilding;
			if (!b)
			{
				LogError("Building disappeared");
				return;
			}
			if (b == v.currentlyLandedBuilding)
			{
				LogInfo("Vehicle mission %s: Already at building", name);
				return;
			}
			auto vehicleTile = v.tileObject;
			if (takeOffCheck(state, v, name))
			{
				return;
			}
			/* Am I already above a landing pad? If so land */
			auto position = vehicleTile->getOwningTile()->position;
			LogInfo("Vehicle mission %s: at position %s", name, position);
			for (auto &padLocation : b->landingPadLocations)
			{
				auto abovePadLocation = padLocation;
				abovePadLocation.z += 1;
				if (abovePadLocation == position)
				{
					LogInfo("Mission %s: Landing on pad %s", name, padLocation);
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

			for (auto &dest : b->landingPadLocations)
			{
				// Simply find the nearest landing pad to the current location and route to that
				// Don't pay attention to stuff that blocks us, as things will likely move anyway...

				// We actually want the tile above the pad itself
				auto aboveDest = dest;
				aboveDest.z += 1;
				if (position == aboveDest)
					continue;
				Vec3<float> currentPosition = position;
				Vec3<float> landingPadPosition = aboveDest;

				float distance = glm::length(currentPosition - landingPadPosition);

				if (distance < shortestPathCost)
					shortestPathPad = aboveDest;
			}

			LogInfo("Vehicle mission %s: Pathing to pad at %s", name, shortestPathPad);
			auto *gotoMission = VehicleMission::gotoLocation(state, v, shortestPathPad);
			v.missions.emplace_front(gotoMission);
			gotoMission->start(state, v);
			return;
		}
		case MissionType::InfiltrateSubvert:
		{
			switch (missionCounter)
			{
				// Goto location above building, when there, deposit aliens
				case 0:
				{
					auto name = this->getName();
					auto b = this->targetBuilding;
					if (!b)
					{
						LogError("Building disappeared");
						return;
					}
					auto vehicleTile = v.tileObject;
					if (takeOffCheck(state, v, name))
					{
						return;
					}

					auto &map = vehicleTile->map;

					auto pos = v.tileObject->getPosition();

					// If not yet above building - goto location above it
					if (pos.x < b->bounds.p0.x || pos.x >= b->bounds.p1.x ||
					    pos.y < b->bounds.p0.y || pos.y >= b->bounds.p1.y)
					{
						Vec3<int> goodPos{0, 0, 0};
						std::set<Vec2<int>> rooftop;
						// Choose highest layer above building, choose middle tile within that
						for (int z = map.size.z - 1; z > 0; z--)
						{
							bool foundObject = false;
							for (int x = b->bounds.p0.x; x < b->bounds.p1.x; x++)
							{
								for (int y = b->bounds.p0.y; y < b->bounds.p1.y; y++)
								{
									auto t = map.getTile(x, y, z);
									if (!t->ownedObjects.empty())
									{
										foundObject = true;
										rooftop.emplace(x, y);
									}
								}
							}
							if (foundObject)
							{
								int xPos = 0;
								int yPos = 0;
								for (auto &vec : rooftop)
								{
									xPos += vec.x;
									yPos += vec.y;
								}
								xPos /= (int)rooftop.size();
								yPos /= (int)rooftop.size();
								// ensure object is within roof
								if (rooftop.find({xPos, yPos}) == rooftop.end())
								{
									xPos = (*rooftop.begin()).x;
									yPos = (*rooftop.begin()).y;
								}

								goodPos = {xPos, yPos, z};
								break;
							}
						}
						if (goodPos.z != 0)
						{
							goodPos.z = glm::min(goodPos.z + 1, map.size.z - 1);

							LogInfo("Vehicle mission %s: Pathing to infiltration spot %s", name,
							        goodPos);
							auto *gotoMission =
							    VehicleMission::gotoLocation(state, v, goodPos, true);
							v.missions.emplace_front(gotoMission);
							gotoMission->start(state, v);
							return;
						}
						else
						{
							LogError("Mission %s: Can't find a spot to infiltrate from", name);
							v.missions.emplace_back(VehicleMission::gotoPortal(state, v));
							missionCounter = 2;
						}
					}

					LogInfo("Vehicle mission %s: Infiltrating from {%f,%f,%f}", name, pos.x, pos.y,
					        pos.z);

					// If arrived to a location above building, deposit aliens or subvert
					// FIXME: Handle subversion
					if (subvert)
						LogError("Implement subversion!");
					auto doodad = v.city->placeDoodad(
					    StateRef<DoodadType>{&state, "DOODAD_14_INFILTRATION_BIG"},
					    v.tileObject->getPosition() - Vec3<float>{0, 0, 0.5f});

					auto *snoozeMission = VehicleMission::snooze(state, v, doodad->lifetime * 2);
					v.missions.emplace_front(snoozeMission);
					snoozeMission->start(state, v);
					missionCounter++;
					return;
				}
				// Deposited aliens, place aliens in building and retreat
				// FIXME: Actually Deposit aliens in building or subvert
				case 1:
				{
					// retreat
					v.missions.emplace_back(VehicleMission::gotoPortal(state, v));
					missionCounter++;
					return;
				}
				default:
					LogError("Starting a completed Infiltration mission?");
					return;
			}
		}
		default:
			LogWarning("TODO: Implement start");
			return;
	}
}

void VehicleMission::setPathTo(GameState &state, Vehicle &v, Vec3<int> target, int maxIterations,
                               bool checkValidity, bool giveUpIfInvalid)
{
	auto vehicleTile = v.tileObject;
	if (vehicleTile)
	{
		auto &map = vehicleTile->map;
		auto to = map.getTile(target);

		if (checkValidity)
		{
			// Check if target tile has no scenery permanently blocking it
			// If it does, go up until we've got clear sky
			while (true)
			{
				bool containsScenery = false;
				for (auto &obj : to->ownedObjects)
				{
					if (obj->getType() == TileObject::Type::Scenery)
					{
						auto sceneryTile = std::static_pointer_cast<TileObjectScenery>(obj);
						if (sceneryTile->scenery.lock()->type->isLandingPad)
						{
							continue;
						}
						containsScenery = true;
						break;
					}
				}
				if (!containsScenery)
				{
					break;
				}
				if (giveUpIfInvalid)
				{
					targetLocation = vehicleTile->getPosition();
					return;
				}
				LogInfo("Cannot move to %d %d %d, contains scenery that is not a landing pad",
				        target.x, target.y, target.z);
				if (type == MissionType::Crash)
				{
					LogError("Crashing into inaccessible tile?");
				}
				target.z++;
				if (target.z >= map.size.z)
				{
					LogError("No space in the sky? Reached %d %d %d", target.x, target.y, target.z);
					return;
				}
				to = map.getTile(target);
			}
			// Check if target tile has no vehicle termporarily blocking it
			// If it does, find a random location around it that is not blocked
			bool containsVehicle = false;
			for (auto &obj : to->ownedObjects)
			{
				if (obj->getType() == TileObject::Type::Vehicle)
				{
					containsVehicle = true;
					break;
				}
			}
			if (containsVehicle)
			{
				// How far to deviate from target point
				int maxDiff = 2;
				// Calculate bounds
				int midX = target.x;
				int dX = midX + maxDiff + 1 > map.size.x
				             ? map.size.x - maxDiff - 1
				             : (midX - maxDiff < 0 ? maxDiff - midX : 0);
				midX += dX;
				int midY = target.y;
				int dY = midY + maxDiff + 1 > map.size.x
				             ? map.size.x - maxDiff - 1
				             : (midY - maxDiff < 0 ? maxDiff - midY : 0);
				midY += dY;
				int midZ = (int)v.altitude;
				int dZ = midZ + maxDiff + 1 > map.size.x
				             ? map.size.x - maxDiff - 1
				             : (midZ - maxDiff < 0 ? maxDiff - midZ : 0);
				midZ += dZ;

				if (pickNearest)
				{
					Vec3<int> newTarget;
					bool foundNewTarget = false;
					for (int i = 0; i <= maxDiff && !foundNewTarget; i++)
					{
						for (int x = midX - i; x <= midX + i && !foundNewTarget; x++)
						{
							for (int y = midY - i; y <= midY + i && !foundNewTarget; y++)
							{
								for (int z = midZ - i; z <= midZ + i && !foundNewTarget; z++)
								{
									// Only pick points on the edge of each iteration
									if (x == midX - i || x == midX + i || y == midY - i ||
									    y == midY + i || z == midZ - i || z == midZ + i)
									{
										auto t = map.getTile(x, y, z);
										if (t->ownedObjects.empty())
										{
											newTarget = t->position;
											foundNewTarget = true;
											break;
										}
									}
								}
							}
						}
					}
					if (foundNewTarget)
					{
						LogWarning(
						    "Target %d,%d,%d was unreachable, found new closest target %d,%d,%d",
						    target.x, target.y, target.z, newTarget.x, newTarget.y, newTarget.z);
						target = newTarget;
					}
				}
				else
				{
					std::list<Vec3<int>> sideStepLocations;

					for (int x = midX - maxDiff; x <= midX + maxDiff; x++)
					{
						for (int y = midY - maxDiff; y <= midY + maxDiff; y++)
						{
							for (int z = midZ - maxDiff; z <= midZ + maxDiff; z++)
							{
								auto t = map.getTile(x, y, z);
								if (t->ownedObjects.empty())
								{
									sideStepLocations.push_back(t->position);
								}
							}
						}
					}
					if (!sideStepLocations.empty())
					{
						auto newTarget = listRandomiser(state.rng, sideStepLocations);
						LogWarning(
						    "Target %d,%d,%d was unreachable, found new random target %d,%d,%d",
						    target.x, target.y, target.z, newTarget.x, newTarget.y, newTarget.z);
						target = newTarget;
					}
				}
			}
		}

		auto path = map.findShortestPath(vehicleTile->getOwningTile()->position, target,
		                                 maxIterations, FlyingVehicleTileHelper{map, v});

		// Always start with the current position
		this->currentPlannedPath.push_back(vehicleTile->getOwningTile()->position);
		for (auto &p : path)
		{
			this->currentPlannedPath.push_back(p);
		}
	}
	else
	{
		LogError("Mission %s: Take off before pathfinding!", this->getName());
	}
}

bool VehicleMission::advanceAlongPath(GameState &state, Vec3<float> &dest, Vehicle &v)
{
	// Add {0.5,0.5,0.5} to make it route to the center of the tile
	static const Vec3<float> offset{0.5f, 0.5f, 0.5f};
	static const Vec3<float> offsetLand{0.5f, 0.5f, 0.0f};

	if (currentPlannedPath.empty())
		return false;
	currentPlannedPath.pop_front();
	if (currentPlannedPath.empty())
		return false;
	auto pos = currentPlannedPath.front();

	// Land/TakeOff mission does not check for collision or path skips
	if (type == MissionType::Land)
	{
		dest = Vec3<float>{pos.x, pos.y, pos.z} + offsetLand;
		return true;
	}
	if (type == MissionType::TakeOff)
	{
		dest = Vec3<float>{pos.x, pos.y, pos.z} + offset;
		return true;
	}

	// See if we can actually go there
	auto tFrom = v.tileObject->getOwningTile();
	auto tTo = tFrom->map.getTile(pos);
	if (tFrom->position != pos &&
	    (std::abs(tFrom->position.x - pos.x) > 1 || std::abs(tFrom->position.y - pos.y) > 1 ||
	     std::abs(tFrom->position.z - pos.z) > 1 ||
	     !FlyingVehicleTileHelper{tFrom->map, v}.canEnterTile(tFrom, tTo)))
	{
		// Next tile became impassable, pick a new path
		currentPlannedPath.clear();
		v.missions.emplace_front(restartNextMission(state, v));
		v.missions.front()->start(state, v);
		return false;
	}

	// See if we can make a shortcut
	// When ordering move to vehidle already on the move, we can have a situation
	// where going directly to 2nd step in the path is faster than going to the first
	// In this case, we should skip unnesecary steps
	auto it = ++currentPlannedPath.begin();
	// Start with position after next
	// If next position has a node and we can go directly to that node
	// Then update current position and iterator
	while (it != currentPlannedPath.end() &&
	       (tFrom->position == *it ||
	        (std::abs(tFrom->position.x - it->x) <= 1 && std::abs(tFrom->position.y - it->y) <= 1 &&
	         std::abs(tFrom->position.z - it->z) <= 1 &&
	         FlyingVehicleTileHelper{tFrom->map, v}.canEnterTile(tFrom, tFrom->map.getTile(*it)))))
	{
		currentPlannedPath.pop_front();
		pos = currentPlannedPath.front();
		tTo = tFrom->map.getTile(pos);
		it = ++currentPlannedPath.begin();
	}

	dest = Vec3<float>{pos.x, pos.y, pos.z} + offset;
	return true;
}

bool VehicleMission::isTakingOff(Vehicle &v)
{
	return type == MissionType::TakeOff && currentPlannedPath.size() > 2 &&
	       (v.position.z - ((int)v.position.z)) <= 0.5f;
}

UString VehicleMission::getName()
{
	static const std::map<VehicleMission::MissionType, UString> TypeMap = {
	    {MissionType::GotoLocation, "GotoLocation"},
	    {MissionType::GotoBuilding, "GotoBuilding"},
	    {MissionType::GotoPortal, "GotoBuilding"},
	    {MissionType::FollowVehicle, "FollowVehicle"},
	    {MissionType::AttackVehicle, "AttackVehicle"},
	    {MissionType::AttackBuilding, "AttackBuilding"},
	    {MissionType::Snooze, "Snooze"},
	    {MissionType::TakeOff, "TakeOff"},
	    {MissionType::Land, "Land"},
	    {MissionType::Crash, "Crash"},
	    {MissionType::Patrol, "Patrol"},
	    {MissionType::InfiltrateSubvert, "Infiltrate/Subvert"},
	    {MissionType::RestartNextMission, "RestartNextMission"},
	};
	UString name = "UNKNOWN";
	const auto it = TypeMap.find(this->type);
	if (it != TypeMap.end())
		name = it->second;
	switch (this->type)
	{
		case MissionType::GotoLocation:
			name += format(" %s", this->targetLocation.x);
			break;
		case MissionType::GotoBuilding:
			name += " " + this->targetBuilding.id;
			break;
		case MissionType::FollowVehicle:
			name += " " + this->targetVehicle.id;
			break;
		case MissionType::AttackBuilding:
			name += " " + this->targetBuilding.id;
			break;
		case MissionType::Snooze:
			name += format(" for %u ticks", this->timeToSnooze);
			break;
		case MissionType::TakeOff:
			name += " from " + this->targetBuilding.id;
			break;
		case MissionType::Land:
			name += " in " + this->targetBuilding.id;
			break;
		case MissionType::Crash:
			name += format(" landing on %s", this->targetLocation);
			break;
		case MissionType::AttackVehicle:
			name += format(" target \"%s\"", this->targetVehicle.id);
			break;
		case MissionType::Patrol:
			name += format(" %s", this->targetLocation.x);
			break;
		case MissionType::GotoPortal:
			name += format(" %s", this->targetLocation.x);
			break;
		case MissionType::InfiltrateSubvert:
			name += " " + this->targetBuilding.id + " " + (subvert ? "subvert" : "infiltrate");
			break;
		case MissionType::RestartNextMission:
			break;
	}
	return name;
}

} // namespace OpenApoc

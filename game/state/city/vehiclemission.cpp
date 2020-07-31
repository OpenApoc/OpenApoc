#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include "game/state/city/vehiclemission.h"
#include "framework/configfile.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "framework/sound.h"
#include "game/state/city/base.h"
#include "game/state/city/building.h"
#include "game/state/city/city.h"
#include "game/state/city/scenery.h"
#include "game/state/city/vehicle.h"
#include "game/state/gameevent.h"
#include "game/state/gamestate.h"
#include "game/state/rules/city/citycommonsamplelist.h"
#include "game/state/rules/city/scenerytiletype.h"
#include "game/state/rules/city/vehicletype.h"
#include "game/state/shared/doodad.h"
#include "game/state/shared/organisation.h"
#include "game/state/tilemap/collision.h"
#include "game/state/tilemap/tilemap.h"
#include "game/state/tilemap/tileobject_doodad.h"
#include "game/state/tilemap/tileobject_scenery.h"
#include "game/state/tilemap/tileobject_shadow.h"
#include "game/state/tilemap/tileobject_vehicle.h"
#include "library/strings_format.h"
#include <glm/glm.hpp>

namespace OpenApoc
{

using AdjustTargetResult = VehicleTargetHelper::AdjustTargetResult;
using VehicleAvoidance = VehicleTargetHelper::VehicleAvoidance;
using Reachability = VehicleTargetHelper::Reachability;

namespace
{
// Add {0.5,0.5,0.5} to make it route to the center of the tile
static const Vec3<float> offsetFlying{0.5f, 0.5f, 0.5f};
static const Vec3<float> offsetLandInto{0.5f, 0.5f, 0.01f};
static const Vec3<float> offsetLaunch{0.5f, 0.5f, -1.0f};

// Self-destruct timer for UFOs.
// division /5 because need to round to 5 mins
// TODO: find a way how to extract from the game data
static const std::map<UString, std::pair<unsigned, unsigned>> selfDestructTimer = {
    {"VEHICLETYPE_ALIEN_PROBE", {10 / 5 * TICKS_PER_MINUTE, 90 / 5 * TICKS_PER_MINUTE}},
    {"VEHICLETYPE_ALIEN_SCOUT", {10 / 5 * TICKS_PER_MINUTE, 90 / 5 * TICKS_PER_MINUTE}},
    {"VEHICLETYPE_ALIEN_TRANSPORTER", {15 / 5 * TICKS_PER_MINUTE, 120 / 5 * TICKS_PER_MINUTE}},
    {"VEHICLETYPE_ALIEN_FAST_ATTACK_SHIP", {15 / 5 * TICKS_PER_MINUTE, 120 / 5 * TICKS_PER_MINUTE}},
    {"VEHICLETYPE_ALIEN_DESTROYER", {15 / 5 * TICKS_PER_MINUTE, 120 / 5 * TICKS_PER_MINUTE}},
    {"VEHICLETYPE_ALIEN_ASSAULT_SHIP", {15 / 5 * TICKS_PER_MINUTE, 180 / 5 * TICKS_PER_MINUTE}},
    {"VEHICLETYPE_ALIEN_BOMBER", {10 / 5 * TICKS_PER_MINUTE, 120 / 5 * TICKS_PER_MINUTE}},
    {"VEHICLETYPE_ALIEN_ESCORT", {15 / 5 * TICKS_PER_MINUTE, 120 / 5 * TICKS_PER_MINUTE}},
    {"VEHICLETYPE_ALIEN_BATTLESHIP", {60 / 5 * TICKS_PER_MINUTE, 240 / 5 * TICKS_PER_MINUTE}},
    {"VEHICLETYPE_ALIEN_MOTHERSHIP", {60 / 5 * TICKS_PER_MINUTE, 240 / 5 * TICKS_PER_MINUTE}}};
} // namespace

FlyingVehicleTileHelper::FlyingVehicleTileHelper(TileMap &map, const Vehicle &v)
    : map(map), v(v), altitude((int)v.altitude)
{
	int xMax = 0;
	int yMax = 0;
	for (const auto &s : v.type->size)
	{
		xMax = std::max(xMax, s.second.x);
		yMax = std::max(yMax, s.second.y);
	}
	size = {xMax, yMax};
	large = size.x > 1 || size.y > 1;
}

bool FlyingVehicleTileHelper::canEnterTile(Tile *from, Tile *to, bool ignoreStaticUnits,
                                           bool ignoreMovingUnits, bool ignoreAllUnits) const
{
	float nothing;
	bool none1;
	bool none2;
	return canEnterTile(from, to, false, none1, nothing, none2, ignoreStaticUnits,
	                    ignoreMovingUnits, ignoreAllUnits);
}

float FlyingVehicleTileHelper::pathOverheadAlloawnce() const { return 1.25f; }

// Support 'from' being nullptr for if a vehicle is being spawned in the map
bool FlyingVehicleTileHelper::canEnterTile(Tile *from, Tile *to, bool, bool &, float &cost, bool &,
                                           bool, bool, bool) const
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
		LogError("FromPos == ToPos %s", toPos);
		return false;
	}
	if (!map.tileIsValid(toPos))
	{
		LogError("ToPos %s is not on the map", toPos);
		return false;
	}

	// We cannot move into underground tile unless we're checking if it's passable (no "from")
	// We only enter that if we want to land into building
	// and in that case passability is ignored anyways
	if (from && toPos.z == 0)
	{
		return false;
	}

	// Allow pathing into tiles with crashes
	bool foundCrash = false;
	bool foundScenery = false;
	const auto self = v.shared_from_this();
	for (auto &obj : to->intersectingObjects)
	{
		if (obj->getType() == TileObject::Type::Vehicle)
		{
			auto vehicleTile = std::static_pointer_cast<TileObjectVehicle>(obj);
			// Large vehicles span multiple tiles
			if (vehicleTile->getVehicle() == self)
			{
				continue;
			}
			// Non-crashed can go into crashed
			bool vehicleCrashed = vehicleTile->getVehicle()->crashed;
			if (v.crashed || !vehicleCrashed)
			{
				return false;
			}
			if (vehicleCrashed)
			{
				foundCrash = true;
			}
		}
	}
	for (auto &obj : to->ownedObjects)
	{
		if (!foundScenery && obj->getType() == TileObject::Type::Scenery)
		{
			auto sceneryTile = std::static_pointer_cast<TileObjectScenery>(obj);
			if (sceneryTile->scenery.lock()->type->isLandingPad)
			{
				continue;
			}
			foundScenery = true;
		}
	}
	if (foundScenery && !foundCrash)
	{
		return false;
	}

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

float FlyingVehicleTileHelper::adjustCost(Vec3<int> nextPosition, int z) const
{
	if ((nextPosition.z < altitude && z == 1) || (nextPosition.z > altitude && z == -1) ||
	    nextPosition.z == altitude)
	{
		return -0.5f;
	}
	return 0;
}

float FlyingVehicleTileHelper::getDistance(Vec3<float> from, Vec3<float> to) const
{
	return getDistanceStatic(from, to);
}

float FlyingVehicleTileHelper::getDistance(Vec3<float> from, Vec3<float> toStart,
                                           Vec3<float> toEnd) const
{
	return getDistanceStatic(from, toStart, toEnd);
}

float FlyingVehicleTileHelper::getDistanceStatic(Vec3<float> from, Vec3<float> to)
{
	return glm::length(to - from);
}

float FlyingVehicleTileHelper::getDistanceStatic(Vec3<float> from, Vec3<float> toStart,
                                                 Vec3<float> toEnd)
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

bool FlyingVehicleTileHelper::canLandOnTile(Tile *to) const
{
	if (!to)
	{
		LogError("No 'to' position supplied");
		return false;
	}
	Vec3<int> toPos = to->position;

	for (int y = 0; y < size.y; y++)
	{
		for (int x = 0; x < size.x; x++)
		{
			for (auto &obj : map.getTile(x + toPos.x, y + toPos.y, toPos.z)->ownedObjects)
			{
				if (obj->getType() == TileObject::Type::Scenery)
				{
					auto scenery = std::static_pointer_cast<TileObjectScenery>(obj)->scenery.lock();
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

Vec3<int> FlyingVehicleTileHelper::findTileToLandOn(GameState &, sp<TileObjectVehicle> vTile) const
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

Vec3<float> FlyingVehicleTileHelper::findSidestep(GameState &state, sp<TileObjectVehicle> vTile,
                                                  sp<TileObjectVehicle> targetTile,
                                                  float distancePref) const
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

		if (static_cast<int>(newPosition.z) < altitude)
		{
			newPosition.z += 1;
		}
		else if (static_cast<int>(newPosition.z) > altitude)
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

VehicleMission *VehicleMission::gotoLocation(GameState &state, Vehicle &v, Vec3<int> target,
                                             bool allowTeleporter, bool pickNearest,
                                             int attemptsToGiveUpAfter)
{
	// TODO
	// Pseudocode:
	// if (in building)
	// 	prepend(TakeOff)
	// routeClosestICanTo(target);
	auto *mission = new VehicleMission();
	mission->type = MissionType::GotoLocation;
	mission->pickNearest = pickNearest;
	mission->reRouteAttempts = attemptsToGiveUpAfter;
	mission->allowTeleporter = allowTeleporter;
	if (!VehicleTargetHelper::adjustTargetToClosest(state, v, target, VehicleAvoidance::Ignore,
	                                                true)
	         .foundSuitableTarget)
	{
		mission->cancelled = true;
	}
	mission->targetLocation = target;
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
	else
	{
		target = pickRandom(state.rng, v.city->portals)->position;
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

VehicleMission *VehicleMission::departToSpace(GameState &state, Vehicle &v)
{
	auto *mission = new VehicleMission();
	mission->type = MissionType::DepartToSpace;
	mission->pickNearest = true;
	mission->targetLocation = getRandomMapEdgeCoordinates(state, v.city);
	return mission;
}

VehicleMission *VehicleMission::gotoBuilding(GameState &, Vehicle &v, StateRef<Building> target,
                                             bool allowTeleporter)
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
	mission->targetBuilding = target ? target : v.homeBuilding;
	mission->allowTeleporter = allowTeleporter;
	return mission;
}

VehicleMission *VehicleMission::infiltrateOrSubvertBuilding(GameState &, Vehicle &, bool subvert,
                                                            StateRef<Building> target)
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
	mission->attackCrashed = target->crashed;
	return mission;
}

VehicleMission *VehicleMission::attackBuilding(GameState &state [[maybe_unused]],
                                               Vehicle &v [[maybe_unused]],
                                               StateRef<Building> target)
{
	auto *mission = new VehicleMission();
	mission->type = MissionType::AttackBuilding;
	mission->targetBuilding = target;
	return mission;
}

VehicleMission *VehicleMission::followVehicle(GameState &, Vehicle &, StateRef<Vehicle> target)
{
	auto *mission = new VehicleMission();
	mission->type = MissionType::FollowVehicle;
	mission->targetVehicle = target;
	return mission;
}

VehicleMission *VehicleMission::followVehicle(GameState &, Vehicle &,
                                              std::list<StateRef<Vehicle>> &targets)
{
	auto *mission = new VehicleMission();
	mission->type = MissionType::FollowVehicle;
	if (!targets.empty())
	{
		mission->targetVehicle = targets.front();
		mission->targets = targets;
		mission->targets.pop_front();
	}
	return mission;
}

VehicleMission *VehicleMission::recoverVehicle(GameState &state [[maybe_unused]],
                                               Vehicle &v [[maybe_unused]],
                                               StateRef<Vehicle> target)
{
	auto *mission = new VehicleMission();
	mission->type = MissionType::RecoverVehicle;
	mission->targetVehicle = target;
	return mission;
}

VehicleMission *VehicleMission::offerService(GameState &state [[maybe_unused]],
                                             Vehicle &v [[maybe_unused]], StateRef<Building> target)
{
	auto *mission = new VehicleMission();
	mission->type = MissionType::OfferService;
	if (target)
	{
		mission->targetBuilding = target;
	}
	else
	{
		mission->missionCounter = 1;
	}
	return mission;
}

VehicleMission *VehicleMission::snooze(GameState &, Vehicle &, unsigned int snoozeTicks)
{
	auto *mission = new VehicleMission();
	mission->type = MissionType::Snooze;
	mission->timeToSnooze = snoozeTicks;
	return mission;
}

VehicleMission *VehicleMission::selfDestruct(GameState &state, Vehicle &v)
{
	unsigned timer = TICKS_PER_HOUR;
	auto timerUFO = selfDestructTimer.find(v.type.id);
	if (timerUFO != selfDestructTimer.cend())
	{
		timer = 5 * std::uniform_int_distribution<unsigned>(timerUFO->second.first,
		                                                    timerUFO->second.second)(state.rng);
	}
	auto *mission = new VehicleMission();
	mission->type = MissionType::SelfDestruct;
	mission->timeToSnooze = timer;
	return mission;
}

VehicleMission *VehicleMission::arriveFromDimensionGate(GameState &state, Vehicle &v, int ticks)
{
	auto *mission = new VehicleMission();
	mission->type = MissionType::ArriveFromDimensionGate;
	// find max delay arrival and increment
	int lastTicks = -DIMENSION_GATE_DELAY;
	for (auto &v2 : state.vehicles)
	{
		if (v2.second->city == v.city && v2.second->owner == v.owner)
		{
			for (auto &m : v2.second->missions)
			{
				if (m->type == MissionType::ArriveFromDimensionGate)
				{
					lastTicks = std::max(lastTicks, (int)m->timeToSnooze);
				}
			}
		}
	}
	if (lastTicks == -DIMENSION_GATE_DELAY)
	{
		lastTicks += ticks;
	}
	mission->timeToSnooze = lastTicks + DIMENSION_GATE_DELAY;
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

VehicleMission *VehicleMission::patrol(GameState &, Vehicle &, bool home, unsigned int counter)
{
	auto *mission = new VehicleMission();
	mission->type = MissionType::Patrol;
	mission->missionCounter = counter;
	mission->patrolHome = home;
	return mission;
}

VehicleMission *VehicleMission::teleport(GameState &state [[maybe_unused]],
                                         Vehicle &v [[maybe_unused]], Vec3<int> target)
{
	auto *mission = new VehicleMission();
	mission->type = MissionType::Teleport;
	mission->targetLocation = target;
	return mission;
}

VehicleMission *VehicleMission::takeOff(Vehicle &v)
{
	if (!v.currentBuilding)
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

VehicleMission *VehicleMission::investigateBuilding(GameState &, Vehicle &v [[maybe_unused]],
                                                    StateRef<Building> target, bool allowTeleporter)
{
	auto *mission = new VehicleMission();
	mission->type = MissionType::InvestigateBuilding;
	mission->targetBuilding = target;
	mission->allowTeleporter = allowTeleporter;
	return mission;
}

AdjustTargetResult VehicleTargetHelper::adjustTargetToClosestRoad(Vehicle &v, Vec3<int> &target)
{
	auto &map = *v.city->map;
	auto reachability = isReachableTargetRoad(v, target);
	// Clicked on road
	if (reachability == Reachability::Reachable)
	{
		return {reachability, true};
	}
	// Try to find road in general vicinity of +-10 tiles
	Vec3<int> closestPos = target;
	int closestDist = INT_MAX;
	for (int x = std::max(0, target.x - 10); x <= std::min(map.size.x - 1, target.x + 10); x++)
	{
		for (int y = std::max(0, target.y - 10); y <= std::min(map.size.y - 1, target.y + 10); y++)
		{
			for (int z = 0; z < map.size.z; z++)
			{
				auto reachabilityHere = isReachableTargetRoad(v, {x, y, z});
				if (reachabilityHere == Reachability::Reachable)
				{
					int dist = std::abs(target.x - x) + std::abs(target.y - y) +
					           std::abs(target.z - z) / 2;
					if (dist < closestDist)
					{
						closestDist = dist;
						closestPos = {x, y, z};
					}
				}
			}
		}
	}
	if (closestDist < INT_MAX)
	{
		target = closestPos;
		return {reachability, true};
	}
	// Try to find road anywhere
	for (int x = 0; x < map.size.x; x++)
	{
		for (int y = 0; y < map.size.y; y++)
		{
			// Already checked
			if ((std::abs(target.x - x) <= 10) && (std::abs(target.y - y) <= 10))
			{
				continue;
			}
			for (int z = 0; z < map.size.z; z++)
			{
				auto reachabilityHere = isReachableTargetRoad(v, {x, y, z});
				if (reachabilityHere == Reachability::Reachable)
				{
					int dist = std::abs(target.x - x) + std::abs(target.y - y) +
					           std::abs(target.z - z) / 2;
					if (dist < closestDist)
					{
						closestDist = dist;
						closestPos = {x, y, z};
					}
				}
			}
		}
	}
	if (closestDist < INT_MAX)
	{
		target = closestPos;
		return {reachability, true};
	}
	LogWarning("No road exists anywhere in the city? Really?");
	return {reachability, false};
}

AdjustTargetResult VehicleTargetHelper::adjustTargetToClosestGround(Vehicle &v, Vec3<int> &target)
{
	auto &map = *v.city->map;
	auto reachability = isReachableTargetGround(v, target);
	// Clicked on reachable ground
	if (reachability == Reachability::Reachable)
	{
		return {reachability, true};
	}
	// Try to find ground in general vicinity of +-10 tiles
	Vec3<int> closestPos = target;
	int closestDist = INT_MAX;
	for (int x = std::max(0, target.x - 10); x <= std::min(map.size.x - 1, target.x + 10); x++)
	{
		for (int y = std::max(0, target.y - 10); y <= std::min(map.size.y - 1, target.y + 10); y++)
		{
			for (int z = 0; z < map.size.z; z++)
			{
				auto reachabilityHere = isReachableTargetGround(v, {x, y, z});
				if (reachabilityHere == Reachability::Reachable)
				{
					int dist = std::abs(target.x - x) + std::abs(target.y - y) +
					           std::abs(target.z - z) / 2;
					if (dist < closestDist)
					{
						closestDist = dist;
						closestPos = {x, y, z};
					}
				}
			}
		}
	}
	if (closestDist < INT_MAX)
	{
		target = closestPos;
		return {reachability, true};
	}
	// Try to find ground anywhere
	for (int x = 0; x < map.size.x; x++)
	{
		for (int y = 0; y < map.size.y; y++)
		{
			// Already checked
			if ((std::abs(target.x - x) <= 10) && (std::abs(target.y - y) <= 10))
			{
				continue;
			}
			for (int z = 0; z < map.size.z; z++)
			{
				auto reachabilityHere = isReachableTargetGround(v, {x, y, z});
				if (reachabilityHere == Reachability::Reachable)
				{
					int dist = std::abs(target.x - x) + std::abs(target.y - y) +
					           std::abs(target.z - z) / 2;
					if (dist < closestDist)
					{
						closestDist = dist;
						closestPos = {x, y, z};
					}
				}
			}
		}
	}
	if (closestDist < INT_MAX)
	{
		target = closestPos;
		return {reachability, true};
	}
	LogError("NO GROUND IN THE CITY!? WTF?");
	return {reachability, false};
}

AdjustTargetResult
VehicleTargetHelper::adjustTargetToClosestFlying(GameState &state, Vehicle &v, Vec3<int> &target,
                                                 const VehicleAvoidance vehicleAvoidance)
{
	auto &map = *v.city->map;
	auto reachability = isReachableTargetFlying(v, target);
	// Clicked on reachable tile
	if (reachability == Reachability::Reachable ||
	    (reachability == Reachability::BlockedByVehicle &&
	     vehicleAvoidance == VehicleAvoidance::Ignore))
	{
		return {Reachability::Reachable, true};
	}

	// Check if target tile has no scenery permanently blocking it
	// If it does, go up until we've got clear sky
	while (reachability == Reachability::BlockedByScenery)
	{
		target.z++;
		if (target.z >= map.size.z)
		{
			LogError("No space in the sky? Reached %d %d %d", target.x, target.y, target.z);
			return {reachability, false};
		}
		reachability = isReachableTargetFlying(v, target);
		if (reachability == Reachability::Reachable ||
		    (reachability == Reachability::BlockedByVehicle &&
		     vehicleAvoidance == VehicleAvoidance::Ignore))
		{
			return {Reachability::BlockedByScenery, true};
		}
	}

	// Find a random location around the blocking vehicle that is not blocked

	// How far to deviate from target point
	int maxDiff = 2;
	// Calculate bounds
	int midX = target.x;
	midX = midX + maxDiff + 1 > map.size.x ? map.size.x - maxDiff - 1
	                                       : (midX - maxDiff < 0 ? maxDiff : midX);
	int midY = target.y;
	midY = midY + maxDiff + 1 > map.size.y ? map.size.y - maxDiff - 1
	                                       : (midY - maxDiff < 0 ? maxDiff : midY);
	int midZ = (int)v.altitude;
	midZ = midZ + maxDiff + 1 > map.size.z ? map.size.z - maxDiff - 1
	                                       : (midZ - maxDiff < 0 ? maxDiff : midZ);

	if (vehicleAvoidance == VehicleAvoidance::PickNearbyPoint)
	{
		Vec3<int> newTarget;
		bool foundNewTarget = false;
		for (int i = 0; i <= maxDiff && !foundNewTarget; i++)
		{
			for (int x = midX - i; x <= midX + i && !foundNewTarget; x++)
			{
				for (int y = midY - i; y <= midY + i && !foundNewTarget; y++)
				{
					for (int z = midZ - i; z <= midZ + i; z++)
					{
						// Only pick points on the edge of each iteration
						if (x == midX - i || x == midX + i || y == midY - i || y == midY + i ||
						    z == midZ - i || z == midZ + i)
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
			LogWarning("Target %d,%d,%d was unreachable, found new closest target %d,%d,%d",
			           target.x, target.y, target.z, newTarget.x, newTarget.y, newTarget.z);
			target = newTarget;
		}
	}
	else if (vehicleAvoidance == VehicleAvoidance::Sidestep)
	{
		std::list<Vec3<int>> sideStepLocations;

		for (int x = midX - maxDiff; x <= midX + maxDiff; x++)
		{
			for (int y = midY - maxDiff; y <= midY + maxDiff; y++)
			{
				for (int z = midZ - maxDiff; z <= midZ + maxDiff; z++)
				{
					if (!map.tileIsValid(x, y, z))
					{
						continue;
					}
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
			auto newTarget = pickRandom(state.rng, sideStepLocations);
			LogWarning("Target %s was unreachable, side-stepping to  %s.", target, newTarget);
			target = newTarget;
		}
	}
	else
	{
		LogError("Unknown value for vehicleAvoidance: %d", static_cast<int>(vehicleAvoidance));
	}
	return {reachability, true};
}

AdjustTargetResult
VehicleTargetHelper::adjustTargetToClosest(GameState &state, Vehicle &v, Vec3<int> &target,
                                           const VehicleAvoidance vehicleAvoidance,
                                           bool adjustForFlying)
{
	switch (v.type->type)
	{
		case VehicleType::Type::Road:
			return adjustTargetToClosestRoad(v, target);
		case VehicleType::Type::ATV:
			return adjustTargetToClosestGround(v, target);
		default:
			LogError("Vehicle [%s] has unknown type [%s]", v.name, v.type->name);
			[[fallthrough]];
		case VehicleType::Type::Flying:
		case VehicleType::Type::UFO:
			if (!adjustForFlying)
			{
				// Just report given target as reachable.
				return {Reachability::Reachable, true};
			}
			return adjustTargetToClosestFlying(state, v, target, vehicleAvoidance);
	}
}

Reachability VehicleTargetHelper::isReachableTarget(const Vehicle &v, Vec3<int> target)
{
	switch (v.type->type)
	{
		case VehicleType::Type::Road:
			return isReachableTargetRoad(v, target);
		case VehicleType::Type::ATV:
			return isReachableTargetGround(v, target);
		default:
			LogError("Vehicle [%s] has unknown type [%s]", v.name, v.type->name);
			[[fallthrough]];
		case VehicleType::Type::Flying:
		case VehicleType::Type::UFO:
			return isReachableTargetFlying(v, target);
	}
}

Reachability VehicleTargetHelper::isReachableTargetFlying(const Vehicle &v, Vec3<int> target)
{
	auto &map = *v.city->map;
	auto targetTile = map.getTile(target);

	// Check if target tile has no scenery permanently blocking it
	for (auto &obj : targetTile->ownedObjects)
	{
		if (obj->getType() == TileObject::Type::Scenery)
		{
			auto sceneryTile = std::static_pointer_cast<TileObjectScenery>(obj);
			if (!sceneryTile->scenery.lock()->type->isLandingPad)
			{
				return Reachability::BlockedByScenery;
			}
		}
	}

	// Check if target tile has no vehicle temporarily blocking it
	for (auto &obj : targetTile->intersectingObjects)
	{
		if (obj->getType() == TileObject::Type::Vehicle)
		{
			auto otherVehicle = std::static_pointer_cast<TileObjectVehicle>(obj)->getVehicle();
			// Don't collide with self.
			if (v.name == otherVehicle->name)
				continue;
			// Non-crashed can go into crashed
			if (v.crashed || !otherVehicle->crashed)
			{
				return Reachability::BlockedByVehicle;
			}
		}
	}

	return Reachability::Reachable;
}

Reachability VehicleTargetHelper::isReachableTargetRoad(const Vehicle &v, Vec3<int> target)
{
	auto &map = *v.city->map;
	auto scenery = map.getTile(target)->presentScenery;

	// Check if target tile is a road
	if (scenery && scenery->type->tile_type == SceneryTileType::TileType::Road)
	{
		return Reachability::Reachable;
	}

	return Reachability::BlockedByScenery;
}

Reachability VehicleTargetHelper::isReachableTargetGround(const Vehicle &v, Vec3<int> target)
{
	auto &map = *v.city->map;
	auto scenery = map.getTile(target)->presentScenery;

	// Check if target tile can be accessed by an ATV
	if (scenery)
	{
		auto walkMode = scenery->type->getATVMode();
		switch (walkMode)
		{
			case SceneryTileType::WalkMode::Onto:
			{
				// If moving to an "onto" tile must have clear above
				auto checkedPos = target + Vec3<int>(0, 0, 1);
				if (map.tileIsValid(checkedPos))
				{
					auto checkedTile = map.getTile(checkedPos);
					if (checkedTile->presentScenery)
					{
						break;
					}
				}
			}
			// Intentional fall-through
			case SceneryTileType::WalkMode::Into:
				return Reachability::Reachable;
			case SceneryTileType::WalkMode::None:
				break;
		}
	}

	return Reachability::BlockedByScenery;
}

bool VehicleMission::takeOffCheck(GameState &state, Vehicle &v)
{
	if (!v.tileObject)
	{
		if (v.currentBuilding)
		{
			v.addMission(state, VehicleMission::takeOff(v));
			return true;
		}
		else
		{
			return false;
		}
	}
	return false;
}

bool VehicleMission::teleportCheck(GameState &state, Vehicle &v)
{
	if (allowTeleporter && v.canTeleport() &&
	    (std::abs(targetLocation.x - (int)v.position.x) > TELEPORTER_SPREAD ||
	     std::abs(targetLocation.y - (int)v.position.y) > TELEPORTER_SPREAD))
	{
		v.addMission(state, VehicleMission::teleport(state, v, targetLocation));
		return true;
	}
	return false;
}

bool VehicleMission::getNextDestination(GameState &state, Vehicle &v, Vec3<float> &destPos,
                                        float &destFacing, int &turboTiles)
{
	static const std::set<TileObject::Type> sceneryVehicleSet = {TileObject::Type::Scenery,
	                                                             TileObject::Type::Vehicle};

	if (cancelled)
	{
		return false;
	}
	switch (this->type)
	{
		case MissionType::TakeOff:      // Fall-through
		case MissionType::GotoLocation: // Fall-through
		case MissionType::Crash:
		case MissionType::Land:
		case MissionType::Patrol:
		case MissionType::AttackBuilding:
		{
			return advanceAlongPath(state, v, destPos, destFacing, turboTiles);
		}
		case MissionType::FollowVehicle:
		{
			auto vTile = v.tileObject;
			auto targetTile = this->targetVehicle->tileObject;

			if (vTile && targetTile)
			{
				setFollowPath(state, v);
				if (!currentPlannedPath.empty())
				{
					return advanceAlongPath(state, v, destPos, destFacing, turboTiles);
				}
			}
			return false;
		}
		case MissionType::AttackVehicle:
		{
			auto vTile = v.tileObject;
			auto targetTile = this->targetVehicle->tileObject;

			if (vTile && targetTile)
			{
				if (v.type->isGround())
				{
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
						{
							currentPlannedPath.clear();
						}

						// Face target to fire
						Vec2<float> targetFacingVector = {targetVehicle->position.x - v.position.x,
						                                  targetVehicle->position.y - v.position.y};
						if (targetFacingVector.x != 0 || targetFacingVector.y != 0)
						{
							targetFacingVector = glm::normalize(targetFacingVector);
							float a1 = acosf(-targetFacingVector.y);
							float a2 = asinf(targetFacingVector.x);
							float angleToTarget = a2 >= 0 ? a1 : 2.0f * (float)M_PI - a1;
							// Bring angle to one of directional alignments
							int angleToTargetInt = angleToTarget / (float)M_PI * 4.0f + 0.5f;
							angleToTarget = (float)angleToTargetInt * (float)M_PI / 4.0f;
							if (destFacing != angleToTarget)
							{
								LogWarning("Vehicle %s facing target", v.name);
								destFacing = angleToTarget;
								return true;
							}
						}
					}
					else
					{
						setFollowPath(state, v);
					}
					if (!currentPlannedPath.empty())
					{
						return advanceAlongPath(state, v, destPos, destFacing, turboTiles);
					}
				}
				else
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

					// Have LOS if first thing we hit is target vehicle or not scenery
					bool haveLOS = true;
					auto hitObject =
					    vTile->map.findCollision(v.position, targetTile->getVoxelCentrePosition(),
					                             sceneryVehicleSet, vTile, true);
					if (hitObject)
					{
						if (hitObject.obj->getType() == TileObject::Type::Vehicle)
						{
							auto vehicle =
							    std::static_pointer_cast<TileObjectVehicle>(hitObject.obj);
							if (vehicle != targetTile)
							{
								haveLOS = false;
							}
						}
						else if (hitObject.obj->getType() == TileObject::Type::Scenery)
						{
							haveLOS = false;
						}
					}

					if (haveLOS && vTile->getDistanceTo(targetTile) < distancePreference)
					{
						// target is in range, we're done with pathing and start maneuvering
						if (!currentPlannedPath.empty())
						{
							currentPlannedPath.clear();
						}
						// FIXME: Use vehicle engagement rules here, for now just face target
						// with 33%
						// chance
						if (randBoundsExclusive(state.rng, 0, 100) < 33)
						{
							Vec2<float> targetFacingVector = {
							    targetVehicle->position.x - v.position.x,
							    targetVehicle->position.y - v.position.y};
							if (targetFacingVector.x != 0 || targetFacingVector.y != 0)
							{
								targetFacingVector = glm::normalize(targetFacingVector);
								float a1 = acosf(-targetFacingVector.y);
								float a2 = asinf(targetFacingVector.x);
								float angleToTarget = a2 >= 0 ? a1 : 2.0f * (float)M_PI - a1;
								// Bring angle to one of directional alignments
								int angleToTargetInt = angleToTarget / (float)M_PI * 4.0f + 0.5f;
								angleToTarget = (float)angleToTargetInt * (float)M_PI / 4.0f;
								if (destFacing != angleToTarget)
								{
									LogWarning("Vehicle %s facing target", v.name);
									destFacing = angleToTarget;
									return true;
								}
							}
						}
						auto newPosition =
						    tileHelper.findSidestep(state, vTile, targetTile, distancePreference);
						if (newPosition != vTile->getPosition())
						{
							destPos = newPosition;
							return true;
						}
						return false;
					}
					else
					{
						setFollowPath(state, v);
					}
					if (!currentPlannedPath.empty())
					{
						currentPlannedPath.pop_front();
						if (currentPlannedPath.empty())
							return false;
						auto pos = currentPlannedPath.front();
						destPos = Vec3<float>{pos.x, pos.y, pos.z}
						          // Add {0.5,0.5,0.5} to make it route to the center of the tile
						          + Vec3<float>{0.5, 0.5, 0.5};
						return true;
					}
				}
			}
			return false;
		}
		case MissionType::GotoBuilding:
		case MissionType::InvestigateBuilding:
		{
			if (v.currentBuilding != this->targetBuilding)
			{
				auto name = this->getName();
				LogError("Vehicle mission %s: getNextDestination() shouldn't be called unless "
				         "you've reached the target?",
				         name);
			}
			destPos = {0, 0, 9};
			return false;
		}
		case MissionType::Snooze:
		case MissionType::SelfDestruct:
		case MissionType::ArriveFromDimensionGate:
		case MissionType::RestartNextMission:
		case MissionType::GotoPortal:
		case MissionType::DepartToSpace:
		case MissionType::OfferService:
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
		// Pick attack target
		case MissionType::Patrol:
		{
			if (finished)
			{
				return;
			}
			float range = v.getFiringRange();
			if (v.tileObject && range > 0)
			{
				auto enemy = v.findClosestEnemy(state, v.tileObject);
				if (enemy && v.tileObject->getDistanceTo(enemy) < range)
				{
					StateRef<Vehicle> vehicleRef(&state, enemy->getVehicle());
					currentPlannedPath.clear();
					v.addMission(state, VehicleMission::attackVehicle(state, v, vehicleRef));
				}
			}
			return;
		}
		// Port out or path to portal
		case MissionType::GotoPortal:
		{
			if (cancelled)
			{
				return;
			}
			if (finished)
			{
				// Cannot port out?
				if (!v.hasDimensionShifter())
				{
					if (config().getBool("OpenApoc.NewFeature.CrashingDimensionGate"))
					{
						v.velocity = v.type->directionToVector(v.direction) * v.getSpeed();
						v.crash(state, nullptr);
					}
					else
					{
						takePositionNearPortal(state, v);
					}
				}
				else // Port out
				{
					// Update score for UFO incursion
					if (v.owner == state.getAliens() && v.city.id == "CITYMAP_HUMAN")
					{
						int incursionScore = -v.type->score / 4;
						state.weekScore.incursions += incursionScore;
						state.totalScore.incursions += incursionScore;
					}

					for (auto &city : state.cities)
					{
						if (city.second != v.city.getSp())
						{
							v.enterDimensionGate(state);
							v.city = {&state, city.second};
							if (v.owner == state.getPlayer())
							{
								// Delay for returning / going in
								int delay = 0;
								if (city.first == "CITYMAP_HUMAN")
								{
									delay = randBoundsInclusive(state.rng, TICKS_PER_HOUR,
									                            2 * TICKS_PER_HOUR);
								}
								else
								{
									delay = randBoundsInclusive(state.rng, TICKS_PER_SECOND,
									                            2 * TICKS_PER_SECOND);
								}
								v.addMission(
								    state, VehicleMission::arriveFromDimensionGate(state, v, delay),
								    true);
							}
							else
							{
								v.equipDefaultEquipment(state);
								v.leaveDimensionGate(state);
								std::uniform_int_distribution<int> xyPos(20, 120);
								v.setPosition(
								    {xyPos(state.rng), xyPos(state.rng), v.city->size.z - 1});
							}
							break;
						}
					}
				}
				// Cancelling as otherwise we can be in another dimension when we update
				cancelled = true;
				return;
			}
			return;
		}
		// Port out or path to space
		case MissionType::DepartToSpace:
		{
			if (finished)
			{
				v.die(state, true);
				return;
			}
			takeOffCheck(state, v);
			return;
		}
		// Move to the ground spot
		case MissionType::Crash:
		{
			if (finished)
			{
				if (v.city.id == "CITYMAP_HUMAN")
				{
					fw().pushEvent(new GameVehicleEvent(GameEventType::UfoCrashed,
					                                    {&state, v.shared_from_this()}));
				}
				else
				{
					v.die(state, false);
				}
				return;
			}
			auto vTile = v.tileObject;
			if (vTile && !finished && this->currentPlannedPath.empty())
			{
				LogWarning("Crash landing failed, restartng...");
				v.addMission(state, restartNextMission(state, v));
			}
			return;
		}
		case MissionType::GotoLocation:
		case MissionType::Land:
		{
			return;
		}
		case MissionType::InvestigateBuilding:
		{
			if (finished && v.owner == state.getPlayer() && v.currentBuilding->detected)
			{
				v.currentBuilding->decreasePendingInvestigatorCount(state);
			}
			return;
		}
		case MissionType::InfiltrateSubvert:
		case MissionType::GotoBuilding:
		case MissionType::OfferService:
		case MissionType::AttackVehicle:
		case MissionType::FollowVehicle:
		case MissionType::RestartNextMission:
		case MissionType::TakeOff:
		case MissionType::RecoverVehicle:
		case MissionType::AttackBuilding:
			return;
		case MissionType::SelfDestruct:
			if (finished)
			{
				v.die(state);
				return;
			}
			updateTimer(ticks);
			return;
		case MissionType::ArriveFromDimensionGate:
			if (missionCounter == 0 && timeToSnooze == 0)
			{
				v.addMission(state, restartNextMission(state, v));
			}
			updateTimer(ticks);
			return;
		case MissionType::Snooze:
			updateTimer(ticks);
			return;
		default:
			LogWarning("TODO: Implement update");
			return;
	}
} // namespace OpenApoc

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

bool VehicleMission::isFinishedInternal(GameState &state, Vehicle &v)
{
	if (cancelled)
	{
		return true;
	}
	switch (this->type)
	{
		case MissionType::GotoLocation:
		case MissionType::Crash:
		// Note that GotoPortal/DepartToSpace never has planned path but still checks for target
		// loc
		case MissionType::DepartToSpace:
		case MissionType::GotoPortal:
		{
			auto vTile = v.tileObject;
			if (vTile && this->currentPlannedPath.empty() &&
			    (pickedNearest || vTile->getOwningTile()->position == this->targetLocation))
			{
				return true;
			}
			return false;
		}
		case MissionType::AttackVehicle:
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
				LogInfo("Vehicle attack mission: Target not on the map");
				return true;
			}
			if (!attackCrashed && (t->crashed || t->sliding || t->falling))
			{
				return true;
			}
			return this->targetVehicle->health == 0;
		}
		case MissionType::FollowVehicle:
		{
			// Target was destroyed / lost tileobject
			while (!targets.empty() &&
			       (!targetVehicle || !targetVehicle->tileObject || targetVehicle->crashed ||
			        this->targetVehicle->health == 0 || targetVehicle->city != v.city))
			{
				targetVehicle = targets.front();
				targets.pop_front();
			}
			if (!targetVehicle || !targetVehicle->tileObject || targetVehicle->crashed ||
			    this->targetVehicle->health == 0 || targetVehicle->city != v.city)
			{
				return true;
			}
			return false;
		}
		case MissionType::TakeOff:
			return v.tileObject && this->currentPlannedPath.empty();
		case MissionType::Land:
			return missionCounter > 1;
		case MissionType::InfiltrateSubvert:
			return missionCounter > 1;
		case MissionType::OfferService:
			return missionCounter > 1;
		case MissionType::ArriveFromDimensionGate:
			return missionCounter > 0;
		case MissionType::RecoverVehicle:
			return missionCounter > 0;
		case MissionType::Patrol:
			return this->missionCounter == 0 && this->currentPlannedPath.empty();
		case MissionType::GotoBuilding:
		case MissionType::InvestigateBuilding:
			return this->targetBuilding == v.currentBuilding;
		case MissionType::SelfDestruct:
		case MissionType::Snooze:
			return this->timeToSnooze == 0;
		case MissionType::RestartNextMission:
		case MissionType::Teleport:
			return true;
		case MissionType::AttackBuilding:
			return targetBuilding && !targetBuilding->isAlive();
		default:
			LogWarning("TODO: Implement isFinishedInternal");
			return false;
	}
}

void VehicleMission::start(GameState &state, Vehicle &v)
{
	switch (this->type)
	{
		case MissionType::Teleport:
		{
			if (!v.canTeleport())
			{
				return;
			}
			auto &map = *state.current_city->map;
			auto canEnter = FlyingVehicleTileHelper(map, v);
			Vec3<int> targetTile = {-1, -1, -1};
			bool found = false;
			for (int i = 0; i < 100; i++)
			{
				// Random teleportation
				if (targetLocation.x == -1)
				{
					targetTile = {randBoundsExclusive(state.rng, 0, map.size.x),
					              randBoundsExclusive(state.rng, 0, map.size.y),
					              map.size.z + randBoundsInclusive(
					                               state.rng, -std::min(TELEPORTER_SPREAD, 5), -1)};
				}
				// Targeted teleportation
				else
				{
					targetTile = {
					    targetLocation.x +
					        randBoundsInclusive(state.rng, -TELEPORTER_SPREAD, TELEPORTER_SPREAD),
					    targetLocation.y +
					        randBoundsInclusive(state.rng, -TELEPORTER_SPREAD, TELEPORTER_SPREAD),
					    map.size.z +
					        randBoundsInclusive(state.rng, -std::min(TELEPORTER_SPREAD, 5), -1)};
				}
				if (!map.tileIsValid(targetTile))
				{
					targetTile.x = targetTile.x < 0 ? 0 : map.size.x - 1;
					targetTile.y = targetTile.y < 0 ? 0 : map.size.y - 1;
				}
				if (canEnter.canEnterTile(nullptr, map.getTile(targetTile)))
				{
					found = true;
					break;
				}
			}
			if (!found)
			{
				targetTile = v.position;
			}
			if (v.currentBuilding)
			{
				v.leaveBuilding(state, targetTile);
			}
			else
			{
				v.facing = v.goalFacing;
				v.ticksToTurn = 0;
				v.angularVelocity = 0.0f;
				v.setPosition((Vec3<float>)targetTile + offsetFlying);
				v.goalPosition = v.position;
				v.velocity = {0.0f, 0.0f, 0.0f};
				v.updateSprite(state);
			}
			if (state.city_common_sample_list->teleport)
			{
				fw().soundBackend->playSample(state.city_common_sample_list->teleport,
				                              v.getPosition());
			}
			return;
		}
		case MissionType::TakeOff:
		{
			if (v.tileObject)
			{
				return;
			}
			auto b = v.currentBuilding;
			if (!b)
			{
				LogError("Building disappeared");
				return;
			}

			auto &map = *b->city->map;
			unsigned snoozeTicks = TICKS_PER_SECOND / 2;
			if (v.type->isGround())
			{
				// Looking for free exit
				do // just so we could use continue to exit
				{
					auto exitLocation = b->carEntranceLocation;
					auto exitTile = map.getTile(exitLocation);
					if (!exitTile)
					{
						LogError("Invalid entrance location %s - outside map?", exitLocation.x);
						snoozeTicks = TICKS_PER_HOUR / 2;
						continue;
					}
					auto scenery = exitTile->presentScenery;
					if (!scenery)
					{
						LogInfo("Tried exit %s - destroyed", exitLocation);
						snoozeTicks = TICKS_PER_HOUR / 2;
						continue;
					}
					bool exitIsBusy = false;
					for (auto &obj : exitTile->ownedObjects)
					{
						if (obj->getType() == TileObject::Type::Vehicle)
						{
							auto v = std::static_pointer_cast<TileObjectVehicle>(obj)->getVehicle();
							if (v->crashed)
							{
								// If the doors are blocked by crashed vehicle then need more time
								// to solve the problem.
								snoozeTicks = TICKS_PER_HOUR / 2;
							}
							exitIsBusy = true;
							break;
						}
					}
					if (exitIsBusy)
					{
						continue;
					}
					LogInfo("Launching vehicle from building \"%s\" at exit %s", b.id,
					        exitLocation);
					Vec3<float> leaveLocation = exitTile->getRestingPosition();
					float facing = 0.0f;
					if (scenery->type->connection[0])
					{
						facing = 0.0f;
						leaveLocation += Vec3<float>{0.0f, 0.49f, 0.0f};
					}
					else if (scenery->type->connection[1])
					{
						facing = (float)M_PI_2;
						leaveLocation += Vec3<float>{-0.49f, 0.0f, 0.0f};
					}
					else if (scenery->type->connection[2])
					{
						facing = (float)M_PI;
						leaveLocation += Vec3<float>{0.0f, -0.49f, 0.0f};
					}
					else if (scenery->type->connection[3])
					{
						facing = (float)M_PI + (float)M_PI_2;
						leaveLocation += Vec3<float>{0.49f, 0.0f, 0.0f};
					}
					v.leaveBuilding(state, leaveLocation, facing);
					LogInfo("Launching vehicle from building \"%s\" at pad %s", b.id,
					        leaveLocation);
					this->currentPlannedPath = {exitLocation, exitLocation};
					return;
				} while (false);
			}
			else
			{
				// Looking for free pad
				for (auto &padLocation : b->landingPadLocations)
				{
					auto padTile = map.getTile(padLocation);
					// Pad destroyed
					if (!padTile->presentScenery || !padTile->presentScenery->type->isLandingPad)
					{
						continue;
					}
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
					belowPadLocation += offsetLaunch;
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
					v.leaveBuilding(state, belowPadLocation);
					return;
				}
			}
			LogInfo("No free exit in building \"%s\" free - waiting", b.id);
			v.addMission(state, snooze(state, v, snoozeTicks));
			return;
		}
		case MissionType::Land:
		{
			auto b = this->targetBuilding;
			if (!b)
			{
				LogError("Building disappeared");
				cancelled = true;
				return;
			}
			switch (missionCounter)
			{
				case 0:
				{
					auto vehicleTile = v.tileObject;
					if (!vehicleTile)
					{
						LogError("Trying to land vehicle not in the city?");
						return;
					}
					if (v.type->isGround())
					{
						auto vehiclePosition = vehicleTile->getOwningTile()->position;
						if (vehiclePosition != targetBuilding->carEntranceLocation)
						{
							LogError("Vehicle at %s not inside vehicle entrance for building %s",
							         vehiclePosition, b.id);
							return;
						}
						// No action required for ground vehicles, they just disappear into the
						// depot
						v.enterBuilding(state, b);
						missionCounter = 2;
						return;
					}
					else
					{
						auto vehiclePosition = vehicleTile->getOwningTile()->position;
						if (vehiclePosition.z < 2)
						{
							LogError("Vehicle trying to land off bottom of map %s",
							         vehiclePosition);
							cancelled = true;
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
							LogError("Vehicle at %s not directly above a landing pad for "
							         "building %s",
							         vehiclePosition, b.id);
							cancelled = true;
							return;
						}
						this->currentPlannedPath = {padPosition, padPosition - Vec3<int>{0, 0, 1}};
						missionCounter++;
						return;
					}
					return;
				}
				case 1:
				{
					// Flying vehicle reached landing spot
					v.enterBuilding(state, b);
					missionCounter++;
					return;
				}
				case 2:
				{
					LogError("Starting a complete Land mission?");
					return;
				}
				default:
				{
					LogError("Unhandled missionCounter in %s", getName());
					return;
				}
			}
		}
		case MissionType::DepartToSpace:
			if (v.type->isGround())
			{
				LogError("Ground vehcile on depart to space mission!? WTF!?");
			}
		// Intentional fall-through
		case MissionType::GotoPortal:
		{
			if (v.type->isGround())
			{
				cancelled = true;
				return;
			}
			if (isFinishedInternal(state, v))
			{
				return;
			}
			if (!isFinished(state, v))
			{
				LogInfo("Vehicle mission %s: Pathing to %s", getName(), targetLocation);
				v.addMission(state, VehicleMission::gotoLocation(state, v, targetLocation,
				                                                 allowTeleporter, pickNearest));
			}
			return;
		}
		case MissionType::GotoLocation:
		{
			if (isFinishedInternal(state, v))
			{
				return;
			}
			if (teleportCheck(state, v))
			{
				return;
			}
			if (takeOffCheck(state, v))
			{
				return;
			}
			else
			{
				if (this->currentPlannedPath.empty())
				{
					int iterationCount = getDefaultIterationCount(v);
					iterationCount *=
					    glm::length((Vec3<float>)targetLocation - v.position) < 33 ? 3 : 1;
					iterationCount *=
					    glm::length((Vec3<float>)targetLocation - v.position) < 11 ? 3 : 1;
					setPathTo(state, v, targetLocation, iterationCount, true, reRouteAttempts == 0);
				}
			}
			return;
		}
		case MissionType::Patrol:
		{
			if (isFinishedInternal(state, v))
			{
				return;
			}
			if (takeOffCheck(state, v))
			{
				return;
			}
			else
			{
				if (this->currentPlannedPath.empty())
				{
					if (missionCounter == 0)
					{
						return;
					}
					missionCounter--;

					if (patrolHome)
					{
						std::uniform_int_distribution<int> xPos(v.homeBuilding->bounds.p0.x - 5,
						                                        v.homeBuilding->bounds.p1.x + 5);
						std::uniform_int_distribution<int> yPos(v.homeBuilding->bounds.p0.y - 5,
						                                        v.homeBuilding->bounds.p1.y + 5);
						setPathTo(state, v,
						          v.getPreferredPosition(xPos(state.rng), yPos(state.rng)),
						          getDefaultIterationCount(v));
					}
					else
					{
						std::uniform_int_distribution<int> xyPos(25, 115);
						setPathTo(state, v,
						          v.getPreferredPosition(xyPos(state.rng), xyPos(state.rng)),
						          getDefaultIterationCount(v));
					}
				}
			}
			return;
		}
		case MissionType::AttackBuilding:
		{
			if (!targetBuilding)
			{
				if (!acquireTargetBuilding(state, v))
				{
					cancelled = true;
					return;
				}
			}
			if (takeOffCheck(state, v))
			{
				return;
			}
			else
			{
				if (this->currentPlannedPath.empty())
				{
					std::uniform_int_distribution<int> xPos(targetBuilding->bounds.p0.x - 5,
					                                        targetBuilding->bounds.p1.x + 5);
					std::uniform_int_distribution<int> yPos(targetBuilding->bounds.p0.y - 5,
					                                        targetBuilding->bounds.p1.y + 5);
					setPathTo(state, v, v.getPreferredPosition(xPos(state.rng), yPos(state.rng)),
					          getDefaultIterationCount(v));
				}
			}
			return;
		}
		case MissionType::Crash:
		{
			if (v.type->type != VehicleType::Type::UFO)
			{
				LogError("Only UFOs can crash, how did we end up starting this?");
				cancelled = true;
				return;
			}
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
				v.addMission(
				    state, VehicleMission::gotoLocation(state, v, randomNearbyPos, false, true, 1));
				return;
			}
			this->targetLocation = tile;
			setPathTo(state, v, tile, getDefaultIterationCount(v));
			return;
		}
		case MissionType::FollowVehicle:
		case MissionType::AttackVehicle:
		{
			auto name = this->getName();
			LogInfo("Vehicle mission %s checking state", name);
			if (isFinished(state, v))
			{
				LogInfo("Vehicle mission %s became finished", name);
				return;
			}
			auto t = this->targetVehicle;
			if (v.shared_from_this() == t.getSp())
			{
				LogError("Vehicle mission %s: Targeting itself", name);
				return;
			}
			auto vehicleTile = v.tileObject;
			if (takeOffCheck(state, v))
			{
				return;
			}
			setFollowPath(state, v);
			return;
		}
		case MissionType::InvestigateBuilding:
		case MissionType::GotoBuilding:
		{
			if (isFinishedInternal(state, v))
			{
				return;
			}
			auto name = this->getName();
			LogInfo("Vehicle mission %s checking state", name);
			auto b = this->targetBuilding;
			if (!b)
			{
				LogError("Building disappeared");
				return;
			}
			if (b == v.currentBuilding)
			{
				LogInfo("Vehicle mission %s: Already at building", name);
				return;
			}
			if (allowTeleporter)
			{
				targetLocation = {(targetBuilding->bounds.p0.x + targetBuilding->bounds.p1.x) / 2,
				                  (targetBuilding->bounds.p0.y + targetBuilding->bounds.p1.y) / 2,
				                  0};
				if (teleportCheck(state, v))
				{
					return;
				}
			}
			// Leave building
			if (takeOffCheck(state, v))
			{
				return;
			}
			// Actually go there
			auto vehicleTile = v.tileObject;
			if (v.type->isGround())
			{
				/* Am I already in a car depot? If so land */
				auto position = vehicleTile->getOwningTile()->position;
				if (position == targetBuilding->carEntranceLocation)
				{
					LogInfo("Mission %s: Entering depot on entrance %s", name, position);
					v.addMission(state, VehicleMission::land(v, b));
					return;
				}

				LogInfo("Vehicle mission %s: Pathing to entrance at %s", name,
				        b->carEntranceLocation);
				v.addMission(state, VehicleMission::gotoLocation(state, v, b->carEntranceLocation,
				                                                 allowTeleporter, false, 1));
				// If we can't path to building then cancel
				auto &gotoMission = v.missions.front();
				if (gotoMission->cancelled || gotoMission->currentPlannedPath.empty() ||
				    gotoMission->currentPlannedPath.back() == position)
				{
					cancelled = true;
				}
				return;
			}
			else
			{
				/* Am I already above a landing pad? If so land */
				auto position = vehicleTile->getOwningTile()->position;
				LogInfo("Vehicle mission %s: at position %s", name, position);
				for (auto &padLocation : b->landingPadLocations)
				{
					auto padTile = b->city->map->getTile(padLocation);
					// Pad destroyed
					if (!padTile->presentScenery || !padTile->presentScenery->type->isLandingPad)
					{
						continue;
					}
					auto abovePadLocation = padLocation;
					abovePadLocation.z += 1;
					if (abovePadLocation == position)
					{
						LogInfo("Mission %s: Landing on pad %s", name, padLocation);
						v.addMission(state, VehicleMission::land(v, b));
						return;
					}
				}
				// I must be in the air and not above a pad - try to find the closest pad
				Vec3<int> shortestPathPad = {0, 0, 0};
				float shortestPathCost = FLT_MAX;

				for (int attempt = 1; attempt <= 2; attempt++)
				{
					// At first attempt: path to unoccupied pad
					// At second attempt: path to any pad
					for (auto &dest : b->landingPadLocations)
					{
						auto padTile = b->city->map->getTile(dest);
						// Pad destroyed?
						if (!padTile->presentScenery ||
						    !padTile->presentScenery->type->isLandingPad)
						{
							continue;
						}
						// Pad occupied? (only on first attempt)
						if (attempt == 1)
						{
							bool occupied = false;
							for (auto &o : padTile->intersectingObjects)
							{
								if (o->getType() == TileObject::Type::Vehicle)
								{
									auto vehicle = std::static_pointer_cast<TileObjectVehicle>(o)
									                   ->getVehicle();
									if (!vehicle->crashed)
									{
										occupied = true;
										break;
									}
								}
							}
							if (occupied)
							{
								continue;
							}
						}

						// We actually want the tile above the pad itself
						auto aboveDest = dest;
						aboveDest.z += 1;
						if (position == aboveDest)
							continue;
						Vec3<float> currentPosition = position;
						Vec3<float> landingPadPosition = aboveDest;

						float distance = glm::length(currentPosition - landingPadPosition);

						if (distance < shortestPathCost)
						{
							shortestPathCost = distance;
							shortestPathPad = aboveDest;
						}
					}
					if (shortestPathCost != FLT_MAX)
					{
						break;
					}
				}
				if (shortestPathCost != FLT_MAX)
				{
					LogInfo("Vehicle mission %s: Pathing to pad at %s", name, shortestPathPad);
					v.addMission(state, VehicleMission::gotoLocation(state, v, shortestPathPad,
					                                                 allowTeleporter, false, 1));
					// If we can't path to building's pad then snooze
					auto &gotoMission = v.missions.front();
					if (gotoMission->cancelled || gotoMission->currentPlannedPath.empty() ||
					    gotoMission->currentPlannedPath.back() == position)
					{
						v.addMission(state, VehicleMission::snooze(state, v, TICKS_PER_SECOND));
					}
				}
				else
				{
					LogInfo("Vehicle mission %s: No pads, pathing to random location near building",
					        name);
					std::uniform_int_distribution<int> xPos(targetBuilding->bounds.p0.x - 2,
					                                        targetBuilding->bounds.p1.x + 2);
					std::uniform_int_distribution<int> yPos(targetBuilding->bounds.p0.y - 2,
					                                        targetBuilding->bounds.p1.y + 2);
					v.addMission(state,
					             VehicleMission::gotoLocation(
					                 state, v,
					                 v.getPreferredPosition(xPos(state.rng), yPos(state.rng)),
					                 allowTeleporter, true, 1));
				}
				return;
			}
		}
		case MissionType::RecoverVehicle:
		{
			if (!targetVehicle || !VehicleMission::canRecoverVehicle(state, v, *targetVehicle))
			{
				cancelled = true;
				return;
			}
			// Try to advance on vehicle
			switch (missionCounter)
			{
				case 0:
				{
					// Vehicle has crashed successfully and we're on top of it
					if (targetVehicle->crashed &&
					    (Vec3<int>)v.position == (Vec3<int>)targetVehicle->position)
					{
						missionCounter++;

						StateRef<Vehicle> thisRef = {&state,
						                             Vehicle::getId(state, v.shared_from_this())};

						// Launch vehicle assault on aliens
						if (targetVehicle->owner == state.getAliens())
						{
							auto event = new GameVehicleEvent(GameEventType::UfoRecoveryBegin,
							                                  targetVehicle, thisRef);
							fw().pushEvent(event);

							// Mission will now begin
							return;
						}
						// Recovering crashed non-ufo
						else
						{
							if (targetVehicle->homeBuilding)
							{
								auto target = targetVehicle;
								v.addMission(state, VehicleMission::gotoBuilding(
								                        state, v, targetVehicle->homeBuilding));
								v.carriedVehicle = target;
								target->carriedByVehicle = thisRef;
								target->clearMissions(state, true);
								if (target->smokeDoodad)
								{
									target->smokeDoodad->remove(state);
									target->smokeDoodad.reset();
								}
							}
							else
							{
								targetVehicle->die(state, true);
							}
							return;
						}
					}
					// Route towards target
					v.addMission(state,
					             VehicleMission::gotoLocation(state, v, targetVehicle->position));
					return;
				}
				default:
					// Nothing to do, launched battle or rescued by now
					return;
			}
			return;
		}
		case MissionType::OfferService:
		{
			switch (missionCounter)
			{
				// Go to target building and pick up cargo
				case 0:
				{
					if (!targetBuilding)
					{
						LogError("Building disappeared");
						return;
					}
					// Already in target building - pick up cargo and restart mission to start
					// ferrying
					if (v.currentBuilding == targetBuilding)
					{
						missionCounter++;
						v.provideService(state, true);
						// Restart mission with counter = 1, start offering service
						v.addMission(state, VehicleMission::restartNextMission(state, v));
					}
					// Not in target building yet - move to it
					else
					{
						v.addMission(state, VehicleMission::gotoBuilding(state, v, targetBuilding));
					}
					return;
				}
				// Provide service
				case 1:
				{
					// Find our ferry destination
					auto destination = v.getServiceDestination(state);
					if (destination)
					{
						// We still have stuff to ferry
						// Wait a bit and go to destination
						v.addMission(state, VehicleMission::gotoBuilding(state, v, destination));
						if (v.owner != state.getPlayer())
						{
							v.addMission(state, VehicleMission::snooze(state, v, TICKS_PER_SECOND));
						}
					}
					else
					{
						// We finished ferrying
						// Wait a bit and return home
						missionCounter++;
						if (v.homeBuilding)
						{
							v.addMission(state, VehicleMission::gotoBuilding(state, v));
							if (v.owner != state.getPlayer())
							{
								v.addMission(state,
								             VehicleMission::snooze(state, v, TICKS_PER_SECOND));
							}
						}
						else
						{
							// Was a temporal ferry, vanish now
							v.die(state, true);
						}
					}
					return;
				}
				case 2:
				{
					return;
				}
				default:
				{
					LogError("Unhandled missionCounter in %s", getName());
					return;
				}
			}
		}
		case MissionType::InfiltrateSubvert:
		{
			if (!targetBuilding)
			{
				if (!acquireTargetBuilding(state, v))
				{
					cancelled = true;
					return;
				}
			}
			// Ground can't infiltrate
			if (v.type->isGround())
			{
				cancelled = true;
				return;
			}
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
					if (takeOffCheck(state, v))
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
							v.addMission(state, VehicleMission::gotoLocation(state, v, goodPos,
							                                                 false, true));
							return;
						}
						else
						{
							cancelled = true;
							return;
						}
					}

					LogInfo("Vehicle mission %s: Infiltrating from {%f,%f,%f}", name, pos.x, pos.y,
					        pos.z);

					// If arrived to a location above building, deposit aliens or subvert
					// FIXME: Handle subversion
					if (subvert)
					{
						auto doodad = v.city->placeDoodad(
						    StateRef<DoodadType>{&state, "DOODAD_11_SUBVERSION_BIG"},
						    v.tileObject->getPosition() - Vec3<float>{0, 0, 0.5f});
						v.addMission(state, VehicleMission::snooze(state, v, doodad->lifetime));
					}
					else
					{
						auto doodad = v.city->placeDoodad(
						    StateRef<DoodadType>{&state, "DOODAD_14_INFILTRATION_BIG"},
						    v.tileObject->getPosition() - Vec3<float>{0, 0, 0.5f});
						v.addMission(state, VehicleMission::snooze(state, v, doodad->lifetime));
					}
					missionCounter++;
					return;
				}
				// Played deposit animation fully, place aliens in building and retreat
				case 1:
				{
					// Deposit aliens or subvert
					if (subvert)
					{
						if (randBoundsExclusive(state.rng, 0, 100) < state.micronoidRainChance)
						{
							targetBuilding->owner->infiltrationValue = 200;
						}
					}
					else
					{
						// Deposit aliens
						for (auto &pair : v.type->crew_deposit)
						{
							targetBuilding->current_crew[pair.first] += pair.second;
						}
						if (targetBuilding->base)
						{
							fw().pushEvent(new GameDefenseEvent(GameEventType::DefendTheBase,
							                                    targetBuilding->base, v.owner));
						}
						else
						{
							targetBuilding->alienMovement(state);
						}
					}
					// Retreat
					v.addMission(state, VehicleMission::snooze(state, v, TICKS_PER_SECOND));
					missionCounter++;
					return;
				}
				case 2:
				{
					return;
				}
				default:
				{
					LogError("Unhandled missionCounter in %s", getName());
					return;
				}
			}
		}
		case MissionType::ArriveFromDimensionGate:
		{
			switch (missionCounter)
			{
				// When timer finished spawn from gate
				case 0:
				{
					if (timeToSnooze > 0)
					{
						return;
					}
					v.leaveDimensionGate(state);
					if (v.city.id == "CITYMAP_HUMAN" && v.owner == state.getPlayer())
					{
						v.addMission(state, VehicleMission::gotoBuilding(state, v));
					}
					else
					{
						takePositionNearPortal(state, v);
					}
					missionCounter++;
					return;
				}
				// Mission finished
				case 1:
				{
					return;
				}
				default:
				{
					LogError("Unhandled missionCounter in %s", getName());
					return;
				}
			}
		}
		case MissionType::SelfDestruct:
		{
			if (v.smokeDoodad)
			{
				LogError("Restarting self destruct?");
			}
			else
			{
				v.setCrashed(state);
			}
			return;
		}
		case MissionType::RestartNextMission:
		case MissionType::Snooze:
			// No setup
			return;
		default:
			LogError("TODO: Implement start %s", getName());
			return;
	}
}

void VehicleMission::setPathTo(GameState &state, Vehicle &v, Vec3<int> target, int maxIterations,
                               bool checkValidity, bool giveUpIfInvalid)
{
	currentPlannedPath.clear();
	auto vehicleTile = v.tileObject;
	if (!vehicleTile)
	{
		LogError("Mission %s: Take off before pathfinding!", this->getName());
		return;
	}
	const auto avoidance =
	    pickNearest ? VehicleAvoidance::PickNearbyPoint : VehicleAvoidance::Sidestep;
	auto adjustResult =
	    VehicleTargetHelper::adjustTargetToClosest(state, v, target, avoidance, checkValidity);
	if (!adjustResult.foundSuitableTarget)
	{
		cancelled = true;
		return;
	}
	// If destination is permanently unreachable, either give up or decrease reRouteAttempts.
	if (adjustResult.reachability == Reachability::BlockedByScenery)
	{
		if (giveUpIfInvalid)
		{
			cancelled = true;
			return;
		}
		else if (reRouteAttempts > 0)
		{
			reRouteAttempts--;
		}
	}
	// If our vehicle avoidance strategy is to pick a nearby point, and if a nearby point was in
	// fact picked, then allow the mission to finish upon reaching the nearby point.
	if (adjustResult.reachability == Reachability::BlockedByVehicle &&
	    avoidance == VehicleAvoidance::PickNearbyPoint)
	{
		pickedNearest = true;
	}

	std::list<Vec3<int>> path;
	float distance = 0.0f;
	auto position = vehicleTile->getOwningTile()->position;
	switch (v.type->type)
	{
		case VehicleType::Type::Road:
			path = v.city->findShortestPath(position, target,
			                                GroundVehicleTileHelper{*v.city->map, v});
			distance = GroundVehicleTileHelper::getDistanceStatic(position, target);
			break;
		case VehicleType::Type::ATV:
			path = v.city->map->findShortestPath(position, target, maxIterations,
			                                     GroundVehicleTileHelper{*v.city->map, v});
			distance = GroundVehicleTileHelper::getDistanceStatic(position, target);
			break;
		case VehicleType::Type::Flying:
		case VehicleType::Type::UFO:
			path = v.city->map->findShortestPath(position, target, maxIterations,
			                                     FlyingVehicleTileHelper{*v.city->map, v});
			distance = FlyingVehicleTileHelper::getDistanceStatic(position, target);
			break;
	}

	// Did not reach destination
	if (path.empty() || path.back() != target)
	{
		// If target was close enough to reach
		if (maxIterations > (int)distance)
		{
			// If told to give up - cancel mission
			if (giveUpIfInvalid)
			{
				cancelled = true;
				return;
			}
			// If not told to give up - subtract attempt
			else
			{
				if (reRouteAttempts > 0)
				{
					reRouteAttempts--;
				}
			}
		}
	}

	// Always start with the current position
	this->currentPlannedPath.push_back(vehicleTile->getOwningTile()->position);
	for (auto &p : path)
	{
		this->currentPlannedPath.push_back(p);
	}
}

void VehicleMission::setFollowPath(GameState &state, Vehicle &v)
{
	auto targetTile = targetVehicle->tileObject;

	if (type == MissionType::FollowVehicle)
	{
		if (v.type->isGround())
		{
			// If we're not aiming for target's location
			// or we're not at planned location and with empty path
			if (targetTile->getOwningTile()->position != this->targetLocation ||
			    (v.tileObject->getOwningTile()->position != targetLocation &&
			     currentPlannedPath.empty()))
			{
				// adjust the path if target moved or planned path didn't bring us to target
				this->targetLocation = targetTile->getOwningTile()->position;
				setPathTo(state, v, this->targetLocation, getDefaultIterationCount(v), false);
			}
		}
		else // isFlying
		{
			bool needRePath = false;
			// Do something if our target location is outside of range of target
			// Or we're not at planned location and with empty path
			if (glm::length((Vec3<float>)(targetTile->getOwningTile()->position -
			                              this->targetLocation)) > FOLLOW_RANGE ||
			    (v.tileObject->getOwningTile()->position != targetLocation &&
			     currentPlannedPath.empty()))
			{
				needRePath = true;
			}
			// Maneuver if path is empty and enemies nearby
			else if (currentPlannedPath.empty())
			{
				float range = v.getFiringRange();
				if (range > 0.0f)
				{
					auto enemy = v.findClosestEnemy(state, v.tileObject);
					if (enemy && enemy->getDistanceTo(v.position) < range)
					{
						needRePath = true;
					}
				}
			}
			// Pick a random point around target
			if (needRePath)
			{
				targetLocation = targetTile->getOwningTile()->position;
				targetLocation.x +=
				    randBoundsInclusive(state.rng, -FOLLOW_BOUNDS_XY, FOLLOW_BOUNDS_XY);
				targetLocation.y +=
				    randBoundsInclusive(state.rng, -FOLLOW_BOUNDS_XY, FOLLOW_BOUNDS_XY);
				targetLocation.z +=
				    randBoundsInclusive(state.rng, -FOLLOW_BOUNDS_Z, FOLLOW_BOUNDS_Z);
				targetLocation.x = clamp(targetLocation.x, 0, v.city->size.x - 1);
				targetLocation.y = clamp(targetLocation.y, 0, v.city->size.y - 1);
				targetLocation.z = clamp(targetLocation.z, 0, v.city->size.z - 1);
				setPathTo(state, v, targetLocation, getDefaultIterationCount(v), true);
			}
		}
	}
	else // Attack vehicle
	{
		if (v.type->isGround())
		{
			// If we're not aiming for target's location
			// or we're not at planned location and with empty path
			if (targetTile->getOwningTile()->position != this->targetLocation ||
			    (v.tileObject->getOwningTile()->position != targetLocation &&
			     currentPlannedPath.empty()))
			{
				// adjust the path if target moved or planned path didn't bring us to target
				this->targetLocation = targetTile->getOwningTile()->position;
				setPathTo(state, v, this->targetLocation, getDefaultIterationCount(v), false);
			}
		}
		else // isFlying
		{
			// If we're not aiming for target's location (adjusted by our preference)
			// or we're not at planned location and with empty path
			if (v.getPreferredPosition(targetTile->getOwningTile()->position) !=
			        this->targetLocation ||
			    (v.tileObject->getOwningTile()->position != targetLocation &&
			     currentPlannedPath.empty()))
			{
				// adjust the path if target moved or planned path didn't bring us to target
				targetLocation = v.getPreferredPosition(targetTile->getOwningTile()->position);
				setPathTo(state, v, this->targetLocation, getDefaultIterationCount(v), false);
			}
		}
	}
}

bool VehicleMission::advanceAlongPath(GameState &state, Vehicle &v, Vec3<float> &destPos,
                                      float &destFacing [[maybe_unused]], int &turboTiles)
{
	if (currentPlannedPath.empty())
	{
		v.addMission(state, restartNextMission(state, v));
		return false;
	}
	currentPlannedPath.pop_front();
	if (currentPlannedPath.empty())
	{
		v.addMission(state, restartNextMission(state, v));
		return false;
	}
	auto pos = currentPlannedPath.front();

	// Land/TakeOff mission does not check for collision or path skips
	if (type == MissionType::Land)
	{
		if (v.type->isGround())
		{
			destPos = v.city->map->getTile(pos)->getRestingPosition();
		}
		else
		{
			destPos = pos;
			destPos += offsetLandInto;
		}
		return true;
	}
	if (type == MissionType::TakeOff)
	{
		if (v.type->isGround())
		{
			destPos = v.city->map->getTile(pos)->getRestingPosition();
		}
		else
		{
			destPos = pos;
			destPos += offsetFlying;
		}
		return true;
	}

	// See if we can actually go there
	auto tFrom = v.tileObject->getOwningTile();
	auto tTo = tFrom->map.getTile(pos);
	if (tFrom->position != pos)
	{
		bool cantGo = std::abs(tFrom->position.x - pos.x) > 1 ||
		              std::abs(tFrom->position.y - pos.y) > 1 ||
		              std::abs(tFrom->position.z - pos.z) > 1;
		if (v.type->isGround())
		{
			cantGo = cantGo || !GroundVehicleTileHelper{tFrom->map, v}.canEnterTile(tFrom, tTo);
		}
		else
		{
			cantGo = cantGo || !FlyingVehicleTileHelper{tFrom->map, v}.canEnterTile(tFrom, tTo);
		}
		if (cantGo)
		{
			// Next tile became impassable, pick a new path
			currentPlannedPath.clear();
			v.addMission(state, restartNextMission(state, v));
			return false;
		}
	}

	// See if we can make a shortcut
	{
		// When ordering move to vehicle already on the move, we can have a situation
		// where going directly to 2nd step in the path is faster than going to the first
		// In this case, we should skip unnecessary steps
		auto it = ++currentPlannedPath.begin();
		// Start with position after next
		// If next position has a node and we can go directly to that node
		// Then update current position and iterator
		while (it != currentPlannedPath.end())
		{
			bool canSkip = tFrom->position == *it;
			if (!canSkip)
			{
				bool cantSkip = std::abs(tFrom->position.x - it->x) > 1 ||
				                std::abs(tFrom->position.y - it->y) > 1 ||
				                std::abs(tFrom->position.z - it->z) > 1;
				if (v.type->isGround())
				{
					cantSkip = cantSkip || !GroundVehicleTileHelper{tFrom->map, v}.canEnterTile(
					                           tFrom, tFrom->map.getTile(*it));
				}
				else
				{
					cantSkip = cantSkip || !FlyingVehicleTileHelper{tFrom->map, v}.canEnterTile(
					                           tFrom, tFrom->map.getTile(*it));
				}
				canSkip = canSkip || !cantSkip;
			}
			if (!canSkip)
			{
				break;
			}

			currentPlannedPath.pop_front();
			pos = currentPlannedPath.front();
			tTo = tFrom->map.getTile(pos);
			it = ++currentPlannedPath.begin();
		}
	}

	// Advance tiles in turbo mode
	while (turboTiles > 0 && currentPlannedPath.size() > 1)
	{
		auto it = ++currentPlannedPath.begin();
		if (v.type->isGround())
		{
			if (!GroundVehicleTileHelper{tFrom->map, v}.canEnterTile(tTo, tTo->map.getTile(*it)))
			{
				break;
			}
		}
		else
		{
			if (!FlyingVehicleTileHelper{tFrom->map, v}.canEnterTile(tTo, tTo->map.getTile(*it)))
			{
				break;
			}
		}

		currentPlannedPath.pop_front();
		pos = currentPlannedPath.front();
		tFrom = tTo;
		tTo = tFrom->map.getTile(pos);
		turboTiles--;
	}

	auto sceneryTo = tTo->presentScenery;
	if (v.type->isGround())
	{
		destPos = tTo->getRestingPosition();
	}
	else
	{
		// If there's a scenery there then we must be rescuing, go to the top
		if (sceneryTo)
		{
			destPos = tTo->getRestingPosition(false, true);
		}
		else
		{
			destPos = pos;
			destPos += offsetFlying;
		}
	}

	return true;
}

bool VehicleMission::isTakingOff(Vehicle &v)
{
	return type == MissionType::TakeOff && currentPlannedPath.size() > 2 &&
	       (v.position.z - ((int)v.position.z)) <= 0.5f;
}

int VehicleMission::getDefaultIterationCount(Vehicle &v)
{
	// Both ATVs and flyers pathfind to a smaller depth
	return (v.type->type == VehicleType::Type::Road) ? 100 : 50;
}

Vec3<float> VehicleMission::getRandomMapEdgeCoordinates(GameState &state, StateRef<City> city)
{
	if (randBool(state.rng))
	{
		std::uniform_int_distribution<int> xPos(0, city->size.x - 1);
		if (randBool(state.rng))
		{
			return {xPos(state.rng), 0, city->size.z - 1};
		}
		else
		{
			return {xPos(state.rng), city->size.y - 1, city->size.z - 1};
		}
	}
	else
	{
		std::uniform_int_distribution<int> yPos(0, city->size.y - 1);
		if (randBool(state.rng))
		{
			return {0, yPos(state.rng), city->size.z - 1};
		}
		else
		{
			return {city->size.x - 1, yPos(state.rng), city->size.z - 1};
		}
	}
}

bool VehicleMission::acquireTargetBuilding(GameState &state, Vehicle &v)
{
	std::list<StateRef<Building>> availableBuildings;
	for (auto &b : v.city->buildings)
	{
		if (!b.second->isAlive())
		{
			continue;
		}
		if (std::abs(b.second->bounds.p0.x / 2 + b.second->bounds.p1.x / 2 - v.position.x) +
		        std::abs(b.second->bounds.p0.y / 2 + b.second->bounds.p1.y / 2 - v.position.y) <=
		    TARGET_BUILDING_DISTANCE_LIMIT)
		{
			availableBuildings.emplace_back(&state, b.first);
		}
	}
	if (!availableBuildings.empty())
	{
		targetBuilding = pickRandom(state.rng, availableBuildings);
	}

	return (bool)targetBuilding;
}

void VehicleMission::updateTimer(unsigned ticks)
{
	if (ticks >= this->timeToSnooze)
		this->timeToSnooze = 0;
	else
		this->timeToSnooze -= ticks;
}

void VehicleMission::takePositionNearPortal(GameState &state, Vehicle &v)
{
	std::uniform_int_distribution<int> xPos((int)v.position.x - 2, (int)v.position.x + 2);
	std::uniform_int_distribution<int> yPos((int)v.position.y - 2, (int)v.position.y + 2);
	v.addMission(state, VehicleMission::gotoLocation(
	                        state, v, v.getPreferredPosition(xPos(state.rng), yPos(state.rng))));
}

bool VehicleMission::canRecoverVehicle(const GameState &state, const Vehicle &v,
                                       const Vehicle &target)
{
	// Ground can't recover
	if (v.type->isGround())
	{
		return false;
	}
	// Target not crashed or dead or already rescued
	if (target.isDead() || (!target.crashed && !target.sliding && !target.falling) ||
	    target.carriedByVehicle)
	{
		return false;
	}
	// Only X-Com pursues sliding or falling crafts
	if (!target.crashed && v.owner != state.getPlayer())
	{
		return false;
	}
	// Can't rescue
	if (target.owner != state.getAliens() && !v.type->canRescueCrashed)
	{
		return false;
	}
	// Only X-Com can storm UFO
	if (target.owner == state.getAliens() && v.owner != state.getPlayer())
	{
		return false;
	}
	// Find soldier
	bool needSoldiersButNotFound =
	    v.owner == state.getPlayer() ||
	    v.owner->isRelatedTo(target.owner) == Organisation::Relation::Hostile;
	for (const auto &a : v.currentAgents)
	{
		if (a->type->role == AgentType::Role::Soldier)
		{
			needSoldiersButNotFound = false;
			break;
		}
	}
	if (needSoldiersButNotFound)
	{
		return false;
	}
	// Don't attempt to rescue permanently unreachable craft
	if (VehicleTargetHelper::isReachableTarget(v, target.position) ==
	    Reachability::BlockedByScenery)
	{
		return false;
	}

	return true;
}

UString VehicleMission::getName()
{
	static const std::map<VehicleMission::MissionType, UString> TypeMap = {
	    {MissionType::GotoLocation, "GotoLocation"},
	    {MissionType::GotoBuilding, "GotoBuilding"},
	    {MissionType::GotoPortal, "GotoPortal"},
	    {MissionType::DepartToSpace, "DepartToSpace"},
	    {MissionType::FollowVehicle, "FollowVehicle"},
	    {MissionType::RecoverVehicle, "RecoverVehicle"},
	    {MissionType::AttackVehicle, "AttackVehicle"},
	    {MissionType::AttackBuilding, "AttackBuilding"},
	    {MissionType::Snooze, "Snooze for"},
	    {MissionType::SelfDestruct, "SelfDestruct in"},
	    {MissionType::ArriveFromDimensionGate, "ArriveFromDimensionGate in"},
	    {MissionType::TakeOff, "TakeOff from"},
	    {MissionType::Land, "Land into"},
	    {MissionType::Crash, "Crash landing"},
	    {MissionType::Patrol, "Patrol"},
	    {MissionType::InfiltrateSubvert, "Infiltrate/Subvert"},
	    {MissionType::RestartNextMission, "RestartNextMission"},
	    {MissionType::OfferService, "OfferServices"},
	    {MissionType::Teleport, "Teleport"},
	    {MissionType::InvestigateBuilding, "InvestigateBuilding"},
	};
	UString name = "UNKNOWN";
	const auto it = TypeMap.find(this->type);
	if (it != TypeMap.end())
		name = it->second;
	switch (this->type)
	{
		case MissionType::GotoLocation:
		case MissionType::Patrol:
		case MissionType::GotoPortal:
		case MissionType::DepartToSpace:
		case MissionType::Crash:
		case MissionType::Teleport:
			name += format(" %s", this->targetLocation);
			break;
		case MissionType::FollowVehicle:
		case MissionType::RecoverVehicle:
		case MissionType::AttackVehicle:
			name += format(" %s", this->targetVehicle.id);
			break;
		case MissionType::GotoBuilding:
		case MissionType::AttackBuilding:
		case MissionType::TakeOff:
		case MissionType::Land:
		case MissionType::InvestigateBuilding:
			name += " " + this->targetBuilding.id;
			break;
		case MissionType::ArriveFromDimensionGate:
		case MissionType::SelfDestruct:
		case MissionType::Snooze:
			name += format(" %u ticks", this->timeToSnooze);
			break;
		case MissionType::InfiltrateSubvert:
			name += " " + this->targetBuilding.id + " " + (subvert ? "[Subvert]" : "[Infiltrate]");
			break;
		case MissionType::RestartNextMission:
			break;
		case MissionType::OfferService:
			name += format(" counter: %u, target %s", missionCounter,
			               targetBuilding ? targetBuilding->name : "null");
			break;
	}
	return name;
}

GroundVehicleTileHelper::GroundVehicleTileHelper(TileMap &map, Vehicle &v)
    : GroundVehicleTileHelper(map, v.type->type)
{
}
GroundVehicleTileHelper::GroundVehicleTileHelper(TileMap &map, VehicleType::Type type)
    : map(map), type(type)
{
}

bool GroundVehicleTileHelper::canEnterTile(Tile *from, Tile *to, bool ignoreStaticUnits,
                                           bool ignoreMovingUnits, bool ignoreAllUnits) const
{
	float nothing;
	bool none1;
	bool none2;
	return canEnterTile(from, to, false, none1, nothing, none2, ignoreStaticUnits,
	                    ignoreMovingUnits, ignoreAllUnits);
}

float GroundVehicleTileHelper::pathOverheadAlloawnce() const { return 1.25f; }

bool GroundVehicleTileHelper::canEnterTile(Tile *from, Tile *to, bool, bool &, float &cost, bool &,
                                           bool, bool, bool) const
{
	if (!from)
	{
		LogError("No 'from' position supplied");
		return false;
	}
	Vec3<int> fromPos = from->position;
	if (!to)
	{
		LogError("No 'to' position supplied");
		return false;
	}
	Vec3<int> toPos = to->position;
	if (fromPos == toPos)
	{
		LogError("FromPos == ToPos %s", toPos);
		return false;
	}
	if (!map.tileIsValid(toPos))
	{
		LogError("ToPos %s is not on the map", toPos);
		return false;
	}

	auto dir = toPos - fromPos;
	// Ground can only move along one of x or y
	// And cannot go strictly vertical
	if (std::abs(dir.x) + std::abs(dir.y) > 1 || (dir.z != 0 && dir.x == 0 && dir.y == 0))
	{
		return false;
	}

	sp<Scenery> sceneryFrom = from->presentScenery;
	sp<Scenery> sceneryTo = to->presentScenery;
	if (!sceneryFrom || !sceneryTo)
	{
		return false;
	}
	int forward = convertDirection(dir);
	int backward = convertDirection(-dir);
	if (type == VehicleType::Type::Road)
	{
		// General passability check
		if (!isMoveAllowedRoad(*sceneryFrom, forward) || !isMoveAllowedRoad(*sceneryTo, backward))
		{
			return false;
		}
		// If moving from hill tile and towards hill direction can only go up a scenery
		if (sceneryFrom->type->isHill && sceneryFrom->type->hill[forward])
		{
			if (dir.z != 1)
			{
				return false;
			}
		}
		// If moving to hill and from hill direction can only go down a scenery
		if (sceneryTo->type->isHill && sceneryTo->type->hill[backward])
		{
			if (dir.z != -1)
			{
				return false;
			}
		}
		if (!sceneryFrom->type->isHill && !sceneryTo->type->isHill)
		{
			if (dir.z != 0)
			{
				return false;
			}
		}
	}
	else
	{
		// General passability check
		if (!isMoveAllowedATV(*sceneryFrom, forward) || !isMoveAllowedATV(*sceneryTo, backward))
		{
			return false;
		}
		// If moving to an "onto" tile must have clear above (unless road)
		if (sceneryTo->type->getATVMode() == SceneryTileType::WalkMode::Onto &&
		    sceneryTo->type->tile_type != SceneryTileType::TileType::Road)
		{
			auto checkedPos = (Vec3<int>)sceneryTo->currentPosition + Vec3<int>(0, 0, 1);
			if (map.tileIsValid(checkedPos))
			{
				auto checkedTile = map.getTile(checkedPos);
				if (checkedTile->presentScenery)
				{
					return false;
				}
			}
		}
		// If moving from hill to nonhill
		// - cannot move down into an "into" tile
		// - cannot move adjacently if no hill direction either way
		if (sceneryFrom->type->isHill && !sceneryTo->type->isHill)
		{
			switch (dir.z)
			{
				case 1:
					break;
				case 0:
					if (!sceneryFrom->type->hill[forward] && !sceneryFrom->type->hill[backward])
					{
						return false;
					}
					break;
				case -1:
					if (sceneryTo->type->getATVMode() == SceneryTileType::WalkMode::Into)
					{
						return false;
					}
					break;
			}
		}
		// If moving from nonhill to hill
		// - cannot move up from an "into" tile
		// - cannot move adjacently if no hill direction either way
		if (!sceneryFrom->type->isHill && sceneryTo->type->isHill)
		{
			switch (dir.z)
			{
				case 1:
					if (sceneryFrom->type->getATVMode() == SceneryTileType::WalkMode::Into)
					{
						return false;
					}
					break;
				case 0:
					if (!sceneryTo->type->hill[forward] && !sceneryTo->type->hill[backward])
					{
						return false;
					}
					break;
				case -1:
					break;
			}
		}
		// If moving from hill tile and towards hill direction must go "up"
		if (sceneryFrom->type->isHill && sceneryFrom->type->hill[forward])
		{
			switch (dir.z)
			{
				// Going up on Z means we must go to "into" tile or hill
				case 1:
					if (sceneryTo->type->getATVMode() != SceneryTileType::WalkMode::Into &&
					    !sceneryTo->type->isHill)
					{
						return false;
					}
					break;
				// Going same Z means we must go to "onto" tile
				case 0:
					if (sceneryTo->type->getATVMode() != SceneryTileType::WalkMode::Onto)
					{
						return false;
					}
					break;
				// Going down on Z is impossible
				case -1:
					return false;
			}
		}
		// If moving to hill and from hill direction
		if (sceneryTo->type->isHill && sceneryTo->type->hill[backward])
		{
			switch (dir.z)
			{
				// Going up on Z is impossible
				case 1:
					return false;
				// Going same Z means we must go from "onto" tile
				case 0:
					if (sceneryFrom->type->getATVMode() != SceneryTileType::WalkMode::Onto)
					{
						return false;
					}
					break;
				// Going down on Z means we must go from "into" tile or another hill
				case -1:
					if (sceneryFrom->type->getATVMode() != SceneryTileType::WalkMode::Into &&
					    !sceneryFrom->type->isHill)
					{
						return false;
					}
					break;
			}
		}
		// If moving from non-hill into non-hill
		if (!sceneryFrom->type->isHill && !sceneryTo->type->isHill)
		{
			switch (dir.z)
			{
				// Going up on Z means going from onto to into
				case 1:
					if (sceneryFrom->type->getATVMode() != SceneryTileType::WalkMode::Onto ||
					    sceneryTo->type->getATVMode() != SceneryTileType::WalkMode::Into)
					{
						return false;
					}
					break;
				// Going same Z means we must go between same walk modes
				case 0:
					if (sceneryFrom->type->getATVMode() != sceneryTo->type->getATVMode())
					{
						return false;
					}
					break;
				// Going down on Z means we must go from into to onto
				case -1:
					if (sceneryFrom->type->getATVMode() != SceneryTileType::WalkMode::Into ||
					    sceneryTo->type->getATVMode() != SceneryTileType::WalkMode::Onto)
					{
						return false;
					}
					break;
			}
		}
		// If moving from hill into hill
		if (sceneryFrom->type->isHill && sceneryTo->type->isHill)
		{
			switch (dir.z)
			{
				// Going up on Z means using hillFrom's slope
				case 1:
					if (!sceneryFrom->type->hill[forward])
					{
						return false;
					}
					break;
				// Going same Z is fine
				case 0:
					break;
				// Going down on Z means using hillTo's slope
				case -1:
					if (!sceneryTo->type->hill[backward])
					{
						return false;
					}
					break;
			}
		}
	}

	cost = 1.0f;
	return true;
}

float GroundVehicleTileHelper::getDistance(Vec3<float> from, Vec3<float> to) const
{
	return getDistanceStatic(from, to);
}

float GroundVehicleTileHelper::getDistance(Vec3<float> from, Vec3<float> toStart,
                                           Vec3<float> toEnd) const
{
	return getDistanceStatic(from, toStart, toEnd);
}

float GroundVehicleTileHelper::getDistanceStatic(Vec3<float> from, Vec3<float> to)
{
	return std::abs(from.x - to.x) + std::abs(from.y - to.y);
}

float GroundVehicleTileHelper::getDistanceStatic(Vec3<float> from, Vec3<float> toStart,
                                                 Vec3<float> toEnd)
{
	auto diffStart = toStart - from;
	auto diffEnd = toEnd - from - Vec3<float>{1.0f, 1.0f, 1.0f};
	auto xDiff = from.x >= toStart.x && from.x < toEnd.x
	                 ? 0.0f
	                 : std::min(std::abs(diffStart.x), std::abs(diffEnd.x));
	auto yDiff = from.y >= toStart.y && from.y < toEnd.y
	                 ? 0.0f
	                 : std::min(std::abs(diffStart.y), std::abs(diffEnd.y));
	return xDiff + yDiff;
}

int GroundVehicleTileHelper::convertDirection(Vec3<int> dir) const
{
	// No sanity checks, assuming only one coord is non-zero
	if (dir.y == -1)
	{
		return 0;
	}
	if (dir.x == 1)
	{
		return 1;
	}
	if (dir.y == 1)
	{
		return 2;
	}
	if (dir.x == -1)
	{
		return 3;
	}
	LogError("Impossible to reach here? convertDirection for 0,0,0?");
	return -1;
}

bool GroundVehicleTileHelper::isMoveAllowedRoad(Scenery &scenery, int dir) const
{
	switch (scenery.type->tile_type)
	{
		// Can traverse road only according to flags
		case SceneryTileType::TileType::Road:
			return scenery.type->connection[dir];
		case SceneryTileType::TileType::PeopleTubeJunction:
		case SceneryTileType::TileType::PeopleTube:
		case SceneryTileType::TileType::General:
		case SceneryTileType::TileType::CityWall:
			return false;
	}
	LogError("Unhandled situiation in isMoveAllowedRoad, can't reach here?");
	return false;
}

bool GroundVehicleTileHelper::isMoveAllowedATV(Scenery &scenery, int dir) const
{
	// Adhere to terminals otherwise we exit through walls
	if (scenery.type->road_type == SceneryTileType::RoadType::Terminal)
	{
		return scenery.type->connection[dir];
	}
	switch (scenery.type->getATVMode())
	{
		// Can traverse roads and walk in tiles
		case SceneryTileType::WalkMode::Into:
		case SceneryTileType::WalkMode::Onto:
			return true;
		case SceneryTileType::WalkMode::None:
			return false;
	}
	LogError("Unhandled situiation in isMoveAllowedATV, can't reach here?");
	return false;
}

} // namespace OpenApoc

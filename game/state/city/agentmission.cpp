#include "game/state/city/agentmission.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "framework/sound.h"
#include "game/state/city/building.h"
#include "game/state/city/city.h"
#include "game/state/city/citycommonsamplelist.h"
#include "game/state/city/doodad.h"
#include "game/state/city/scenery.h"
#include "game/state/agent.h"
#include "game/state/gamestate.h"
#include "game/state/organisation.h"
#include "game/state/rules/scenery_tile_type.h"
#include "game/state/tileview/tile.h"
#include "game/state/tileview/tileobject_doodad.h"
#include "game/state/tileview/tileobject_scenery.h"
#include "game/state/tileview/tileobject_shadow.h"
#include "library/strings_format.h"
#include <glm/glm.hpp>

namespace OpenApoc
{

AgentTileHelper::AgentTileHelper(TileMap &map, Agent &a) : map(map), a(a) {}

bool AgentTileHelper::canEnterTile(Tile *from, Tile *to, bool ignoreStaticUnits,
                                           bool ignoreAllUnits) const
{
	float nothing;
	bool none1;
	bool none2;
	return canEnterTile(from, to, false, none1, nothing, none2, ignoreStaticUnits, ignoreAllUnits);
}

float AgentTileHelper::pathOverheadAlloawnce() const { return 1.25f; }

// Support 'from' being nullptr for if a agent is being spawned in the map
bool AgentTileHelper::canEnterTile(Tile *from, Tile *to, bool, bool &, float &cost, bool &,
                                           bool, bool) const
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

	//FIXME: Implement agent pathfinding

	cost = glm::length(Vec3<float>{fromPos} - Vec3<float>{toPos});
	return true;
}


float AgentTileHelper::getDistance(Vec3<float> from, Vec3<float> to) const
{
	return glm::length(to - from);
}

float AgentTileHelper::getDistance(Vec3<float> from, Vec3<float> toStart,
                                           Vec3<float> toEnd) const
{
	auto diffStart = toStart - from;
	auto diffEnd = toEnd - from - Vec3<float>{1.0f, 1.0f, 1.0f};
	auto xDiff = from.x >= toStart.x && from.x < toEnd.x ? 0.0f : std::min(std::abs(diffStart.x),
	                                                                       std::abs(diffEnd.x));
	auto yDiff = from.y >= toStart.y && from.y < toEnd.y ? 0.0f : std::min(std::abs(diffStart.y),
	                                                                       std::abs(diffEnd.y));
	auto zDiff = from.z >= toStart.z && from.z < toEnd.z ? 0.0f : std::min(std::abs(diffStart.z),
	                                                                       std::abs(diffEnd.z));
	return sqrtf(xDiff * xDiff + yDiff * yDiff + zDiff * zDiff);
}

AgentMission *AgentMission::gotoBuilding(GameState &, Agent &, StateRef<Building> target,
                                             bool allowTeleporter)
{
	auto *mission = new AgentMission();
	mission->type = MissionType::GotoBuilding;
	mission->targetBuilding = target;
	mission->allowTeleporter = allowTeleporter;
	return mission;
}

AgentMission *AgentMission::snooze(GameState &, Agent &, unsigned int snoozeTicks)
{
	auto *mission = new AgentMission();
	mission->type = MissionType::Snooze;
	mission->timeToSnooze = snoozeTicks;
	return mission;
}

AgentMission *AgentMission::restartNextMission(GameState &, Agent &)
{
	auto *mission = new AgentMission();
	mission->type = MissionType::RestartNextMission;
	return mission;
}

AgentMission *AgentMission::awaitPickup(GameState &, Agent &)
{
	auto *mission = new AgentMission();
	mission->type = MissionType::AwaitPickup;
	return mission;
}

AgentMission *AgentMission::teleport(GameState &state, Agent &a, StateRef<Building> b)
{
	auto *mission = new AgentMission();
	mission->type = MissionType::Teleport;
	mission->targetBuilding = b;
	return mission;
}

bool AgentMission::teleportCheck(GameState &state, Agent &a)
{
	if (allowTeleporter && a.canTeleport())
	{
		auto *teleportMission = AgentMission::teleport(state, a, targetBuilding);
		a.missions.emplace_front(teleportMission);
		teleportMission->start(state, v);
		return true;
	}
	return false;
}

bool AgentMission::getNextDestination(GameState &state, Agent &a, Vec3<float> &destPos)
{
	switch (this->type)
	{
		case MissionType::GotoBuilding:
		{
			return advanceAlongPath(state, a, destPos);
		}
		case MissionType::Snooze:
		case MissionType::RestartNextMission:
		case MissionType::AwaitPickup:
		{
			return false;
		}
		default:
			LogWarning("TODO: Implement getNextDestination");
			return false;
	}
	return false;
}

void AgentMission::update(GameState &state, Agent &a, unsigned int ticks, bool finished)
{
	finished = finished || isFinishedInternal(state, a);
	switch (this->type)
	{
		case MissionType::GotoBuilding:
		{
			auto vTile = a.tileObject;
			if (vTile && !finished && this->currentPlannedPath.empty())
			{
				if (reRouteAttempts > 0)
				{
					reRouteAttempts--;
					setPathTo(state, a, targetLocation);
				}
				else
				{
					// Finall attempt, give up if fails
					setPathTo(state, a, targetLocation, 500, true, true);
				}
			}
			return;
		}
		case MissionType::AwaitPickup:
			LogError("Check if pickup coming or order pickup");
			return;
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

bool AgentMission::isFinished(GameState &state, Agent &a, bool callUpdateIfFinished)
{
	if (isFinishedInternal(state, a))
	{
		if (callUpdateIfFinished)
		{
			update(state, a, 0, true);
		}
		return true;
	}
	return false;
}

bool AgentMission::isFinishedInternal(GameState &, Agent &a)
{
	switch (this->type)
	{
		case MissionType::GotoBuilding:
			return this->targetBuilding == a.currentBuilding;
		case MissionType::Snooze:
			return this->timeToSnooze == 0;
		case MissionType::AwaitPickup:
			LogError("Implement awaitpickup isfinished");
			return true;
		case MissionType::RestartNextMission:
		case MissionType::Teleport:
			return true;
		default:
			LogWarning("TODO: Implement isFinishedInternal");
			return false;
	}
}

void AgentMission::start(GameState &state, Agent &a)
{
	switch (this->type)
	{
		case MissionType::Teleport:
		{
			if (!a.canTeleport())
			{
				return;
			}
			auto &map = *state.current_city->map;
			auto canEnter = AgentTileHelper(map, v);
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
				targetTile = a.position;
			}
			if (a.currentBuilding)
			{
				a.leaveBuilding(state, targetTile);
			}
			else
			{
				a.facing = a.goalFacing;
				a.ticksToTurn = 0;
				a.angularVelocity = 0.0f;
				a.setPosition((Vec3<float>)targetTile + Vec3<float>{0.5f, 0.5f, 0.5f});
				a.goalPosition = a.position;
				a.velocity = {0.0f, 0.0f, 0.0f};
				a.updateSprite(state);
			}
			if (state.city_common_sample_list->teleport)
			{
				fw().soundBackend->playSample(state.city_common_sample_list->teleport,
				                              a.getPosition());
			}
			return;
		}
		case MissionType::RestartNextMission:
		case MissionType::Snooze:
			// No setup
			return;
		default:
			LogError("TODO: Implement start");
			return;
	}
}

void AgentMission::setPathTo(GameState &state, Agent &a, Vec3<int> target, int maxIterations,
                               bool checkValidity, bool giveUpIfInvalid)
{
	auto agentTile = a.tileObject;
	if (agentTile)
	{
		auto &map = agentTile->map;
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
					targetLocation = agentTile->getPosition();
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
			// Check if target tile has no agent termporarily blocking it
			// If it does, find a random location around it that is not blocked
			bool containsAgent = false;
			for (auto &obj : to->ownedObjects)
			{
				if (obj->getType() == TileObject::Type::Agent)
				{
					containsAgent = true;
					break;
				}
			}
			if (containsAgent)
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
				int midZ = (int)a.altitude;
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

		auto path = map.findShortestPath(agentTile->getOwningTile()->position, target,
		                                 maxIterations, AgentTileHelper{map, v});

		// Always start with the current position
		this->currentPlannedPath.push_back(agentTile->getOwningTile()->position);
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

bool AgentMission::advanceAlongPath(GameState &state, Agent &a, Vec3<float> &destPos)
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
		destPos = Vec3<float>{pos.x, pos.y, pos.z} + offsetLand;
		return true;
	}
	if (type == MissionType::TakeOff)
	{
		destPos = Vec3<float>{pos.x, pos.y, pos.z} + offset;
		return true;
	}

	// See if we can actually go there
	auto tFrom = a.tileObject->getOwningTile();
	auto tTo = tFrom->map.getTile(pos);
	if (tFrom->position != pos &&
	    (std::abs(tFrom->position.x - pos.x) > 1 || std::abs(tFrom->position.y - pos.y) > 1 ||
	     std::abs(tFrom->position.z - pos.z) > 1 ||
	     !AgentTileHelper{tFrom->map, v}.canEnterTile(tFrom, tTo)))
	{
		// Next tile became impassable, pick a new path
		currentPlannedPath.clear();
		a.missions.emplace_front(restartNextMission(state, v));
		a.missions.front()->start(state, v);
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
	         AgentTileHelper{tFrom->map, v}.canEnterTile(tFrom, tFrom->map.getTile(*it)))))
	{
		currentPlannedPath.pop_front();
		pos = currentPlannedPath.front();
		tTo = tFrom->map.getTile(pos);
		it = ++currentPlannedPath.begin();
	}

	destPos = Vec3<float>{pos.x, pos.y, pos.z} + offset;
	return true;
}

bool AgentMission::isTakingOff(Agent &a)
{
	return type == MissionType::TakeOff && currentPlannedPath.size() > 2 &&
	       (a.position.z - ((int)a.position.z)) <= 0.5f;
}

UString AgentMission::getName()
{
	static const std::map<AgentMission::MissionType, UString> TypeMap = {
	    {MissionType::GotoLocation, "GotoLocation"},
	    {MissionType::GotoBuilding, "GotoBuilding"},
	    {MissionType::GotoPortal, "GotoBuilding"},
	    {MissionType::FollowAgent, "FollowAgent"},
	    {MissionType::AttackAgent, "AttackAgent"},
	    {MissionType::AttackBuilding, "AttackBuilding"},
	    {MissionType::Snooze, "Snooze"},
	    {MissionType::TakeOff, "TakeOff"},
	    {MissionType::Land, "Land"},
	    {MissionType::Crash, "Crash"},
	    {MissionType::Patrol, "Patrol"},
	    {MissionType::InfiltrateSubvert, "Infiltrate/Subvert"},
	    {MissionType::RestartNextMission, "RestartNextMission"},
	    {MissionType::Teleport, "Teleport"},
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
		case MissionType::FollowAgent:
			name += " " + this->targetAgent.id;
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
		case MissionType::AttackAgent:
			name += format(" target \"%s\"", this->targetAgent.id);
			break;
		case MissionType::Patrol:
			name += format(" %s", this->targetLocation.x);
			break;
		case MissionType::GotoPortal:
			name += format(" %s", this->targetLocation.x);
			break;
		case MissionType::Teleport:
			name += format(" random around %s", this->targetLocation.x);
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

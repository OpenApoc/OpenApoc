#include "game/state/city/agentmission.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "framework/sound.h"
#include "game/state/city/building.h"
#include "game/state/city/city.h"
#include "game/state/city/scenery.h"
#include "game/state/gameevent.h"
#include "game/state/gamestate.h"
#include "game/state/rules/battle/battlecommonsamplelist.h"
#include "game/state/rules/city/scenerytiletype.h"
#include "game/state/shared/agent.h"
#include "game/state/shared/doodad.h"
#include "game/state/shared/organisation.h"
#include "game/state/tilemap/tilemap.h"
#include "game/state/tilemap/tileobject_doodad.h"
#include "game/state/tilemap/tileobject_scenery.h"
#include "game/state/tilemap/tileobject_shadow.h"
#include "library/strings_format.h"
#include <glm/glm.hpp>

namespace OpenApoc
{

AgentTileHelper::AgentTileHelper(TileMap &map) : map(map) {}

bool AgentTileHelper::canEnterTile(Tile *from, Tile *to, bool ignoreStaticUnits,
                                   bool ignoreMovingUnits, bool ignoreAllUnits) const
{
	float nothing;
	bool none1;
	bool none2;
	return canEnterTile(from, to, false, none1, nothing, none2, ignoreStaticUnits,
	                    ignoreMovingUnits, ignoreAllUnits);
}

float AgentTileHelper::pathOverheadAlloawnce() const { return 1.25f; }

bool AgentTileHelper::canEnterTile(Tile *from, Tile *to, bool, bool &, float &cost, bool &, bool,
                                   bool, bool) const
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

	// Agents can only move along one axis
	auto dir = toPos - fromPos;
	if (std::abs(dir.x) + std::abs(dir.y) + std::abs(dir.z) > 1)
	{
		return false;
	}

	// Agents can only move to and from scenery
	sp<Scenery> sceneryFrom = from->presentScenery;
	sp<Scenery> sceneryTo = to->presentScenery;
	if (!sceneryFrom || !sceneryTo)
	{
		return false;
	}

	// General passability check
	int forward = convertDirection(dir);
	if (!isMoveAllowed(*sceneryFrom, forward) || !isMoveAllowed(*sceneryTo, convertDirection(-dir)))
	{
		return false;
	}

	// If going sideways we can only go:
	// - into junction or tube
	// - on subway level
	if (forward < 4)
	{
		if (sceneryTo->type->tile_type != SceneryTileType::TileType::PeopleTubeJunction &&
		    sceneryTo->type->tile_type != SceneryTileType::TileType::PeopleTube &&
		    toPos.z != SUBWAY_HEIGHT_AGENT)
		{
			return false;
		}
	}

	// If going up/down into general we must have a tube junction or crew quarters above
	if (forward >= 4 && sceneryTo->type->tile_type == SceneryTileType::TileType::General)
	{
		auto building = sceneryTo->building;
		bool foundJunctionOrCrew = false;
		Vec3<int> checkedPos = sceneryTo->currentPosition;
		do
		{
			checkedPos.z++;
			if (checkedPos.z >= map.size.z)
			{
				break;
			}
			if (building->crewQuarters == checkedPos)
			{
				foundJunctionOrCrew = true;
				continue;
			}
			auto checkedTile = map.getTile(checkedPos);
			sp<Scenery> checkedScenery = checkedTile->presentScenery;
			if (checkedScenery &&
			    checkedScenery->type->tile_type == SceneryTileType::TileType::PeopleTubeJunction)
			{
				foundJunctionOrCrew = true;
				continue;
			}
		} while (!foundJunctionOrCrew);
		if (!foundJunctionOrCrew)
		{
			return false;
		}
	}

	cost = 1.0f;
	return true;
}

float AgentTileHelper::getDistance(Vec3<float> from, Vec3<float> to) const
{
	return std::abs(from.x - to.x) + std::abs(from.y - to.y) + std::abs(from.z - to.z);
}

float AgentTileHelper::getDistance(Vec3<float> from, Vec3<float> toStart, Vec3<float> toEnd) const
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
	return xDiff + yDiff + zDiff;
}

int AgentTileHelper::convertDirection(Vec3<int> dir) const
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
	if (dir.z == 1)
	{
		return 4;
	}
	if (dir.z == -1)
	{
		return 5;
	}
	LogError("Impossible to reach here? convertDirection for 0,0,0?");
	return -1;
}

bool AgentTileHelper::isMoveAllowed(Scenery &scenery, int dir) const
{
	switch (scenery.type->tile_type)
	{
		// Can traverse people's tube only according to flags
		case SceneryTileType::TileType::PeopleTube:
			return scenery.type->tube[dir];
		// Can traverse junction according to flags or up/down
		case SceneryTileType::TileType::PeopleTubeJunction:
			return scenery.type->tube[dir] || dir == 4 || dir == 5;
		// Can traverse roads and general only if they are part of a building
		case SceneryTileType::TileType::General:
		case SceneryTileType::TileType::Road:
			return (bool)scenery.building;
		// Cannot traverse walls ever
		case SceneryTileType::TileType::CityWall:
			return false;
	}
	LogError("Unhandled situiation in isMoveAllowed, can't reach here?");
	return false;
}

AgentMission *AgentMission::gotoBuilding(GameState &, Agent &a, StateRef<Building> target,
                                         bool allowTeleporter, bool allowTaxi)
{
	auto *mission = new AgentMission();
	mission->type = MissionType::GotoBuilding;
	mission->targetBuilding = target ? target : a.homeBuilding;
	mission->allowTeleporter = allowTeleporter && a.type->role == AgentType::Role::Soldier;
	mission->allowTaxi = allowTaxi || a.type->role != AgentType::Role::Soldier;
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

AgentMission *AgentMission::awaitPickup(GameState &, Agent &, StateRef<Building> target)
{
	auto *mission = new AgentMission();
	mission->type = MissionType::AwaitPickup;
	mission->timeToSnooze = PICKUP_TIMEOUT;
	mission->targetBuilding = target;
	return mission;
}

AgentMission *AgentMission::teleport(GameState &state [[maybe_unused]], Agent &a [[maybe_unused]],
                                     StateRef<Building> b)
{
	auto *mission = new AgentMission();
	mission->type = MissionType::Teleport;
	mission->targetBuilding = b;
	return mission;
}

AgentMission *AgentMission::investigateBuilding(GameState &, Agent &a, StateRef<Building> target,
                                                bool allowTeleporter, bool allowTaxi)
{
	auto *mission = new AgentMission();
	mission->type = MissionType::InvestigateBuilding;
	mission->targetBuilding = target;
	mission->allowTeleporter = allowTeleporter && a.type->role == AgentType::Role::Soldier;
	mission->allowTaxi = allowTaxi || a.type->role != AgentType::Role::Soldier;
	return mission;
}

bool AgentMission::teleportCheck(GameState &state, Agent &a)
{
	if (allowTeleporter && a.canTeleport())
	{
		auto *teleportMission = AgentMission::teleport(state, a, targetBuilding);
		a.missions.emplace_front(teleportMission);
		teleportMission->start(state, a);
		return true;
	}
	return false;
}

bool AgentMission::getNextDestination(GameState &state, Agent &a, Vec3<float> &destPos)
{
	if (cancelled)
	{
		return false;
	}
	switch (this->type)
	{
		case MissionType::GotoBuilding:
		case MissionType::InvestigateBuilding:
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
			return;
		case MissionType::InvestigateBuilding:
		{
			if (finished && this->targetBuilding->detected && a.owner == state.getPlayer())
			{
				this->targetBuilding->decreasePendingInvestigatorCount(state);
			}
			return;
		}
		case MissionType::RestartNextMission:
			return;
		case MissionType::AwaitPickup:
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
	if (cancelled)
	{
		return true;
	}
	switch (this->type)
	{
		case MissionType::GotoBuilding:
		case MissionType::AwaitPickup:
		case MissionType::InvestigateBuilding:
			return this->targetBuilding == a.currentBuilding;
		case MissionType::Snooze:
			return this->timeToSnooze == 0;
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
		case MissionType::GotoBuilding:
		case MissionType::InvestigateBuilding:
		{
			// Already there?
			if ((Vec3<int>)a.position == targetBuilding->crewQuarters)
			{
				a.enterBuilding(state, targetBuilding);
				if (a.recentlyHired)
				{
					fw().pushEvent(new GameAgentEvent(GameEventType::AgentArrived,
					                                  {&state, a.shared_from_this()}));
					a.recentlyHired = false;
				}
				if (a.recentryTransferred)
				{
					fw().pushEvent(new GameAgentEvent(GameEventType::AgentArrived,
					                                  {&state, a.shared_from_this()}, true));
					a.recentryTransferred = false;
				}
				return;
			}
			// Can teleport there?
			if (teleportCheck(state, a))
			{
				return;
			}
			// Can order a taxi?
			if (a.currentBuilding && allowTaxi)
			{
				allowTaxi = false;
				a.addMission(state, AgentMission::awaitPickup(state, a, targetBuilding));
				return;
			}
			// Need leave building?
			if (a.currentBuilding)
			{
				a.leaveBuilding(state, a.position);
			}
			// Need to path there?
			if (currentPlannedPath.empty())
			{
				this->setPathTo(state, a, this->targetBuilding);
				// Could not path
				if (cancelled)
				{
					if (a.currentBuilding)
					{
						fw().pushEvent(new GameAgentEvent(GameEventType::AgentUnableToReach,
						                                  {&state, a.shared_from_this()}, true));
					}
					else
					{
						// FIXME: Implement agent pathing to closest building when in the field and
						// unable to path
						LogWarning("Implement agent pathing to closest building when in the field "
						           "and unable to path to "
						           "building");
						// For now just get into closest building
						fw().pushEvent(new GameAgentEvent(GameEventType::AgentUnableToReach,
						                                  {&state, a.shared_from_this()}, true));
						float closestDistance = FLT_MAX;
						StateRef<Building> closestBuilding;
						for (auto &b : a.city->buildings)
						{
							auto distance =
							    glm::length(a.position - (Vec3<float>)b.second->crewQuarters);
							if (distance < closestDistance)
							{
								distance = closestDistance;
								closestBuilding = {&state, b.first};
							}
						}
						a.enterBuilding(state, closestBuilding);
					}
				}
			}
		}
		case MissionType::Teleport:
		{
			if (!a.canTeleport())
			{
				return;
			}
			a.enterBuilding(state, targetBuilding);
			if (state.battle_common_sample_list->teleport)
			{
				fw().soundBackend->playSample(state.battle_common_sample_list->teleport,
				                              a.position);
			}
			return;
		}
		case MissionType::AwaitPickup:
		case MissionType::RestartNextMission:
		case MissionType::Snooze:
			// No setup
			return;
		default:
			LogError("TODO: Implement start");
			return;
	}
}

void AgentMission::setPathTo(GameState &state [[maybe_unused]], Agent &a, StateRef<Building> b)
{
	this->currentPlannedPath.clear();
	auto &map = *a.city->map;

	std::list<Vec3<int>> path;
	auto key = Vec3<int>{(Vec3<int>)a.position * map.size + b->crewQuarters};
	if (map.agentPathCache.find(key) != map.agentPathCache.end())
	{
		LogWarning("Found cached path from %s to %s, using it", a.position, b->crewQuarters);
		path = map.agentPathCache[key];
	}
	else
	{
		path = map.findShortestPath(a.position, b->crewQuarters, 2000, AgentTileHelper{map});
		map.agentPathCache[key] = path;
	}
	if (path.empty() || path.back() != b->crewQuarters)
	{
		cancelled = true;
		return;
	}

	// Always start with the current position
	this->currentPlannedPath.push_back(a.position);
	for (auto &p : path)
	{
		this->currentPlannedPath.push_back(p);
	}
}

bool AgentMission::advanceAlongPath(GameState &state, Agent &a, Vec3<float> &destPos)
{
	// Add {0.5,0.5,0.5} to make it route to the center of the tile
	static const Vec3<float> offset{0.5f, 0.5f, 0.5f};

	if (currentPlannedPath.empty())
	{
		a.addMission(state, restartNextMission(state, a));
		return false;
	}
	currentPlannedPath.pop_front();
	if (currentPlannedPath.empty())
	{
		a.addMission(state, restartNextMission(state, a));
		return false;
	}
	auto pos = currentPlannedPath.front();

	// See if we can actually go there
	auto tFrom = a.city->map->getTile(a.position);
	auto tTo = tFrom->map.getTile(pos);
	if (tFrom->position != pos &&
	    (std::abs(tFrom->position.x - pos.x) > 1 || std::abs(tFrom->position.y - pos.y) > 1 ||
	     std::abs(tFrom->position.z - pos.z) > 1 ||
	     !AgentTileHelper{tFrom->map}.canEnterTile(tFrom, tTo)))
	{
		// Next tile became impassable, pick a new path
		currentPlannedPath.clear();
		a.addMission(state, restartNextMission(state, a));
		return false;
	}

	// See if we can make a shortcut
	// When ordering move to vehicle already on the move, we can have a situation
	// where going directly to 2nd step in the path is faster than going to the first
	// In this case, we should skip unnecessary steps
	auto it = ++currentPlannedPath.begin();
	// Start with position after next
	// If next position has a node and we can go directly to that node
	// Then update current position and iterator
	while (it != currentPlannedPath.end() &&
	       (tFrom->position == *it ||
	        (std::abs(tFrom->position.x - it->x) <= 1 && std::abs(tFrom->position.y - it->y) <= 1 &&
	         std::abs(tFrom->position.z - it->z) <= 1 &&
	         AgentTileHelper{tFrom->map}.canEnterTile(tFrom, tFrom->map.getTile(*it)))))
	{
		currentPlannedPath.pop_front();
		pos = currentPlannedPath.front();
		tTo = tFrom->map.getTile(pos);
		it = ++currentPlannedPath.begin();
	}

	destPos = Vec3<float>{pos.x, pos.y, pos.z} + offset;
	return true;
}

UString AgentMission::getName()
{
	static const std::map<AgentMission::MissionType, UString> TypeMap = {
	    {MissionType::GotoBuilding, "GotoBuilding"},
	    {MissionType::Snooze, "Snooze"},
	    {MissionType::AwaitPickup, "AwaitPickup"},
	    {MissionType::RestartNextMission, "RestartNextMission"},
	    {MissionType::Teleport, "Teleport"},
	    {MissionType::InvestigateBuilding, "InvestigateBuilding"},
	};
	UString name = "UNKNOWN";
	const auto it = TypeMap.find(this->type);
	if (it != TypeMap.end())
		name = it->second;
	switch (this->type)
	{
		case MissionType::GotoBuilding:
		case MissionType::InvestigateBuilding:
			name += " " + this->targetBuilding.id;
			break;
		case MissionType::Snooze:
			name += format(" for %u ticks", this->timeToSnooze);
			break;
		case MissionType::Teleport:
			name += " " + this->targetBuilding.id;
			break;
		case MissionType::AwaitPickup:
		case MissionType::RestartNextMission:
			break;
	}
	return name;
}

} // namespace OpenApoc

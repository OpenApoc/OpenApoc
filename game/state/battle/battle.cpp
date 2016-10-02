#include "game/state/battle/battle.h"
#include "framework/framework.h"
#include "framework/trace.h"
#include "game/state/battle/battledoor.h"
#include "game/state/battle/battleitem.h"
#include "game/state/battle/battlemap.h"
#include "game/state/battle/battlemappart.h"
#include "game/state/battle/battlemappart_type.h"
#include "game/state/battle/battleunit.h"
#include "game/state/city/city.h"
#include "game/state/city/doodad.h"
#include "game/state/city/projectile.h"
#include "game/state/city/vehicle.h"
#include "game/state/gamestate.h"
#include "game/state/tileview/collision.h"
#include "game/state/tileview/tile.h"
#include "game/state/tileview/tileobject_battleitem.h"
#include "game/state/tileview/tileobject_battlemappart.h"
#include "game/state/tileview/tileobject_battleunit.h"
#include "game/state/tileview/tileobject_doodad.h"
#include "game/state/tileview/tileobject_projectile.h"
#include "game/state/tileview/tileobject_shadow.h"
#include "library/xorshift.h"
#include <algorithm>
#include <functional>
#include <future>
#include <limits>
#include <unordered_map>

#ifdef _MSC_VER
#pragma warning (push, 1)
#pragma warning(disable : 4503)
#endif

namespace OpenApoc
{
// An ordered list of the types drawn in each layer
// Within the same layer these are ordered by a calculated z based on the 'center' position
static std::vector<std::set<TileObject::Type>> layerMap = {
    // Draw ground first, then put stuff on top of that
    // In order for selectionBracket to be drawn properly, first layer must contain all mapparts and
    // the unit type
    {TileObject::Type::Ground, TileObject::Type::LeftWall, TileObject::Type::RightWall,
     TileObject::Type::Feature, TileObject::Type::Doodad, TileObject::Type::Projectile,
     TileObject::Type::Unit, TileObject::Type::Shadow, TileObject::Type::Item},
};

Battle::~Battle()
{
	TRACE_FN;
	// Note due to backrefs to Tile*s etc. we need to destroy all tile objects
	// before the TileMap
	for (auto &p : this->projectiles)
	{
		if (p->tileObject)
			p->tileObject->removeFromMap();
		p->tileObject = nullptr;
	}
	this->projectiles.clear();
	for (auto &s : this->map_parts)
	{
		if (s->tileObject)
			s->tileObject->removeFromMap();
		s->tileObject = nullptr;
		s->battle = nullptr;
	}
	this->map_parts.clear();
	for (auto &u : this->units)
	{
		u.second->agent->unit = nullptr;
	}
	for (auto &s : this->items)
	{
		if (s->tileObject)
			s->tileObject->removeFromMap();
		if (s->shadowObject)
			s->shadowObject->removeFromMap();
		s->tileObject = nullptr;
	}
	this->items.clear();
	for (auto &d : this->doors)
	{
		d->battle = nullptr;
	}
	this->doors.clear();
}

void Battle::initBattle(GameState &state)
{
	common_image_list = state.battle_common_image_list;
	common_sample_list = state.battle_common_sample_list;
	loadResources(state);
	auto stt = shared_from_this();
	for (auto &s : this->map_parts)
	{
		s->battle = stt;
		if (s->doorID != -1)
		{
			doors[s->doorID]->mapParts.push_back(s);
			doors[s->doorID]->position = s->getPosition();
		}
	}
	for (auto &d : this->doors)
	{
		d->battle = stt;
	}
	for (auto &o : this->items)
	{
		o->battle = stt;
		o->item->ownerItem = o->shared_from_this();
	}
	for (auto &o : this->units)
	{
		o.second->battle = stt;
		if (o.second->focusUnit)
		{
			o.second->focusUnit->focusedByUnits.push_back(o.second);
		}
	}
	if (forces.size() == 0)
	{
		// Init forces and fill squads with nullptrs so that we have where to place units
		for (auto &o : this->participants)
		{
			forces[o];
			for (int i = 0; i < 6; i++)
			{
				forces[o].squads[i].units = std::vector<sp<BattleUnit>>(6);
			}
		}
		// Place units into squads directly to their positions
		for (auto &u : this->units)
		{
			forces[u.second->owner].squads[u.second->squadNumber].units[u.second->squadPosition] = u.second;
		}
		// Trim nullptrs from squad units
		for (auto &o : this->participants)
		{
			for (int i = 0; i < 6; i++)
			{
				for (int j = 5; j >= 0; j--)
				{
					if (forces[o].squads[i].units[j])
					{
						break;
					}
					forces[o].squads[i].units.erase(forces[o].squads[i].units.begin() + j);
				}
			}
		}
	}
	initMap();
}

void Battle::initMap()
{
	if (this->map)
	{
		LogError("Called on battle with existing map");
		return;
	}
	this->map.reset(new TileMap(this->size, VELOCITY_SCALE_BATTLE,
	                            {VOXEL_X_BATTLE, VOXEL_Y_BATTLE, VOXEL_Z_BATTLE}, layerMap));
	for (auto &s : this->map_parts)
	{
		if (s->destroyed)
		{
			continue;
		}
		this->map->addObjectToMap(s);
	}
	for (auto &s : this->map_parts)
	{
		s->findSupport();
	}
	for (auto &s : this->map_parts)
	{
		s->tryCollapse();
	}
	for (auto &o : this->items)
	{
		this->map->addObjectToMap(o);
		if (o->supported)
		{
			o->supported = false;
			o->findSupport(false, true);
		}
	}
	for (auto &u : this->units)
	{
		if (u.second->destroyed || u.second->retreated)
		{
			continue;
		}
		this->map->addObjectToMap(u.second);
	}
	for (auto &p : this->projectiles)
	{
		this->map->addObjectToMap(p);
	}
	for (auto &d : this->doodads)
	{
		this->map->addObjectToMap(d);
	}
}

sp<Doodad> Battle::placeDoodad(StateRef<DoodadType> type, Vec3<float> position)
{
	auto doodad = mksp<Doodad>(position, type);
	map->addObjectToMap(doodad);
	this->doodads.push_back(doodad);
	return doodad;
}

void Battle::addUnit(sp<BattleUnit> unit)
{
	unit->id = UString::format("%s%d", BattleUnit::getPrefix(), (int)units.size());
	units[unit->id] = unit;
}

void Battle::update(GameState &state, unsigned int ticks)
{
	TRACE_FN_ARGS1("ticks", Strings::fromInteger(static_cast<int>(ticks)));

	Trace::start("Battle::update::projectiles->update");
	for (auto it = this->projectiles.begin(); it != this->projectiles.end();)
	{
		auto p = *it++;
		p->update(state, ticks);
	}
	for (auto it = this->projectiles.begin(); it != this->projectiles.end();)
	{
		auto &p = *it++;
		auto c = p->checkProjectileCollision(*map);
		if (c)
		{
			// FIXME: Handle collision
			this->projectiles.erase(c.projectile);
			if (c.projectile->impactSfx)
			{
				fw().soundBackend->playSample(c.projectile->impactSfx, c.position);
			}
			
			// FIXME: Get doodad from weapon definition?
			auto doodad = this->placeDoodad({&state, "DOODAD_EXPLOSION_0"}, c.position);

			switch (c.obj->getType())
			{
				case TileObject::Type::Unit:
				{
					auto unit = std::static_pointer_cast<TileObjectBattleUnit>(c.obj)->getUnit();
					unit->handleCollision(state, c);
					LogWarning("Unit collision");
					break;
				}
				case TileObject::Type::Ground:
				case TileObject::Type::LeftWall:
				case TileObject::Type::RightWall:
				case TileObject::Type::Feature:
				{
					auto mapPartTile = std::static_pointer_cast<TileObjectBattleMapPart>(c.obj);
					// FIXME: Don't just explode mapPart, but damaged tiles/falling stuff? Different
					// explosion doodads? Not all weapons instantly destory buildings too

					auto doodad =
					    this->placeDoodad({&state, "DOODAD_EXPLOSION_2"}, mapPartTile->getCenter());
					mapPartTile->getOwner()->handleCollision(state, c);
					break;
				}
				default:
					LogError("Collision with non-collidable object");
			}
		}
	}
	Trace::end("Battle::update::projectiles->update");
	Trace::start("Battle::update::doors->update");
	for (auto &o : this->doors)
	{
		o->update(state, ticks);
	}
	Trace::end("Battle::update::doors->update");
	Trace::start("Battle::update::map_parts->update");
	for (auto &o : this->map_parts)
	{
		o->update(state, ticks);
	}
	Trace::end("Battle::update::map_parts->update");
	Trace::start("Battle::update::items->update");
	for (auto it = this->items.begin(); it != this->items.end();)
	{
		auto p = *it++;
		p->update(state, ticks);
	}
	Trace::end("Battle::update::items->update");
	Trace::start("Battle::update::units->update");
	for (auto &o : this->units)
	{
		o.second->update(state, ticks);
	}
	Trace::end("Battle::update::units->update");
	Trace::start("Battle::update::doodads->update");
	for (auto it = this->doodads.begin(); it != this->doodads.end();)
	{
		auto d = *it++;
		d->update(state, ticks);
	}
}

void Battle::beginTurn()
{
	if (mode != Mode::TurnBased)
	{
		LogError("beginTurn called in real time?");
		return;
	}
	
	LogWarning("Implement beginning turn!");

	for (auto &u : units)
	{
		if (u.second->owner != currentActiveOrganisation)
		{
			continue;
		}
		u.second->agent->modified_stats.restoreTU();
	}
}

void Battle::endTurn()
{
	if (mode != Mode::TurnBased)
	{
		LogError("endTurn called in real time?");
		return;
	}

	LogWarning("Implement ending turn!");

	// Pass turn to next org, if final org - increment turn counter and pass to first org

	currentTurn++;
	beginTurn();
}

// To be called when battle must be started, before showing battle briefing screen
void Battle::beginBattle(GameState &state, StateRef<Organisation> target_organisation,
                         std::list<StateRef<Agent>> &player_agents, StateRef<Vehicle> player_craft,
                         StateRef<Vehicle> target_craft)
{
	if (state.current_battle)
	{
		LogError("Battle::EnterBattle called while another battle is in progress!");
		return;
	}
	auto b = BattleMap::createBattle(state, target_organisation, player_agents, player_craft,
	                                 target_craft);
	if (!b)
		return;
	state.current_battle = b;
}

// To be called when battle must be started, before showing battle briefing screen
void Battle::beginBattle(GameState &state, StateRef<Organisation> target_organisation,
                         std::list<StateRef<Agent>> &player_agents, StateRef<Vehicle> player_craft,
                         StateRef<Building> target_building)
{
	if (state.current_battle)
	{
		LogError("Battle::EnterBattle called while another battle is in progress!");
		return;
	}
	auto b = BattleMap::createBattle(state, target_organisation, player_agents, player_craft,
	                                 target_building);
	if (!b)
		return;
	state.current_battle = b;
}

// To be called when battle must be started, after the player has assigned agents to squads
void Battle::enterBattle(GameState &state)
{
	if (!state.current_battle)
	{
		LogError("Battle::BeginBattle called with no battle!");
		return;
	}

	auto &b = state.current_battle;

	// FIXME: Spawn units so that hostiles do not have LOS of eacah other

	// Create spawn maps
	std::map<BattleMapSector::LineOfSightBlock::SpawnType,
	         std::list<sp<BattleMapSector::LineOfSightBlock>>>
	    spawnMapsSmallWalker;
	std::map<BattleMapSector::LineOfSightBlock::SpawnType,
	         std::list<sp<BattleMapSector::LineOfSightBlock>>>
	    spawnMapsSmallFlying;
	std::map<BattleMapSector::LineOfSightBlock::SpawnType,
	         std::list<sp<BattleMapSector::LineOfSightBlock>>>
	    spawnMapsSmallAny;
	std::map<BattleMapSector::LineOfSightBlock::SpawnType,
	         std::list<sp<BattleMapSector::LineOfSightBlock>>>
	    spawnMapsLargeFlying;
	std::map<BattleMapSector::LineOfSightBlock::SpawnType,
	         std::list<sp<BattleMapSector::LineOfSightBlock>>>
	    spawnMapsLargeWalker;
	std::map<BattleMapSector::LineOfSightBlock::SpawnType,
	         std::list<sp<BattleMapSector::LineOfSightBlock>>>
	    spawnMapsLargeAny;
	std::map<BattleMapSector::LineOfSightBlock::SpawnType,
	         std::list<sp<BattleMapSector::LineOfSightBlock>>>
	    spawnMapsAnyWalker;
	std::map<BattleMapSector::LineOfSightBlock::SpawnType,
	         std::list<sp<BattleMapSector::LineOfSightBlock>>>
	    spawnMapsAnyFlying;
	std::map<BattleMapSector::LineOfSightBlock::SpawnType,
	         std::list<sp<BattleMapSector::LineOfSightBlock>>>
	    spawnMapsAnyAny;
	std::list<sp<BattleMapSector::LineOfSightBlock>> spawnMapOther;
	std::map<StateRef<Organisation>, BattleMapSector::LineOfSightBlock::SpawnType> spawnTypeMap;

	// Fill organisation to spawn type maps
	for (auto &o : b->participants)
	{
		BattleMapSector::LineOfSightBlock::SpawnType spawnType =
		    BattleMapSector::LineOfSightBlock::SpawnType::Enemy;
		if (o == state.getPlayer())
		{
			spawnType = BattleMapSector::LineOfSightBlock::SpawnType::Player;
		}
		else if (o == state.getCivilian())
		{
			spawnType = BattleMapSector::LineOfSightBlock::SpawnType::Civilian;
		}
		spawnTypeMap[o] = spawnType;
	}

	// Fill spawn maps
	for (auto &l : b->los_blocks)
	{
		if (l->spawn_priority == 0)
		{
			spawnMapOther.push_back(l);
			continue;
		}
		for (int i = 0; i < l->spawn_priority; i++)
		{
			if (l->spawn_large_units)
			{
				if (l->spawn_walking_units)
				{
					spawnMapsLargeWalker[l->spawn_type].push_back(l);
					spawnMapsLargeAny[l->spawn_type].push_back(l);
				}
				else
				{
					spawnMapsLargeFlying[l->spawn_type].push_back(l);
					spawnMapsLargeAny[l->spawn_type].push_back(l);
				}
			}
			else
			{
				if (l->spawn_walking_units)
				{
					spawnMapsSmallWalker[l->spawn_type].push_back(l);
					spawnMapsSmallAny[l->spawn_type].push_back(l);
				}
				else
				{
					spawnMapsSmallFlying[l->spawn_type].push_back(l);
					spawnMapsSmallAny[l->spawn_type].push_back(l);
				}
			}
			if (l->spawn_walking_units)
			{
				spawnMapsAnyWalker[l->spawn_type].push_back(l);
				spawnMapsAnyAny[l->spawn_type].push_back(l);
			}
			else
			{
				spawnMapsAnyFlying[l->spawn_type].push_back(l);
				spawnMapsAnyAny[l->spawn_type].push_back(l);
			}
		}
	}

	// Actually spawn agents
	for (auto &f : state.current_battle->forces)
	{
		for (auto &s : f.second.squads)
		{
			std::vector<sp<BattleUnit>> unitsToSpawn;
			for (auto u : s.units)
				unitsToSpawn.push_back(u);

			while (unitsToSpawn.size() > 0)
			{
				// Determine what kind of units we're trying to spawn
				int neededSpace = 0;
				bool needWalker = false;
				bool needLarge = false;
				for (auto &u : s.units)
				{
					neededSpace++;
					if (u->isLarge())
					{
						needLarge = true;
						neededSpace += 3;
					}
					if (!u->canFly())
					{
						needWalker = true;
					}
				}

				// Make a list of priorities, in which order will we try to find a block
				sp<BattleMapSector::LineOfSightBlock> block = nullptr;
				std::list<std::list<sp<BattleMapSector::LineOfSightBlock>> *> priorityList;
				if (needWalker && needLarge)
				{
					priorityList.push_back(&spawnMapsLargeWalker[spawnTypeMap[f.first]]);
					for (auto l : spawnMapsLargeWalker)
					{
						if (l.first != spawnTypeMap[f.first])
						{
							priorityList.push_back(&spawnMapsLargeWalker[l.first]);
						}
					}
					priorityList.push_back(&spawnMapsLargeFlying[spawnTypeMap[f.first]]);
					for (auto l : spawnMapsLargeFlying)
					{
						if (l.first != spawnTypeMap[f.first])
						{
							priorityList.push_back(&spawnMapsLargeFlying[l.first]);
						}
					}
					priorityList.push_back(&spawnMapsSmallWalker[spawnTypeMap[f.first]]);
					for (auto l : spawnMapsSmallWalker)
					{
						if (l.first != spawnTypeMap[f.first])
						{
							priorityList.push_back(&spawnMapsSmallWalker[l.first]);
						}
					}
					priorityList.push_back(&spawnMapsSmallFlying[spawnTypeMap[f.first]]);
					for (auto l : spawnMapsSmallFlying)
					{
						if (l.first != spawnTypeMap[f.first])
						{
							priorityList.push_back(&spawnMapsSmallFlying[l.first]);
						}
					}
					priorityList.push_back(&spawnMapOther);
				}
				else if (needLarge)
				{
					priorityList.push_back(&spawnMapsLargeAny[spawnTypeMap[f.first]]);
					for (auto l : spawnMapsLargeAny)
					{
						if (l.first != spawnTypeMap[f.first])
						{
							priorityList.push_back(&spawnMapsLargeAny[l.first]);
						}
					}
					priorityList.push_back(&spawnMapsSmallAny[spawnTypeMap[f.first]]);
					for (auto l : spawnMapsSmallAny)
					{
						if (l.first != spawnTypeMap[f.first])
						{
							priorityList.push_back(&spawnMapsSmallAny[l.first]);
						}
					}
					priorityList.push_back(&spawnMapOther);
				}
				else if (needWalker)
				{
					priorityList.push_back(&spawnMapsAnyWalker[spawnTypeMap[f.first]]);
					for (auto l : spawnMapsAnyWalker)
					{
						if (l.first != spawnTypeMap[f.first])
						{
							priorityList.push_back(&spawnMapsAnyWalker[l.first]);
						}
					}
					priorityList.push_back(&spawnMapsAnyFlying[spawnTypeMap[f.first]]);
					for (auto l : spawnMapsAnyFlying)
					{
						if (l.first != spawnTypeMap[f.first])
						{
							priorityList.push_back(&spawnMapsAnyFlying[l.first]);
						}
					}
					priorityList.push_back(&spawnMapOther);
				}
				else
				{
					priorityList.push_back(&spawnMapsAnyAny[spawnTypeMap[f.first]]);
					for (auto l : spawnMapsAnyAny)
					{
						if (l.first != spawnTypeMap[f.first])
						{
							priorityList.push_back(&spawnMapsAnyAny[l.first]);
						}
					}
					priorityList.push_back(&spawnMapOther);
				}

				// Select a block randomly
				for (auto l : priorityList)
				{
					if (l->size() == 0)
						continue;
					block = listRandomiser(state.rng, *l);
					if (block)
						break;
				}

				// If there is no block then just spawn anywhere
				if (!block)
				{
					LogWarning("Map has not enough blocks with spawn points!?!?!?");

					for (int x = 0; x < b->size.x; x++)
					{
						for (int y = 0; y < b->size.y; y++)
						{
							for (int z = 0; z < b->size.z; z++)
							{
								auto u = unitsToSpawn[unitsToSpawn.size() - 1];
								if (u->isLarge())
								{
									if (x < 1 || y < 1 || z >= (b->size.z - 1) ||
									    b->spawnMap[x][y][z] == -1 ||
									    b->spawnMap[x - 1][y][z] == -1 ||
									    b->spawnMap[x][y - 1][z] == -1 ||
									    b->spawnMap[x - 1][y - 1][z] == -1 ||
									    b->spawnMap[x][y][z + 1] == -1 ||
									    b->spawnMap[x - 1][y][z + 1] == -1 ||
									    b->spawnMap[x][y - 1][z + 1] == -1 ||
									    b->spawnMap[x - 1][y - 1][z + 1] == -1)
										continue;
									int height = b->spawnMap[x][y][z];
									height = std::max(b->spawnMap[x][y - 1][z], height);
									height = std::max(b->spawnMap[x - 1][y][z], height);
									height = std::max(b->spawnMap[x - 1][y - 1][z], height);
									b->spawnMap[x][y][z] = -1;
									b->spawnMap[x - 1][y][z] = -1;
									b->spawnMap[x][y - 1][z] = -1;
									b->spawnMap[x - 1][y - 1][z] = -1;
									b->spawnMap[x][y][z + 1] = -1;
									b->spawnMap[x - 1][y][z + 1] = -1;
									b->spawnMap[x][y - 1][z + 1] = -1;
									b->spawnMap[x - 1][y - 1][z + 1] = -1;
									u->position = {x + 0.0f, y + 0.0f,
									               z + ((float)height) / (float)TILE_Z_BATTLE};
									unitsToSpawn.pop_back();
								}
								else
								{
									if (b->spawnMap[x][y][z] == -1)
										continue;
									int height = b->spawnMap[x][y][z];
									b->spawnMap[x][y][z] = -1;
									u->position = {x + 0.5f, y + 0.5f,
									               z + ((float)height) / (float)TILE_Z_BATTLE};
									unitsToSpawn.pop_back();
								}
								if (unitsToSpawn.size() == 0)
									break;
							}
							if (unitsToSpawn.size() == 0)
								break;
						}
						if (unitsToSpawn.size() == 0)
							break;
					}

					if (unitsToSpawn.size() > 0)
					{
						LogError("Map has not big enough to spawn all units!?!?!?");
						return;
					}
					continue;
				} // end of spawning units anywhere in case we can't find a block

				// Actually spawn units
				int startX = randBoundsExclusive(state.rng, block->start.x, block->end.x);
				int startY = randBoundsExclusive(state.rng, block->start.y, block->end.y);
				int z = block->start.z;
				int offset = 0;
				int numSpawned = 0;
				// While we're not completely out of bounds for this block
				// Keep enlarging the offset and spawning units
				while (startX - offset >= block->start.x || startY - offset >= block->start.y ||
				       startX + offset < block->end.x || startY + offset < block->end.y)
				{
					for (int x = startX - offset; x <= startX + offset; x++)
					{
						for (int y = startY - offset; y <= startY + offset; y++)
						{
							if (!block->contains(Vec3<int>{x, y, z}))
								continue;

							auto u = unitsToSpawn[unitsToSpawn.size() - 1];
							if (u->isLarge())
							{
								if (x < 1 || y < 1 || z >= (b->size.z - 1) ||
								    b->spawnMap[x][y][z] == -1 || b->spawnMap[x - 1][y][z] == -1 ||
								    b->spawnMap[x][y - 1][z] == -1 ||
								    b->spawnMap[x - 1][y - 1][z] == -1 ||
								    b->spawnMap[x][y][z + 1] == -1 ||
								    b->spawnMap[x - 1][y][z + 1] == -1 ||
								    b->spawnMap[x][y - 1][z + 1] == -1 ||
								    b->spawnMap[x - 1][y - 1][z + 1] == -1)
									continue;
								int height = b->spawnMap[x][y][z];
								height = std::max(b->spawnMap[x][y - 1][z], height);
								height = std::max(b->spawnMap[x - 1][y][z], height);
								height = std::max(b->spawnMap[x - 1][y - 1][z], height);
								b->spawnMap[x][y][z] = -1;
								b->spawnMap[x - 1][y][z] = -1;
								b->spawnMap[x][y - 1][z] = -1;
								b->spawnMap[x - 1][y - 1][z] = -1;
								b->spawnMap[x][y][z + 1] = -1;
								b->spawnMap[x - 1][y][z + 1] = -1;
								b->spawnMap[x][y - 1][z + 1] = -1;
								b->spawnMap[x - 1][y - 1][z + 1] = -1;
								u->position = {x + 0.0f, y + 0.0f,
								               z + ((float)height) / (float)TILE_Z_BATTLE};
							}
							else
							{
								if (b->spawnMap[x][y][z] == -1)
									continue;
								int height = b->spawnMap[x][y][z];
								b->spawnMap[x][y][z] = -1;
								u->position = {x + 0.5f, y + 0.5f,
								               z + ((float)height) / (float)TILE_Z_BATTLE};
							}
							unitsToSpawn.pop_back();
							numSpawned++;
							if (unitsToSpawn.size() == 0
							    // This makes us spawn every civilian individually
							    || (numSpawned > 0 && f.first == state.getCivilian()))
								break;
						}
						if (unitsToSpawn.size() == 0 ||
						    (numSpawned > 0 && f.first == state.getCivilian()))
							break;
					}
					if (unitsToSpawn.size() == 0 ||
					    (numSpawned > 0 && f.first == state.getCivilian()))
						break;
					offset++;
				} // end of spawning within a block cycle

				// If failed to spawn anything, then this block is no longer appropriate
				if (numSpawned == 0)
				{
					for (auto l : spawnMapsLargeWalker)
					{
						while (true)
						{
							auto pos = std::find(l.second.begin(), l.second.end(), block);
							if (pos == l.second.end())
								break;
							l.second.erase(pos);
						}
					}
					for (auto l : spawnMapsSmallWalker)
					{
						while (true)
						{
							auto pos = std::find(l.second.begin(), l.second.end(), block);
							if (pos == l.second.end())
								break;
							l.second.erase(pos);
						}
					}
					for (auto l : spawnMapsAnyWalker)
					{
						while (true)
						{
							auto pos = std::find(l.second.begin(), l.second.end(), block);
							if (pos == l.second.end())
								break;
							l.second.erase(pos);
						}
					}
					for (auto l : spawnMapsLargeFlying)
					{
						while (true)
						{
							auto pos = std::find(l.second.begin(), l.second.end(), block);
							if (pos == l.second.end())
								break;
							l.second.erase(pos);
						}
					}
					for (auto l : spawnMapsSmallFlying)
					{
						while (true)
						{
							auto pos = std::find(l.second.begin(), l.second.end(), block);
							if (pos == l.second.end())
								break;
							l.second.erase(pos);
						}
					}
					for (auto l : spawnMapsAnyFlying)
					{
						while (true)
						{
							auto pos = std::find(l.second.begin(), l.second.end(), block);
							if (pos == l.second.end())
								break;
							l.second.erase(pos);
						}
					}
					for (auto l : spawnMapsLargeAny)
					{
						while (true)
						{
							auto pos = std::find(l.second.begin(), l.second.end(), block);
							if (pos == l.second.end())
								break;
							l.second.erase(pos);
						}
					}
					for (auto l : spawnMapsSmallAny)
					{
						while (true)
						{
							auto pos = std::find(l.second.begin(), l.second.end(), block);
							if (pos == l.second.end())
								break;
							l.second.erase(pos);
						}
					}
					for (auto l : spawnMapsAnyAny)
					{
						while (true)
						{
							auto pos = std::find(l.second.begin(), l.second.end(), block);
							if (pos == l.second.end())
								break;
							l.second.erase(pos);
						}
					}
					{
						while (true)
						{
							auto pos = std::find(spawnMapOther.begin(), spawnMapOther.end(), block);
							if (pos == spawnMapOther.end())
								break;
							spawnMapOther.erase(pos);
						}
					}
				} // finished erasing filled block
			}
		}
	}

	// Turn units towards map centre
	// Also make sure they're facing in a valid direction
	// And stand in a valid pose
	for (auto p : b->units)
	{
		auto u = p.second;
		int x_diff = (int)(u->position.x - b->size.x / 2);
		int y_diff = (int)(u->position.y - b->size.y / 2);
		if (std::abs(x_diff) > std::abs(y_diff))
		{
			if (x_diff > 0)
			{
				u->facing.x = -1;
			}
			else
			{
				u->facing.x = 1;
			}
		}
		else
		{
			if (y_diff > 0)
			{
				u->facing.y = -1;
			}
			else
			{
				u->facing.y = 1;
			}
		}
		// Facing
		if (!u->agent->isFacingAllowed(u->facing))
		{
			u->facing = setRandomizer(state.rng, u->agent->type->bodyType->allowed_facing);
		}
		// Stance
		if (u->agent->isBodyStateAllowed(AgentType::BodyState::Standing))
		{
			u->setBodyState(AgentType::BodyState::Standing);
		}
		else if (u->agent->isBodyStateAllowed(AgentType::BodyState::Flying))
		{
			u->setBodyState(AgentType::BodyState::Flying);
		}
		else if (u->agent->isBodyStateAllowed(AgentType::BodyState::Kneeling))
		{
			u->setBodyState(AgentType::BodyState::Kneeling);
		}
		else if (u->agent->isBodyStateAllowed(AgentType::BodyState::Prone))
		{
			u->setBodyState(AgentType::BodyState::Prone);
			if (u->canMove() && u->agent->type->bodyType->allowed_facing.size() > 1)
			{
				LogError("Unit %s cannot Stand, Fly or Kneel, but can turn!",
				         u->agent->type.id.cStr());
			}
		}
		else
		{
			LogError("Unit %s cannot Stand, Fly, Kneel or go Prone!", u->agent->type.id.cStr());
		}
		// Miscellaneous
		u->agent->modified_stats.restoreTU();
		u->resetGoal();
	}

	// Find first player unit
	sp<BattleUnit> firstPlayerUnit = nullptr;
	for (auto f : state.current_battle->forces[state.getPlayer()].squads)
	{
		if (f.getNumUnits() > 0)
		{
			firstPlayerUnit = f.units.front();
			break;
		}
	}
	if (!firstPlayerUnit)
	{
		LogError("WTF, no player units found?");
		state.current_battle->battleviewScreenCenter = state.current_battle->map->size / 2;
		state.current_battle->battleviewZLevel = state.current_battle->map->size.z / 2 + 1;
	}
	else
	{
		state.current_battle->battleviewScreenCenter = firstPlayerUnit->position;
		state.current_battle->battleviewZLevel = (int)ceilf(firstPlayerUnit->position.z);
	}

	if (state.current_battle->mission_type == Battle::MissionType::RaidHumans)
	{
		// FIXME: Make X-COM hostile to target org for the duration of this mission
	}

	state.current_battle->initBattle(state);
}

// To be called when battle must be finished and before showing score screen
void Battle::finishBattle(GameState &state)
{
	if (!state.current_battle)
	{
		LogError("Battle::FinishBattle called with no battle!");
		return;
	}
	//  - Identify how battle ended(if enemies present then Failure, otherwise Success)
	//	- (Failure) Determine surviving player agents(kill all player agents that are too far from
	// exits)
	//	- Prepare list of surviving aliens
	//	- (Success) Prepare list of alien bodies
	//	- Remove dead player agents and all enemy agents from the game and vehicles
	//	- Apply experience to stats of living agents
	//	- (Success) Prepare list of loot(including alien saucer equipment)
	//	- Calculate score

	//	(AFTER THIS FUNCTION)
	//  Show score screen
	//	(If mod is on)
	//	- If not enough alien body containment, display alien containment transfer window
	//	- If not enough storage, display storage transfer window
}

// To be called after battle was finished, score screen was shown and before returning to cityscape
void Battle::exitBattle(GameState &state)
{
	if (!state.current_battle)
	{
		LogError("Battle::ExitBattle called with no battle!");
		return;
	}

	state.current_battle->unloadResources(state);
	//  - Apply score
	//	- (UFO mission) Clear UFO crash
	//	- Load loot into vehicles
	//	- Load aliens into bio - trans
	//	- Put surviving aliens back in the building (?or somewhere else if UFO?)
	//  - Restore X-Com relationship to target organisation

	state.current_battle = nullptr;
}

void Battle::loadResources(GameState &state)
{
	battle_map->loadTilesets(state);
	loadImagePacks(state);
	loadAnimationPacks(state);
}

void Battle::unloadResources(GameState &state)
{
	BattleMap::unloadTilesets(state);
	unloadImagePacks(state);
	unloadAnimationPacks(state);
}

void Battle::loadImagePacks(GameState &state)
{
	if (state.battle_unit_image_packs.size() > 0)
	{
		LogInfo("Image packs are already loaded.");
		return;
	}
	// Find out all image packs used by map's units and items
	std::set<UString> imagePacks;
	for (auto &p : units)
	{
		auto &bu = p.second;
		{
			auto packName = BattleUnitImagePack::getNameFromID(bu->agent->type->shadow_pack.id);
			if (imagePacks.find(packName) == imagePacks.end())
				imagePacks.insert(packName);
		}
		for (auto &pv : bu->agent->type->image_packs)
		{
			for (auto &ip : pv)
			{
				auto packName = BattleUnitImagePack::getNameFromID(ip.second.id);
				if (imagePacks.find(packName) == imagePacks.end())
					imagePacks.insert(packName);
			}
		}
		for (auto &ae : bu->agent->equipment)
		{
			{
				auto packName = BattleUnitImagePack::getNameFromID(ae->type->body_image_pack.id);
				if (imagePacks.find(packName) == imagePacks.end())
					imagePacks.insert(packName);
			}
			{
				auto packName = BattleUnitImagePack::getNameFromID(ae->type->held_image_pack.id);
				if (imagePacks.find(packName) == imagePacks.end())
					imagePacks.insert(packName);
			}
		}
	}
	for (auto &bi : items)
	{
		{
			auto packName = BattleUnitImagePack::getNameFromID(bi->item->type->body_image_pack.id);
			if (imagePacks.find(packName) == imagePacks.end())
				imagePacks.insert(packName);
		}
		{
			auto packName = BattleUnitImagePack::getNameFromID(bi->item->type->held_image_pack.id);
			if (imagePacks.find(packName) == imagePacks.end())
				imagePacks.insert(packName);
		}
	}
	// Load all used image packs
	for (auto &imagePackName : imagePacks)
	{
		if (imagePackName.length() == 0)
			continue;
		auto imagePackPath = BattleUnitImagePack::imagePackPath + "/" + imagePackName;
		LogInfo("Loading image pack \"%s\" from \"%s\"", imagePackName.cStr(),
		        imagePackPath.cStr());
		auto imagePack = mksp<BattleUnitImagePack>();
		if (!imagePack->loadImagePack(state, imagePackPath))
		{
			LogError("Failed to load image pack \"%s\" from \"%s\"", imagePackName.cStr(),
			         imagePackPath.cStr());
			continue;
		}
		state.battle_unit_image_packs[UString::format("%s%s", BattleUnitImagePack::getPrefix(),
		                                              imagePackName)] = imagePack;
		LogInfo("Loaded image pack \"%s\" from \"%s\"", imagePackName.cStr(), imagePackPath.cStr());
	}
}

void Battle::unloadImagePacks(GameState &state)
{
	state.battle_unit_image_packs.clear();
	LogInfo("Unloaded all image packs.");
}

void Battle::loadAnimationPacks(GameState &state)
{
	if (state.battle_unit_animation_packs.size() > 0)
	{
		LogInfo("Animation packs are already loaded.");
		return;
	}
	// Find out all animation packs used by units
	std::set<UString> animationPacks;
	for (auto &u : units)
	{
		for (auto &ap : u.second->agent->type->animation_packs)
		{
			auto packName = BattleUnitAnimationPack::getNameFromID(ap.id);
			if (animationPacks.find(packName) == animationPacks.end())
				animationPacks.insert(packName);
		}
	}
	// Load all used animation packs
	for (auto &animationPackName : animationPacks)
	{
		auto animationPackPath =
		    BattleUnitAnimationPack::animationPackPath + "/" + animationPackName;
		LogInfo("Loading animation pack \"%s\" from \"%s\"", animationPackName.cStr(),
		        animationPackPath.cStr());
		auto animationPack = mksp<BattleUnitAnimationPack>();
		if (!animationPack->loadAnimationPack(state, animationPackPath))
		{
			LogError("Failed to load animation pack \"%s\" from \"%s\"", animationPackName.cStr(),
			         animationPackPath.cStr());
			continue;
		}
		state.battle_unit_animation_packs[UString::format(
		    "%s%s", BattleUnitAnimationPack::getPrefix(), animationPackName)] = animationPack;
		LogInfo("Loaded animation pack \"%s\" from \"%s\"", animationPackName.cStr(),
		        animationPackPath.cStr());
	}
}

void Battle::unloadAnimationPacks(GameState &state)
{
	state.battle_unit_animation_packs.clear();
	LogInfo("Unloaded all animation packs.");
}

} // namespace OpenApoc

#ifdef _MSC_VER
#pragma warning (pop)
#endif

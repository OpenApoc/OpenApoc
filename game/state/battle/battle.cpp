#include "game/state/battle/battle.h"
#include "framework/framework.h"
#include "framework/sound.h"
#include "framework/trace.h"
#include "game/state/aequipment.h"
#include "game/state/battle/ai/aitype.h"
#include "game/state/battle/battlecommonimagelist.h"
#include "game/state/battle/battlecommonsamplelist.h"
#include "game/state/battle/battledoor.h"
#include "game/state/battle/battleexplosion.h"
#include "game/state/battle/battlehazard.h"
#include "game/state/battle/battleitem.h"
#include "game/state/battle/battlemap.h"
#include "game/state/battle/battlemappart.h"
#include "game/state/battle/battlemappart_type.h"
#include "game/state/battle/battlescanner.h"
#include "game/state/battle/battleunit.h"
#include "game/state/battle/battleunitanimationpack.h"
#include "game/state/battle/battleunitimagepack.h"
#include "game/state/city/building.h"
#include "game/state/city/city.h"
#include "game/state/city/doodad.h"
#include "game/state/city/projectile.h"
#include "game/state/city/vehicle.h"
#include "game/state/gameevent.h"
#include "game/state/gamestate.h"
#include "game/state/message.h"
#include "game/state/rules/aequipment_type.h"
#include "game/state/rules/damage.h"
#include "game/state/rules/doodad_type.h"
#include "game/state/tileview/collision.h"
#include "game/state/tileview/tile.h"
#include "game/state/tileview/tileobject_battlehazard.h"
#include "game/state/tileview/tileobject_battleitem.h"
#include "game/state/tileview/tileobject_battlemappart.h"
#include "game/state/tileview/tileobject_battleunit.h"
#include "game/state/tileview/tileobject_doodad.h"
#include "game/state/tileview/tileobject_projectile.h"
#include "game/state/tileview/tileobject_shadow.h"
#include "library/strings_format.h"
#include "library/xorshift.h"
#include <algorithm>
#include <glm/glm.hpp>
#include <limits>

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
     TileObject::Type::Unit, TileObject::Type::Shadow, TileObject::Type::Item,
     TileObject::Type::Hazard},
};

Battle::~Battle()
{
	TRACE_FN;
	// Note due to backrefs to Tile*s etc. we need to destroy all tile objects
	// before the TileMap
	map->ceaseBattlescapeUpdates = true;
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
		{
			s->tileObject->removeFromMap();
		}
		s->tileObject = nullptr;
	}
	this->map_parts.clear();
	for (auto &u : this->visibleUnits)
	{
		u.second.clear();
	}
	for (auto &u : this->units)
	{
		u.second->destroy();
	}
	for (auto &i : this->items)
	{
		if (i->tileObject)
			i->tileObject->removeFromMap();
		if (i->shadowObject)
			i->shadowObject->removeFromMap();
		i->tileObject = nullptr;
		i->item->ownerUnit.clear();
	}
	this->items.clear();
	this->doors.clear();
}

void Battle::initBattle(GameState &state, bool first)
{
	common_image_list = state.battle_common_image_list;
	common_sample_list = state.battle_common_sample_list;
	loadResources(state);
	auto stt = shared_from_this();
	for (auto &s : this->map_parts)
	{
		if (s->door)
		{
			s->door->mapParts.push_back(s);
			s->door->position = s->getPosition();
		}
	}
	for (auto &o : this->items)
	{
		o->item->ownerItem = o->shared_from_this();
		o->strategySprite = state.battle_common_image_list->strategyImages->at(480);
	}
	for (auto &o : this->units)
	{
		o.second->strategyImages = state.battle_common_image_list->strategyImages;
		o.second->burningDoodad = state.battle_common_image_list->burningDoodad;
		o.second->genericHitSounds = state.battle_common_sample_list->genericHitSounds;
		o.second->psiSuccessSounds = state.battle_common_sample_list->psiSuccessSounds;
		o.second->psiFailSounds = state.battle_common_sample_list->psiFailSounds;
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
			if (u.second->squadNumber == -1)
			{
				continue;
			}
			forces[u.second->owner].squads[u.second->squadNumber].units[u.second->squadPosition] =
			    u.second;
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
	// From here on, do what needs to be done after map was init
	for (auto &p : this->projectiles)
	{
		if (p->trackedUnit)
			p->trackedObject = p->trackedUnit->tileObject;
	}
	// Fill up los block randomizer
	for (int i = 0; i < (int)losBlocks.size(); i++)
	{
		for (int j = 0; j < losBlocks.at(i)->ai_patrol_priority; j++)
		{
			losBlockRandomizer.push_back(i);
		}
	}

	// Fill up tiles
	tileToLosBlock = std::vector<int>(size.x * size.y * size.z, 0);
	for (int i = 0; i < (int)losBlocks.size(); i++)
	{
		auto l = losBlocks[i];
		for (int x = l->start.x; x < l->end.x; x++)
		{
			for (int y = l->start.y; y < l->end.y; y++)
			{
				for (int z = l->start.z; z < l->end.z; z++)
				{
					tileToLosBlock[z * size.x * size.y + y * size.x + x] = i;
				}
			}
		}
	}
	// Hazards
	for (auto &h : hazards)
	{
		h->updateTileVisionBlock(state);
	}

	// On first run, init support links and items, do vsibility and pathfinding, reset AI
	if (!first)
	{
		return;
	}
	initialMapPartRemoval(state);
	initialMapPartLinkUp();
	for (auto &o : this->items)
	{
		o->tryCollapse();
	}
	// Check which blocks and tiles are visible
	for (auto &u : units)
	{
		u.second->refreshUnitVision(state);
	}
	// Pathfinding
	updatePathfinding(state);
	// AI
	aiBlock.init(state);
	for (auto &o : participants)
	{
		aiBlock.beginTurnRoutine(state, o);
	}
	// leadership
	for (auto &o : participants)
	{
		refreshLeadershipBonus(o);
	}
	// Let pre-placed fires spawn smokes
	StateRef<DamageType> dt = {&state, "DAMAGETYPE_INCENDIARY"};
	std::list<sp<BattleHazard>> fires;
	for (auto &h : hazards)
	{
		if (h->damageType == dt)
		{
			fires.push_back(h);
		}
	}
	for (auto &f : fires)
	{
		f->grow(state);
		f->grow(state);
		f->grow(state);
	}
	// Update units (uses TB function as that's the only thing that needs update)
	for (auto &u : units)
	{
		u.second->updateTB(state);
	}
	// Every thing TB
	if (state.current_battle->mode == Battle::Mode::TurnBased)
	{
		state.current_battle->beginTurn(state);
	}
	// Maybe this battle has no enemies
	checkMissionEnd(state, false);
}

void Battle::initMap()
{
	// If we were generating the map, then map parts are already initiated and we need to init the
	// rest
	if (!this->map)
	{
		this->map.reset(new TileMap(this->size, VELOCITY_SCALE_BATTLE,
		                            {VOXEL_X_BATTLE, VOXEL_Y_BATTLE, VOXEL_Z_BATTLE}, layerMap));
		for (auto &s : this->map_parts)
		{
			if (s->destroyed)
			{
				continue;
			}
			this->map->addObjectToMap(s);
			if (s->type->exit)
			{
				exits.insert(s->position);
			}
		}
		for (auto &h : this->hazards)
		{
			this->map->addObjectToMap(h);
		}
		for (auto &o : this->items)
		{
			this->map->addObjectToMap(o);
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

bool Battle::initialMapCheck(GameState &state, std::list<StateRef<Agent>> agents)
{
	initMap();

	// Mark as low-priority all the enemy spawn los blocks that are seen from any player spawn block
	for (auto &playerSpawn : losBlocks)
	{
		if (playerSpawn->spawn_type != SpawnType::Player || playerSpawn->spawn_priority == 0)
		{
			continue;
		}
		auto sPos = Vec3<float>((playerSpawn->start.x + playerSpawn->end.x) / 2.0f + 0.5f,
		                        (playerSpawn->start.y + playerSpawn->end.y) / 2.0f + 0.5f,
		                        playerSpawn->start.z + 0.5f);
		for (auto &enemySpawn : losBlocks)
		{
			if (enemySpawn->spawn_type != SpawnType::Enemy || enemySpawn->spawn_priority == 0 ||
			    enemySpawn->low_priority)
			{
				continue;
			}
			auto ePos = Vec3<float>((enemySpawn->start.x + enemySpawn->end.x) / 2.0f + 0.5f,
			                        (enemySpawn->start.y + enemySpawn->end.y) / 2.0f + 0.5f,
			                        enemySpawn->start.z + 0.5f);
			if (glm::length(ePos - sPos) > VIEW_DISTANCE ||
			    map->findCollision(sPos, ePos, {}, nullptr, true))
			{
				continue;
			}
			LogWarning("Los block center %s visible from %s", ePos, sPos);
			enemySpawn->low_priority = true;
		}
	}

	// Find out spawn point ratio for enemies
	int spawnPoints = 0;
	int spawnPointsRequired = 0;

	for (auto &enemySpawn : losBlocks)
	{
		if (enemySpawn->spawn_type != SpawnType::Enemy || enemySpawn->low_priority)
		{
			continue;
		}
		for (int x = enemySpawn->start.x; x < enemySpawn->end.x; x++)
		{
			for (int y = enemySpawn->start.y; y < enemySpawn->end.y; y++)
			{
				auto tile = map->getTile(x, y, enemySpawn->start.z);
				if (tile->getCanStand())
				{
					spawnPoints++;
				}
			}
		}
	}
	auto player = state.getPlayer();
	auto civilian = state.getCivilian();
	for (auto &u : agents)
	{
		if (u->owner != player && u->owner != civilian)
		{
			spawnPointsRequired += u->type->bodyType->large ? 4 : 1;
		}
	}

	return spawnPointsRequired == 0 || spawnPoints / spawnPointsRequired >= 4;
}

void linkUpList(std::list<BattleMapPart *> list)
{
	auto next = list.begin();
	auto prev = next++;
	auto cur = next++;

	// In case we are linking to map edge, first map part may be falling and must be linked
	if ((*prev)->willCollapse())
	{
		(*prev)->cancelCollapse();
		(*cur)->supportedParts.emplace_back((*prev)->position, (*prev)->type->type);
	}
	// Link middle
	while (next != list.end())
	{
		(*cur)->cancelCollapse();
		(*prev)->supportedParts.emplace_back((*cur)->position, (*cur)->type->type);
		(*next)->supportedParts.emplace_back((*cur)->position, (*cur)->type->type);

		prev++;
		cur++;
		next++;
	}
	// In case we are linking to map edge, last map part may be falling and must be linked
	if ((*cur)->willCollapse())
	{
		(*cur)->cancelCollapse();
		(*prev)->supportedParts.emplace_back((*cur)->position, (*cur)->type->type);
	}
}

// Remove all solid walls and ground that ended up inside large units
// For now, removes only ground
void Battle::initialMapPartRemoval(GameState &state)
{
	for (auto &u : units)
	{
		if (!u.second->isLarge())
			continue;
		for (int x = 0; x < 1; x++)
		{
			for (int y = 0; y < 1; y++)
			{
				auto t = map->getTile(u.second->position + Vec3<float>{-x, -y, 1.0f});
				if (t->solidGround)
				{
					std::list<sp<TileObjectBattleMapPart>> partsToKill;
					for (auto &o : t->ownedObjects)
					{
						if (o->getType() == TileObject::Type::Ground)
						{
							partsToKill.push_back(
							    std::static_pointer_cast<TileObjectBattleMapPart>(o));
						}
					}
					for (auto &p : partsToKill)
					{
						LogWarning("Removing MP %s at %s as it's blocking unit %s",
						           p->getOwner()->type.id, p->getPosition(), u.first);
						auto mp = p->getOwner();
						mp->destroyed = true;
						mp->tileObject->removeFromMap();
						mp->tileObject.reset();
					}
				}
			}
		}
	}
}

void Battle::initialMapPartLinkUp()
{
	LogWarning("Begun initial map parts link up!");
	auto &mapref = *map;

	for (auto &s : this->map_parts)
	{
		if (!s->destroyed)
		{
			s->queueCollapse();
		}
	}

	for (int z = 0; z < mapref.size.z; z++)
	{
		for (auto &s : this->map_parts)
		{
			if ((int)s->position.z == z && !s->destroyed && s->findSupport())
			{
				s->cancelCollapse();
			}
		}
	}
	LogWarning("Begun map parts link up cycle!");
	bool foundSupport;
	// Establish support based on existing supported map parts
	do
	{
		foundSupport = false;
		for (auto &s : this->map_parts)
		{
			if (!s->willCollapse())
			{
				continue;
			}
			if (s->findSupport())
			{
				s->cancelCollapse();
				foundSupport = true;
			}
		}
	} while (foundSupport);

	// Report unlinked parts
	for (auto &mp : this->map_parts)
	{
		if (mp->willCollapse())
		{
			auto pos = mp->tileObject->getOwningTile()->position;
			LogWarning("MP %s SBT %d at %d %d %d is UNLINKED", mp->type.id,
			           (int)mp->type->getVanillaSupportedById(), pos.x, pos.y, pos.z);
		}
	}

	LogWarning("Attempting link up of unlinked parts");
	// Try to link to objects of same type first, then to anything
	for (int iteration = 0; iteration <= 2; iteration++)
	{
		bool skipTypeCheck = iteration > 0;
		bool skipHardCheck = iteration > 1;
		do
		{
			foundSupport = false;
			for (auto &s : this->map_parts)
			{
				if (!s->willCollapse())
				{
					continue;
				}
				if (s->attachToSomething(!skipTypeCheck, !skipHardCheck))
				{
					s->cancelCollapse();
					foundSupport = true;
				}
			}
		} while (foundSupport);
	}

	// Report unlinked parts
	for (auto &mp : this->map_parts)
	{
		if (mp->willCollapse())
		{
			auto pos = mp->tileObject->getOwningTile()->position;
			LogWarning("MP %s SBT %d at %d %d %d is going to fall", mp->type.id,
			           (int)mp->type->getVanillaSupportedById(), pos.x, pos.y, pos.z);
		}
	}

	mapref.updateAllBattlescapeInfo();
	LogWarning("Link up finished!");
}

void Battle::initialUnitSpawn(GameState &state)
{
	enum class UnitSize
	{
		Small,
		Large,
		Any
	};
	enum class UnitMovement
	{
		Walking,
		Flying,
		Any
	};
	class SpawnBlock
	{
	  public:
		// true = walker, false = flyer
		std::map<bool, std::set<Vec3<int>>> positions;
		Vec3<int> start;
		Vec3<int> end;
	};
	class SpawnKey
	{
	  public:
		SpawnType spawnType = SpawnType::Player;
		UnitSize unitSize = UnitSize::Small;
		UnitMovement unitMovement = UnitMovement::Walking;
		bool lowPriority = false;
		SpawnKey() = default;
		SpawnKey(SpawnType spawnType, UnitSize unitSize, UnitMovement unitMovement,
		         bool lowPriority)
		    : spawnType(spawnType), unitSize(unitSize), unitMovement(unitMovement),
		      lowPriority(lowPriority)
		{
		}
		bool operator<(const SpawnKey &other) const
		{
			return std::tie(spawnType, unitSize, unitMovement, lowPriority) <
			       std::tie(other.spawnType, other.unitSize, other.unitMovement, other.lowPriority);
		}
	};

	std::map<SpawnKey, std::list<sp<SpawnBlock>>> spawnMaps;
	std::map<SpawnKey, std::list<sp<SpawnBlock>>> spawnInverse;
	// Other blocks that have 0 spawn priority, to be used when all others are exhausted
	std::list<sp<SpawnBlock>> spawnOther;
	std::map<StateRef<Organisation>, SpawnType> spawnTypeMap;

	// Fill organisation -> spawnType maps
	for (auto &o : participants)
	{
		SpawnType spawnType = SpawnType::Enemy;
		if (o == state.getPlayer())
		{
			spawnType = SpawnType::Player;
		}
		else if (o == state.getCivilian())
		{
			spawnType = SpawnType::Civilian;
		}
		spawnTypeMap[o] = spawnType;
	}

	// Fill spawn blocks
	std::vector<sp<SpawnBlock>> spawnBlocks;
	for (int i = 0; i < losBlocks.size(); i++)
	{
		auto sb = mksp<SpawnBlock>();
		auto &lb = losBlocks[i];
		for (int x = lb->start.x; x < lb->end.x; x++)
		{
			for (int y = lb->start.y; y < lb->end.y; y++)
			{
				auto tile = map->getTile(x, y, lb->start.z);
				if (tile->getPassable(false, 32))
				{
					sb->positions[false].emplace(x, y, lb->start.z);
					if (tile->getCanStand())
					{
						sb->positions[true].emplace(x, y, lb->start.z);
					}
				}
			}
		}
		sb->start = lb->start;
		sb->end = lb->end;
		spawnBlocks.push_back(sb);

		if (lb->spawn_priority == 0)
		{
			spawnOther.push_back(sb);
			continue;
		}
		for (int i = 0; i < lb->spawn_priority; i++)
		{
			spawnMaps[{lb->spawn_type, lb->spawn_large_units ? UnitSize::Large : UnitSize::Small,
			           lb->spawn_walking_units ? UnitMovement::Walking : UnitMovement::Flying,
			           lb->low_priority}]
			    .push_back(sb);
			spawnMaps[{lb->spawn_type, lb->spawn_large_units ? UnitSize::Large : UnitSize::Small,
			           UnitMovement::Any, lb->low_priority}]
			    .push_back(sb);
			spawnMaps[{lb->spawn_type, UnitSize::Any,
			           lb->spawn_walking_units ? UnitMovement::Walking : UnitMovement::Flying,
			           lb->low_priority}]
			    .push_back(sb);
			spawnMaps[{lb->spawn_type, UnitSize::Any, UnitMovement::Any, lb->low_priority}]
			    .push_back(sb);
			for (int j = 0; j < 3; j++)
			{
				auto st = (SpawnType)j;
				if (st == lb->spawn_type)
				{
					continue;
				}
				spawnInverse[{st, lb->spawn_large_units ? UnitSize::Large : UnitSize::Small,
				              lb->spawn_walking_units ? UnitMovement::Walking
				                                      : UnitMovement::Flying,
				              lb->low_priority}]
				    .push_back(sb);
				spawnInverse[{st, lb->spawn_large_units ? UnitSize::Large : UnitSize::Small,
				              UnitMovement::Any, lb->low_priority}]
				    .push_back(sb);
				spawnInverse[{st, UnitSize::Any, lb->spawn_walking_units ? UnitMovement::Walking
				                                                         : UnitMovement::Flying,
				              lb->low_priority}]
				    .push_back(sb);
				spawnInverse[{st, UnitSize::Any, UnitMovement::Any, lb->low_priority}].push_back(
				    sb);
			}
		}
	}

	// Spawn agents with spawn locations provided
	for (auto &f : state.current_battle->forces)
	{
		for (auto &s : f.second.squads)
		{
			for (auto &u : s.units)
			{
				if (spawnLocations[u->agent->type].empty())
				{
					continue;
				}
				auto pos = listRandomiser(state.rng, spawnLocations[u->agent->type]);
				spawnLocations[u->agent->type].remove(pos);
				auto tile = map->getTile(pos.x, pos.y, pos.z);
				u->position = tile->getRestingPosition(u->isLarge());
			}
		}
	}

	// Actually spawn agents
	for (auto &f : state.current_battle->forces)
	{
		// All units to spawn, grouped by squads, squadless in the back
		std::list<std::list<sp<BattleUnit>>> unitGroupsToSpawn;
		// Add squadless
		unitGroupsToSpawn.emplace_back();
		for (auto &u : units)
		{
			if (u.second->owner == f.first && u.second->squadNumber == -1 &&
			    u.second->position == Vec3<float>{-1.0, -1.0, -1.0})
			{
				unitGroupsToSpawn.front().push_back(u.second);
			}
		}
		// Add squads
		for (auto &s : f.second.squads)
		{
			unitGroupsToSpawn.emplace_front();
			for (auto &u : s.units)
			{
				if (u->position == Vec3<float>{-1.0, -1.0, -1.0})
				{
					unitGroupsToSpawn.front().push_back(u);
				}
			}
		}
		// Go through groups and spawn
		for (auto &list : unitGroupsToSpawn)
		{
			auto &unitsToSpawn = list;

			while (unitsToSpawn.size() > 0)
			{
				// Determine what kind of units we're trying to spawn
				bool needWalker = false;
				bool needLarge = false;
				for (auto &u : unitsToSpawn)
				{
					if (u->isLarge())
					{
						needLarge = true;
					}
					if (!u->canFly())
					{
						needWalker = true;
					}
				}

				// Make a list of priorities, in which order will we try to find a block
				sp<SpawnBlock> block = nullptr;
				std::list<std::list<sp<SpawnBlock>> *> priorityList;
				if (needWalker && needLarge)
				{
					priorityList.push_back(&spawnMaps[{spawnTypeMap[{f.first}], UnitSize::Large,
					                                   UnitMovement::Walking, false}]);
					priorityList.push_back(&spawnMaps[{spawnTypeMap[{f.first}], UnitSize::Large,
					                                   UnitMovement::Walking, true}]);
					priorityList.push_back(&spawnInverse[{spawnTypeMap[{f.first}], UnitSize::Large,
					                                      UnitMovement::Walking, false}]);
					priorityList.push_back(&spawnInverse[{spawnTypeMap[{f.first}], UnitSize::Large,
					                                      UnitMovement::Walking, true}]);

					priorityList.push_back(&spawnMaps[{spawnTypeMap[{f.first}], UnitSize::Large,
					                                   UnitMovement::Flying, false}]);
					priorityList.push_back(&spawnMaps[{spawnTypeMap[{f.first}], UnitSize::Large,
					                                   UnitMovement::Flying, true}]);
					priorityList.push_back(&spawnInverse[{spawnTypeMap[{f.first}], UnitSize::Large,
					                                      UnitMovement::Flying, false}]);
					priorityList.push_back(&spawnInverse[{spawnTypeMap[{f.first}], UnitSize::Large,
					                                      UnitMovement::Flying, true}]);

					priorityList.push_back(&spawnMaps[{spawnTypeMap[{f.first}], UnitSize::Small,
					                                   UnitMovement::Walking, false}]);
					priorityList.push_back(&spawnMaps[{spawnTypeMap[{f.first}], UnitSize::Small,
					                                   UnitMovement::Walking, true}]);
					priorityList.push_back(&spawnInverse[{spawnTypeMap[{f.first}], UnitSize::Small,
					                                      UnitMovement::Walking, false}]);
					priorityList.push_back(&spawnInverse[{spawnTypeMap[{f.first}], UnitSize::Small,
					                                      UnitMovement::Walking, true}]);

					priorityList.push_back(&spawnMaps[{spawnTypeMap[{f.first}], UnitSize::Small,
					                                   UnitMovement::Flying, false}]);
					priorityList.push_back(&spawnMaps[{spawnTypeMap[{f.first}], UnitSize::Small,
					                                   UnitMovement::Flying, true}]);
					priorityList.push_back(&spawnInverse[{spawnTypeMap[{f.first}], UnitSize::Small,
					                                      UnitMovement::Flying, false}]);
					priorityList.push_back(&spawnInverse[{spawnTypeMap[{f.first}], UnitSize::Small,
					                                      UnitMovement::Flying, true}]);
				}
				else if (needLarge)
				{
					priorityList.push_back(&spawnMaps[{spawnTypeMap[{f.first}], UnitSize::Large,
					                                   UnitMovement::Any, false}]);
					priorityList.push_back(&spawnMaps[{spawnTypeMap[{f.first}], UnitSize::Large,
					                                   UnitMovement::Any, true}]);
					priorityList.push_back(&spawnInverse[{spawnTypeMap[{f.first}], UnitSize::Large,
					                                      UnitMovement::Any, false}]);
					priorityList.push_back(&spawnInverse[{spawnTypeMap[{f.first}], UnitSize::Large,
					                                      UnitMovement::Any, true}]);

					priorityList.push_back(&spawnMaps[{spawnTypeMap[{f.first}], UnitSize::Small,
					                                   UnitMovement::Any, false}]);
					priorityList.push_back(&spawnMaps[{spawnTypeMap[{f.first}], UnitSize::Small,
					                                   UnitMovement::Any, true}]);
					priorityList.push_back(&spawnInverse[{spawnTypeMap[{f.first}], UnitSize::Small,
					                                      UnitMovement::Any, false}]);
					priorityList.push_back(&spawnInverse[{spawnTypeMap[{f.first}], UnitSize::Small,
					                                      UnitMovement::Any, true}]);
				}
				else if (needWalker)
				{
					priorityList.push_back(&spawnMaps[{spawnTypeMap[{f.first}], UnitSize::Any,
					                                   UnitMovement::Walking, false}]);
					priorityList.push_back(&spawnMaps[{spawnTypeMap[{f.first}], UnitSize::Any,
					                                   UnitMovement::Walking, true}]);
					priorityList.push_back(&spawnInverse[{spawnTypeMap[{f.first}], UnitSize::Any,
					                                      UnitMovement::Walking, false}]);
					priorityList.push_back(&spawnInverse[{spawnTypeMap[{f.first}], UnitSize::Any,
					                                      UnitMovement::Walking, true}]);

					priorityList.push_back(&spawnMaps[{spawnTypeMap[{f.first}], UnitSize::Any,
					                                   UnitMovement::Flying, false}]);
					priorityList.push_back(&spawnMaps[{spawnTypeMap[{f.first}], UnitSize::Any,
					                                   UnitMovement::Flying, true}]);
					priorityList.push_back(&spawnInverse[{spawnTypeMap[{f.first}], UnitSize::Any,
					                                      UnitMovement::Flying, false}]);
					priorityList.push_back(&spawnInverse[{spawnTypeMap[{f.first}], UnitSize::Any,
					                                      UnitMovement::Flying, true}]);
				}
				else
				{
					priorityList.push_back(&spawnMaps[{spawnTypeMap[{f.first}], UnitSize::Any,
					                                   UnitMovement::Any, false}]);
					priorityList.push_back(&spawnMaps[{spawnTypeMap[{f.first}], UnitSize::Any,
					                                   UnitMovement::Any, true}]);
					priorityList.push_back(&spawnInverse[{spawnTypeMap[{f.first}], UnitSize::Any,
					                                      UnitMovement::Any, false}]);
					priorityList.push_back(&spawnInverse[{spawnTypeMap[{f.first}], UnitSize::Any,
					                                      UnitMovement::Any, true}]);
				}
				priorityList.push_back(&spawnOther);

				// Select a block randomly
				for (auto &l : priorityList)
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

					for (int x = 0; x < size.x; x++)
					{
						for (int y = 0; y < size.y; y++)
						{
							for (int z = 0; z < size.z; z++)
							{
								auto tile = map->getTile(x, y, z);
								auto u = unitsToSpawn.back();
								if (!tile->getPassable(u->isLarge(),
								                       u->agent->type->bodyType->maxHeight) ||
								    (!u->canFly() && !tile->getCanStand(u->isLarge())))
								{
									continue;
								}
								u->position = tile->getRestingPosition(u->isLarge());
								unitsToSpawn.pop_back();
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

				// Spawn units within a block
				int numSpawned = 0;
				if (!block->positions[needWalker].empty())
				{
					auto spawnPos = setRandomiser(state.rng, block->positions[needWalker]);
					int z = spawnPos.z;
					int offset = 0;
					bool stopSpawning = false;
					// While we're not completely out of bounds for this block
					// Keep enlarging the offset and spawning units
					while (spawnPos.x - offset >= block->start.x ||
					       spawnPos.y - offset >= block->start.y ||
					       spawnPos.x + offset < block->end.x || spawnPos.y + offset < block->end.y)
					{
						for (int x = spawnPos.x - offset; x <= spawnPos.x + offset; x++)
						{
							for (int y = spawnPos.y - offset; y <= spawnPos.y + offset; y++)
							{
								auto pos = Vec3<int>{x, y, z};
								if (block->positions[needWalker].find(pos) ==
								    block->positions[needWalker].end())
									continue;

								auto tile = map->getTile(pos);
								auto u = unitsToSpawn.back();
								if (!tile->getPassable(u->isLarge(),
								                       u->agent->type->bodyType->maxHeight) ||
								    (!u->canFly() && !tile->getCanStand(u->isLarge())))
								{
									continue;
								}
								u->position = tile->getRestingPosition(u->isLarge());
								// Clear positions from list
								if (u->isLarge())
								{
									for (int dx = -1; dx <= 0; dx++)
									{
										for (int dy = -1; dy <= 0; dy++)
										{
											auto nPos = pos + Vec3<int>{dx, dy, 0};
											if (block->positions[false].find(nPos) !=
											    block->positions[false].end())
											{
												block->positions[false].erase(nPos);
											}
											if (block->positions[true].find(nPos) !=
											    block->positions[true].end())
											{
												block->positions[true].erase(nPos);
											}
										}
									}
								}
								else
								{
									if (block->positions[false].find(pos) !=
									    block->positions[false].end())
									{
										block->positions[false].erase(pos);
									}
									if (block->positions[true].find(pos) !=
									    block->positions[true].end())
									{
										block->positions[true].erase(pos);
									}
								}
								// FIXME: Consider prone units
								/*
								((!u->agent->isBodyStateAllowed(BodyState::Standing) &&
								!u->agent->isBodyStateAllowed(BodyState::Flying) &&
								!u->agent->isBodyStateAllowed(BodyState::Kneeling)) &&
								!u->canProne(...)
								*/
								unitsToSpawn.pop_back();
								numSpawned++;
								if (unitsToSpawn.size() == 0
								    // This makes us spawn every civilian and loner individually
								    || (numSpawned > 0 && (u->getAIType() == AIType::None ||
								                           u->getAIType() == AIType::Loner ||
								                           u->getAIType() == AIType::Civilian)))
								{
									stopSpawning = true;
									break;
								}
							}
							if (stopSpawning)
								break;
						}
						if (stopSpawning)
							break;
						offset++;
					} // end of spawning within a block cycle
				}     // end of if we have a position in a set

				// If failed to spawn anything, then this block is no longer appropriate
				if (numSpawned == 0)
				{
					for (auto &l : spawnMaps)
					{
						while (true)
						{
							auto pos = std::find(l.second.begin(), l.second.end(), block);
							if (pos == l.second.end())
								break;
							l.second.erase(pos);
						}
					}
					for (auto &l : spawnInverse)
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
							auto pos = std::find(spawnOther.begin(), spawnOther.end(), block);
							if (pos == spawnOther.end())
								break;
							spawnOther.erase(pos);
						}
					}
				} // finished erasing filled block
			}
		}
	}

	// Turn units towards map centre
	// Also make sure they're facing in a valid direction
	// And stand in a valid pose
	for (auto &p : units)
	{
		auto u = p.second;
		int x_diff = (int)(u->position.x - size.x / 2);
		int y_diff = (int)(u->position.y - size.y / 2);
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
			u->facing = setRandomizer(state.rng, *u->agent->getAllowedFacings());
		}
		// Stance
		if (u->agent->isBodyStateAllowed(BodyState::Standing))
		{
			u->setBodyState(state, BodyState::Standing);
			u->setMovementMode(MovementMode::Walking);
		}
		else if (u->agent->isBodyStateAllowed(BodyState::Flying))
		{
			u->setBodyState(state, BodyState::Flying);
			u->setMovementMode(MovementMode::Walking);
		}
		else if (u->agent->isBodyStateAllowed(BodyState::Kneeling))
		{
			u->setBodyState(state, BodyState::Kneeling);
			u->setMovementMode(MovementMode::Prone);
		}
		else if (u->agent->isBodyStateAllowed(BodyState::Prone))
		{
			u->setBodyState(state, BodyState::Prone);
			u->setMovementMode(MovementMode::Prone);
		}
		else
		{
			LogError("Unit %s cannot Stand, Fly, Kneel or go Prone!", u->agent->type.id);
		}
		// Miscellaneous
		u->beginTurn(state);
		u->resetGoal();
	}
}

void Battle::setMode(Mode mode) { this->mode = mode; }

sp<Doodad> Battle::placeDoodad(StateRef<DoodadType> type, Vec3<float> position)
{
	auto doodad = mksp<Doodad>(position, type);
	if (map)
	{
		map->addObjectToMap(doodad);
	}
	this->doodads.push_back(doodad);
	return doodad;
}

sp<BattleUnit> Battle::spawnUnit(GameState &state, StateRef<Organisation> owner,
                                 StateRef<AgentType> agentType, Vec3<float> position,
                                 Vec2<int> facing, BodyState curState, BodyState tarState)
{
	auto agent = state.agent_generator.createAgent(state, owner, agentType);
	auto unit = state.current_battle->placeUnit(state, agent, position);
	unit->falling = true;
	unit->setFacing(state, facing);
	unit->setBodyState(state, curState);
	unit->setMission(state, BattleUnitMission::changeStance(*unit, tarState));
	unit->assignToSquad(*state.current_battle);
	unit->refreshUnitVisibilityAndVision(state);

	unit->strategyImages = state.battle_common_image_list->strategyImages;
	unit->burningDoodad = state.battle_common_image_list->burningDoodad;
	unit->genericHitSounds = state.battle_common_sample_list->genericHitSounds;
	unit->psiSuccessSounds = state.battle_common_sample_list->psiSuccessSounds;
	unit->psiFailSounds = state.battle_common_sample_list->psiFailSounds;

	return unit;
}

sp<BattleExplosion> Battle::addExplosion(GameState &state, Vec3<int> position,
                                         StateRef<DoodadType> doodadType,
                                         StateRef<DamageType> damageType, int power,
                                         int depletionRate, StateRef<Organisation> ownerOrg,
                                         StateRef<BattleUnit> ownerUnit)
{
	// FIXME: Actually read this option
	bool USER_OPTION_EXPLOSIONS_DAMAGE_IN_THE_END = true;

	// Doodad
	if (!doodadType)
	{
		doodadType = {&state, "DOODAD_30_EXPLODING_PAYLOAD"};
	}
	placeDoodad(doodadType, position);

	// Sound
	if (!damageType->explosionSounds.empty())
	{
		fw().soundBackend->playSample(listRandomiser(state.rng, damageType->explosionSounds),
		                              position);
	}

	// Explosion
	auto explosion =
	    mksp<BattleExplosion>(position, damageType, power, depletionRate,
	                          USER_OPTION_EXPLOSIONS_DAMAGE_IN_THE_END, ownerOrg, ownerUnit);
	explosions.insert(explosion);
	return explosion;
}

sp<BattleUnit> Battle::placeUnit(GameState &state, StateRef<Agent> agent)
{
	auto unit = mksp<BattleUnit>();
	UString id = BattleUnit::generateObjectID(state);
	unit->id = id;
	unit->agent = agent;
	unit->strategyImages = state.battle_common_image_list->strategyImages;
	unit->genericHitSounds = state.battle_common_sample_list->genericHitSounds;
	unit->squadNumber = -1;
	unit->cloakTicksAccumulated = CLOAK_TICKS_REQUIRED;
	unit->position = {-1.0, -1.0, -1.0};
	units[id] = unit;
	unit->init(state);
	return unit;
}

sp<BattleUnit> Battle::placeUnit(GameState &state, StateRef<Agent> agent, Vec3<float> position)
{
	auto u = placeUnit(state, agent);
	u->position = position;
	if (map)
	{
		map->addObjectToMap(u);
	}
	return u;
}

sp<BattleDoor> Battle::addDoor(GameState &state)
{
	auto door = mksp<BattleDoor>();
	UString id = BattleDoor::generateObjectID(state);
	door->id = id;
	door->doorSound = state.battle_common_sample_list->door;
	doors[id] = door;
	return door;
}

sp<BattleItem> Battle::placeItem(GameState &state, sp<AEquipment> item, Vec3<float> position)
{
	auto bitem = mksp<BattleItem>();
	bitem->strategySprite = state.battle_common_image_list->strategyImages->at(480);
	bitem->item = item;
	item->ownerItem = bitem;
	if (item->ownerUnit)
	{
		bitem->owner = item->ownerUnit->owner;
	}
	else if (item->ownerAgent)
	{
		bitem->owner = item->ownerAgent->owner;
	}
	else
	{
		bitem->owner = item->ownerOrganisation;
	}
	bitem->position = position;
	if (map)
	{
		map->addObjectToMap(bitem);
	}
	items.push_back(bitem);
	return bitem;
}

sp<BattleHazard> Battle::placeHazard(GameState &state, StateRef<Organisation> owner,
                                     StateRef<BattleUnit> unit, StateRef<DamageType> type,
                                     Vec3<int> position, int ttl, int power,
                                     int initialAgeTTLDivizor, bool delayVisibility)
{
	bool fire = type->hazardType->fire;
	auto hazard = mksp<BattleHazard>(state, type, delayVisibility);
	hazard->ownerOrganisation = owner;
	hazard->ownerUnit = unit;
	hazard->position = position;
	hazard->position += Vec3<float>{0.5f, 0.5f, 0.5f};
	if (fire)
	{
		// lifetime means nothing for fire
		hazard->lifetime = 0;
		// age means how "powerful" the fire is, where 10 is most powerful and 110 is almost dead
		hazard->age = 100 - ttl * 6;
		// Fire is growing, when fading this will be negative
		hazard->power = randBoundsInclusive(state.rng, 1, 2);
	}
	else
	{
		hazard->lifetime = ttl;
		hazard->age = hazard->lifetime * (initialAgeTTLDivizor - 1) / initialAgeTTLDivizor;
		hazard->power = power;
	}
	// Remove existing hazard, ensure possible to do this, place this
	if (map)
	{
		auto tile = map->getTile(position);
		// Cannot add non-fire hazard if tile is blocked or fire hazard if nothing is there to burn
		// at
		if ((!fire && tile->height * 40.0f > 38.0f) || (fire && tile->height * 40.0f < 1.0f))
		{
			return nullptr;
		}
		// Clear existing hazards
		sp<BattleHazard> existingHazard;
		for (auto &obj : tile->ownedObjects)
		{
			if (obj->getType() == TileObject::Type::Hazard)
			{
				existingHazard = std::static_pointer_cast<TileObjectBattleHazard>(obj)->getHazard();
			}
		}
		if (existingHazard)
		{
			// Fire cannot spread into another fire
			if (fire && existingHazard->hazardType->fire)
			{
				return nullptr;
			}
			else
			{
				// Nothing can spread into a fire that's eating up a feature
				if (existingHazard->hazardType->fire)
				{
					LogWarning(
					    "Ensure we are not putting out a fire that is attached to a feature!");
				}
				existingHazard->die(state, false);
			}
		}
		map->addObjectToMap(hazard);
		hazard->updateTileVisionBlock(state);
	}
	// Insert new hazard
	hazards.insert(hazard);
	return hazard;
}

sp<BattleScanner> Battle::addScanner(GameState &state, AEquipment &item)
{
	auto scanner = mksp<BattleScanner>();
	UString id = BattleScanner::generateObjectID(state);
	item.battleScanner = {&state, id};
	scanner->holder = item.ownerAgent->unit;
	scanner->lastPosition = scanner->holder->position;
	scanners[id] = scanner;
	return scanner;
}

void Battle::removeScanner(GameState &state, AEquipment &item)
{
	state.current_battle->scanners.erase(item.battleScanner.id);
	item.battleScanner.clear();
}

void Battle::updateProjectiles(GameState &state, unsigned int ticks)
{
	for (auto it = this->projectiles.begin(); it != this->projectiles.end();)
	{
		auto p = *it++;
		p->update(state, ticks);
	}
	for (auto it = this->projectiles.begin(); it != this->projectiles.end();)
	{
		auto &p = *it++;
		notifyAction(p->position);
		auto c = p->checkProjectileCollision(*map);
		if (c)
		{
			// Alert intended unit that he's on fire
			auto unit = c.projectile->trackedUnit;
			if (unit)
			{
				if (unit->visibleUnits.find(c.projectile->firerUnit) == unit->visibleUnits.end())
				{
					LogWarning("Notify: unit %s that he's taking fire",
					           c.projectile->trackedUnit.id);
				}

				unit->notifyUnderFire(state, c.projectile->firerPosition,
				                      c.projectile->firerUnit->owner == unit->owner ||
				                          visibleUnits[unit->owner].find(c.projectile->firerUnit) !=
				                              visibleUnits[unit->owner].end());
			}
			// Handle collision
			this->projectiles.erase(c.projectile);
			bool playSound = true;
			bool displayDoodad = true;
			if (c.projectile->damageType->explosive)
			{
				auto explosion = addExplosion(
				    state, c.position, c.projectile->doodadType, c.projectile->damageType,
				    c.projectile->damage, c.projectile->depletionRate,
				    c.projectile->firerUnit->owner, c.projectile->firerUnit);
				displayDoodad = false;
			}
			else
			{
				switch (c.obj->getType())
				{
					case TileObject::Type::Unit:
					{
						auto unit =
						    std::static_pointer_cast<TileObjectBattleUnit>(c.obj)->getUnit();
						displayDoodad = !unit->handleCollision(state, c);
						playSound = false;
						break;
					}
					case TileObject::Type::Ground:
					case TileObject::Type::LeftWall:
					case TileObject::Type::RightWall:
					case TileObject::Type::Feature:
					{
						auto mapPartTile = std::static_pointer_cast<TileObjectBattleMapPart>(c.obj);
						displayDoodad = !mapPartTile->getOwner()->handleCollision(state, c);
						playSound = displayDoodad;
						break;
					}
					default:
						LogError("Collision with non-collidable object");
				}
			}
			if (displayDoodad)
			{
				auto doodadType = c.projectile->doodadType;
				if (doodadType)
				{
					auto doodad = this->placeDoodad(doodadType, c.position);
				}
			}
			if (playSound)
			{
				if (c.projectile->impactSfx)
				{
					fw().soundBackend->playSample(c.projectile->impactSfx, c.position);
				}
			}
		}
	}
}

void Battle::updateVision(GameState &state)
{
	std::set<sp<BattleUnit>> unitsToUpdate;
	for (auto &entry : units)
	{
		auto unit = entry.second;
		if (!unit->isConscious())
		{
			continue;
		}
		for (auto &pos : tilesChangedForVision)
		{
			auto vec = pos - (Vec3<int>)unit->position;
			// Quick check it's to the right side of us and in range
			// FIXME: Should we check more thoroughly to save CPU time (probably)?
			if ((vec.x > 0 && unit->facing.x < 0) || (vec.y > 0 && unit->facing.y < 0) ||
			    (vec.x < 0 && unit->facing.x > 0) || (vec.y < 0 && unit->facing.y > 0) ||
			    (vec.x * vec.x + vec.y * vec.y + vec.z * vec.z > 400))
			{
				continue;
			}
			unitsToUpdate.insert(unit);
			break;
		}
	}
	for (auto &unit : unitsToUpdate)
	{
		unit->refreshUnitVision(state);
	}
	tilesChangedForVision.clear();
}

bool findLosBlockCenter(TileMap &map, BattleUnitType type,
                        const BattleMapSector::LineOfSightBlock &lb, Vec3<int> center,
                        Vec3<int> &closestValidPos)
{
	bool large = type == BattleUnitType::LargeFlyer || type == BattleUnitType::LargeWalker;
	bool flying = type == BattleUnitType::LargeFlyer || type == BattleUnitType::SmallFlyer;
	int height = large ? 70 : 32;
	int dist = -1;
	bool somethingHappened;
	do
	{
		dist++;
		somethingHappened = false;
		for (int dx = -dist; dx <= dist; dx++)
		{
			int x = center.x + dx;
			if (x < lb.start.x || x >= lb.end.x)
			{
				continue;
			}
			for (int dy = -dist; dy <= dist; dy++)
			{
				int y = center.y + dy;
				if (y < lb.start.y || y >= lb.end.y)
				{
					continue;
				}
				for (int dz = -dist; dz <= dist; dz++)
				{
					int z = center.z + dz;
					if (z < lb.start.z || z >= lb.end.z)
					{
						continue;
					}
					// At least one coord must be at the edge so that we don't re-check already
					// checked points
					if (std::abs(dx) != dist && std::abs(dy) != dist && std::abs(dz) != dist)
					{
						continue;
					}
					somethingHappened = true;

					auto t = map.getTile(x, y, z);
					if (t->getPassable(large, height) && (flying || t->getCanStand(large)))
					{
						closestValidPos = {x, y, z};
						return true;
					}
				}
			}
		}
	} while (somethingHappened);

	return false;
}

void Battle::updatePathfinding(GameState &)
{
	// How much attempts are given to the pathfinding until giving up and concluding that
	// there is no path between two sectors. This is a multiplier for "distance", which is
	// a minimum number of iterations required to pathfind between two locations
	static const int PATH_ITERATION_LIMIT_MULTIPLIER = 2;

	// How much can resulting path differ from optimal path
	static const int PATH_COST_LIMIT_MULTIPLIER = 2;

	int lbCount = losBlocks.size();
	auto &mapRef = *map;

	// Fill up map of helpers
	// It would be appropriate to use a std::map here, alas, it doesn't work when there's
	// no default constructor available, so I have to use this kludge
	std::vector<BattleUnitTileHelper> helperMap = {BattleUnitTileHelper(mapRef, (BattleUnitType)0),
	                                               BattleUnitTileHelper(mapRef, (BattleUnitType)1),
	                                               BattleUnitTileHelper(mapRef, (BattleUnitType)2),
	                                               BattleUnitTileHelper(mapRef, (BattleUnitType)3)};

	// First update all center positions
	for (int i = 0; i < lbCount; i++)
	{
		if (!blockNeedsUpdate[i])
		{
			continue;
		}
		blockNeedsUpdate[i] = false;

		// Mark all paths including this block as needing an update
		for (int j = 0; j < lbCount; j++)
		{
			if (linkAvailable[i + lbCount * j])
			{
				linkNeedsUpdate[std::min(i, j) + std::max(i, j) * lbCount] = true;
			}
		}

		// Find closest to center valid position for every kind of unit
		auto &lb = *losBlocks[i];
		auto center = (lb.start + lb.end) / 2;
		for (auto &type : BattleUnitTypeList)
		{
			blockAvailable[type][i] =
			    findLosBlockCenter(mapRef, type, lb, center, blockCenterPos[type][i]);
		}
	}

	// Now update all paths
	for (int i = 0; i < lbCount - 1; i++)
	{
		for (int j = i + 1; j < lbCount; j++)
		{
			if (!linkNeedsUpdate[i + j * lbCount])
			{
				continue;
			}
			linkNeedsUpdate[i + j * lbCount] = false;

			// Update link for every kind of unit
			for (auto &type : BattleUnitTypeList)
			{
				// Do not try if one of blocks is unavailable
				if (!blockAvailable[type][i] || !blockAvailable[type][j])
				{
					linkCost[type][i + j * lbCount] = -1;
					linkCost[type][j + i * lbCount] = -1;
					continue;
				}

				// See if path from one center to another center is possible
				// within reasonable number of attempts
				int dX = std::abs(blockCenterPos[type][i].x - blockCenterPos[type][j].x);
				int dY = std::abs(blockCenterPos[type][i].y - blockCenterPos[type][j].y);
				int dZ = std::abs(blockCenterPos[type][i].z - blockCenterPos[type][j].z);
				int distance = (dX + dY + dZ + std::max(dX, std::max(dY, dZ))) / 2;

				float cost = 0.0f;

				auto path = mapRef.findShortestPath(
				    blockCenterPos[type][i], blockCenterPos[type][j],
				    distance * PATH_ITERATION_LIMIT_MULTIPLIER, helperMap[(int)type], false, true,
				    true, &cost, distance * 4 * PATH_COST_LIMIT_MULTIPLIER);

				if (path.empty() || (*path.rbegin()) != blockCenterPos[type][j])
				{
					linkCost[type][i + j * lbCount] = -1;
					linkCost[type][j + i * lbCount] = -1;
				}
				else
				{
					linkCost[type][i + j * lbCount] = (int)cost;
					linkCost[type][j + i * lbCount] = (int)cost;
				}
			}
		}
	}

	// FIXME: Somehow introduce multi-threading here or throttle the load
	// Calculating paths is the more costly operation here. If a big chunk of map is changed,
	// it can take up to half a second to calculate. One option would be to calculate it
	// in a different thread (maybe writing results to a different array, and then just swapping)
	// Another option would be to throttle updates (have a limit on how many can be done per tick)
}

void Battle::update(GameState &state, unsigned int ticks)
{
	TRACE_FN_ARGS1("ticks", Strings::fromInteger(static_cast<int>(ticks)));

	if (missionEndTimer > 0)
	{
		missionEndTimer++;
		ticksWithoutAction = 0;
		for (auto &p : participants)
		{
			ticksWithoutSeenAction[p] = 0;
		}
	}
	Trace::start("Battle::update::turnBased");
	if (mode == Mode::TurnBased)
	{
		ticksWithoutAction += ticks;
		for (auto &p : participants)
		{
			ticksWithoutSeenAction[p]++;
		}
		// Interrupt for lowmorales
		if (!lowMoraleProcessed && interruptQueue.empty() && interruptUnits.empty())
		{
			lowMoraleProcessed = true;
			for (auto &u : units)
			{
				if (u.second->owner != currentActiveOrganisation || !u.second->isConscious() ||
				    u.second->moraleState == MoraleState::Normal ||
				    u.second->agent->modified_stats.time_units == 0)
				{
					continue;
				}
				lowMoraleProcessed = false;
				ticksWithoutAction = TICKS_BEGIN_INTERRUPT;
				interruptQueue.emplace(StateRef<BattleUnit>(&state, u.first), 0);
				if (u.second->owner == currentPlayer ||
				    visibleUnits[currentPlayer].find({&state, u.first}) !=
				        visibleUnits[currentPlayer].end())
				{
					fw().pushEvent(
					    new GameLocationEvent(GameEventType::ZoomView, u.second->position));
				}
				break;
			}
		}
		// Add units from queue to interrupt list
		if (!interruptQueue.empty() && ticksWithoutAction >= TICKS_BEGIN_INTERRUPT)
		{
			for (auto &e : interruptQueue)
			{
				interruptUnits.emplace(e.first, e.second);
			}
			interruptQueue.clear();
			notifyAction();
			turnEndAllowed = false;
		}
		// Turn end condition
		if (ticksWithoutAction >= TICKS_END_TURN)
		{
			if (interruptUnits.empty())
			{
				if (turnEndAllowed)
				{
					endTurn(state);
				}
			}
			else
			{
				// Spend remaining TUs of low morale units
				for (auto &e : interruptUnits)
				{
					if (e.first->moraleState != MoraleState::Normal)
					{
						e.first.getSp()->spendRemainingTU(state);
						e.first.getSp()->focusUnit.clear();
					}
				}
				interruptUnits.clear();
				notifyAction();
				turnEndAllowed = false;
			}
		}
	}
	Trace::end("Battle::end::turnBased");
	Trace::start("Battle::update::projectiles->update");
	updateProjectiles(state, ticks);
	Trace::end("Battle::update::projectiles->update");
	Trace::start("Battle::update::doors->update");
	for (auto &o : this->doors)
	{
		o.second->update(state, ticks);
	}
	Trace::end("Battle::update::doors->update");
	Trace::start("Battle::update::doodads->update");
	for (auto it = this->doodads.begin(); it != this->doodads.end();)
	{
		auto d = *it++;
		d->update(state, ticks);
	}
	Trace::end("Battle::update::doodads->update");
	Trace::start("Battle::update::hazards->update");
	for (auto it = this->hazards.begin(); it != this->hazards.end();)
	{
		auto d = *it++;
		d->update(state, ticks);
	}
	Trace::end("Battle::update::hazards->update");
	Trace::start("Battle::update::explosions->update");
	for (auto it = this->explosions.begin(); it != this->explosions.end();)
	{
		auto d = *it++;
		d->update(state, ticks);
	}
	Trace::end("Battle::update::explosions->update");
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
	Trace::start("Battle::update::scanners->update");
	for (auto &o : this->scanners)
	{
		o.second->update(state, ticks);
	}
	Trace::end("Battle::update::scanners->update");
	Trace::start("Battle::update::units->update");
	for (auto &o : this->units)
	{
		o.second->update(state, ticks);
	}
	Trace::end("Battle::update::units->update");
	Trace::start("Battle::update::ai->think");
	{
		auto result = aiBlock.think(state);
		for (auto &entry : result)
		{
			BattleUnit::executeGroupAIDecision(state, entry.second, entry.first);
		}
	}
	Trace::end("Battle::update::ai->think");

	// Now after we called update() for everything, we update what needs to be updated last

	// Update unit vision for units that see changes in terrain or hazards
	Trace::start("Battle::update::vision");
	updateVision(state);
	Trace::end("Battle::update::vision");
	Trace::start("Battle::update::pathfinding");
	updatePathfinding(state);
	Trace::end("Battle::update::pathfinding");
}

void Battle::updateTBBegin(GameState &state)
{
	notifyAction();
	turnEndAllowed = false;
	lowMoraleProcessed = false;

	Trace::start("Battle::updateTBBegin::units->update");
	for (auto &o : this->units)
	{
		if (o.second->owner == currentActiveOrganisation)
		{
			o.second->updateTB(state);
		}
	}
	Trace::end("Battle::updateTBBegin::units->update");
}

void Battle::updateTBEnd(GameState &state)
{
	Trace::start("Battle::updateTBEnd::hazards->update");
	for (auto it = this->hazards.begin(); it != this->hazards.end();)
	{
		auto d = *it++;
		if (d->ownerOrganisation == currentActiveOrganisation)
		{
			d->updateTB(state);
		}
	}
	Trace::end("Battle::updateTBEnd::hazards->update");
	Trace::start("Battle::updateTBEnd::items->update");
	for (auto it = this->items.begin(); it != this->items.end();)
	{
		auto p = *it++;
		if (p->owner == currentActiveOrganisation)
		{
			p->updateTB(state);
		}
	}
	Trace::end("Battle::updateTBEnd::items->update");
}

int Battle::getLosBlockID(int x, int y, int z) const
{
	return tileToLosBlock.at(z * size.x * size.y + y * size.x + x);
}

bool Battle::getVisible(StateRef<Organisation> org, int x, int y, int z) const
{
	return visibleTiles.at(org).at(z * size.x * size.y + y * size.x + x);
}

void Battle::setVisible(StateRef<Organisation> org, int x, int y, int z, bool val)
{
	visibleTiles[org][z * size.x * size.y + y * size.x + x] = val;
}

void Battle::queueVisionRefresh(Vec3<int> tile) { tilesChangedForVision.insert(tile); }

void Battle::notifyScanners(Vec3<int> position)
{
	for (auto &s : scanners)
	{
		s.second->notifyMovement(position);
	}
}

void Battle::notifyAction(Vec3<int> location, StateRef<BattleUnit> actorUnit)
{
	ticksWithoutAction = 0;
	if (location == EventMessage::NO_LOCATION)
	{
		return;
	}
	for (auto &p : participants)
	{
		if (actorUnit && actorUnit->owner != p &&
		    visibleUnits[p].find(actorUnit) == visibleUnits[p].end())
		{
			continue;
		}
		ticksWithoutSeenAction[p] = 0;
		lastSeenActionLocation[p] = location;
	}
}

int Battle::killStrandedUnits(GameState &state, bool preview)
{
	LogWarning("Implement killing stranded player units");
	return 0;
}

void Battle::abortMission(GameState &state)
{
	killStrandedUnits(state);
	auto player = state.getPlayer();
	for (auto &u : units)
	{
		if (u.second->owner == u.second->agent->owner && u.second->owner == player &&
		    !u.second->isDead() && !u.second->retreated)
		{
			u.second->retreat(state);
		}
	}
}

void Battle::checkMissionEnd(GameState &state, bool retreated, bool forceReCheck)
{
	auto endBeginTimer = std::max((unsigned)1, missionEndTimer);
	if (forceReCheck)
	{
		missionEndTimer = 0;
	}
	else if (missionEndTimer > 0)
	{
		return;
	}
	loserHasRetreated = retreated;
	auto civ = state.getCivilian();
	auto player = state.getPlayer();
	std::set<StateRef<Organisation>> orgsAlive;
	for (auto &p : participants)
	{
		if (p == civ)
		{
			continue;
		}
		for (auto &u : units)
		{
			if (u.second->owner == p && u.second->isConscious())
			{
				orgsAlive.insert(p);
				break;
			}
		}
	}
	if (orgsAlive.find(player) == orgsAlive.end())
	{
		playerWon = false;
		missionEndTimer = endBeginTimer;
	}
	else
	{
		playerWon = true;
		missionEndTimer = endBeginTimer;
		for (auto &org : orgsAlive)
		{
			if (org == player)
			{
				continue;
			}
			if (player->isRelatedTo(org) == Organisation::Relation::Hostile)
			{
				missionEndTimer = 0;
				break;
			}
		}
	}
}

void Battle::checkIfBuildingDisabled(GameState &state)
{
	if (!buildingCanBeDisabled || buildingDisabled)
	{
		return;
	}
	// Find a mission objective unit
	for (auto &u : units)
	{
		if (u.second->owner != targetOrganisation)
		{
			continue;
		}
		if (u.second->agent->type->missionObjective && !u.second->isDead())
		{
			// Mission objective unit found alive
			return;
		}
	}
	// Find a mission objective object
	for (auto &mp : map_parts)
	{
		if (mp->type->missionObjective && !mp->destroyed)
		{
			// Mission objective unit found alive
			return;
		}
	}
	// Found nothing, building disabled
	buildingDisabled = true;
	fw().pushEvent(new GameEvent(GameEventType::BuildingDisabled));
}

void Battle::refreshLeadershipBonus(StateRef<Organisation> org)
{
	Rank highestRank = Rank::Rookie;
	for (auto &u : units)
	{
		if (u.second->owner != org || u.second->isDead() || u.second->retreated)
		{
			continue;
		}
		if ((int)u.second->agent->rank > (int)highestRank)
		{
			highestRank = u.second->agent->rank;
		}
	}
	switch (highestRank)
	{
		case Rank::Rookie:
		case Rank::Squaddie:
			leadershipBonus[org] = 0;
			break;
		case Rank::SquadLeader:
			leadershipBonus[org] = 5;
			break;
		case Rank::Sergeant:
			leadershipBonus[org] = 10;
			break;
		case Rank::Captain:
			leadershipBonus[org] = 15;
			break;
		case Rank::Colonel:
			leadershipBonus[org] = 25;
			break;
		case Rank::Commander:
			leadershipBonus[org] = 50;
			break;
	}
}

void Battle::queuePathfindingRefresh(Vec3<int> tile)
{
	blockNeedsUpdate[getLosBlockID(tile.x, tile.y, tile.z)] = true;
	auto tXgt0 = tile.x > 0;
	auto tYgt0 = tile.y > 0;
	auto tZgt0 = tile.z > 0;
	if (tXgt0)
	{
		blockNeedsUpdate[getLosBlockID(tile.x - 1, tile.y, tile.z)] = true;
		if (tYgt0)
		{
			blockNeedsUpdate[getLosBlockID(tile.x - 1, tile.y - 1, tile.z)] = true;
			if (tZgt0)
			{
				blockNeedsUpdate[getLosBlockID(tile.x - 1, tile.y - 1, tile.z - 1)] = true;
			}
		}
		if (tZgt0)
		{
			blockNeedsUpdate[getLosBlockID(tile.x - 1, tile.y, tile.z - 1)] = true;
		}
	}
	if (tYgt0)
	{
		blockNeedsUpdate[getLosBlockID(tile.x, tile.y - 1, tile.z)] = true;
		if (tZgt0)
		{
			blockNeedsUpdate[getLosBlockID(tile.x, tile.y - 1, tile.z - 1)] = true;
		}
	}
	if (tZgt0)
	{
		blockNeedsUpdate[getLosBlockID(tile.x, tile.y, tile.z - 1)] = true;
	}
}

void Battle::accuracyAlgorithmBattle(GameState &state, Vec3<float> firePosition,
                                     Vec3<float> &target, int accuracy, bool cloaked, bool thrown)
{
	auto dispersion = (float)(100 - accuracy);
	if (cloaked)
	{
		dispersion *= dispersion;
		float cloakDispersion =
		    2000.0f / (BattleUnitTileHelper::getDistanceStatic(firePosition, target) / 4.0f + 3.0f);
		dispersion += cloakDispersion * cloakDispersion;
		dispersion = sqrtf(dispersion);
	}

	auto delta = (target - firePosition) * dispersion / 1000.0f;

	float length_vector =
	    1.0f / std::sqrt(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z);

	std::vector<float> rnd(3);
	while (true)
	{
		rnd[1] = (float)randBoundsExclusive(state.rng, 0, 100000) / 100000.0f;
		rnd[2] = (float)randBoundsExclusive(state.rng, 0, 100000) / 100000.0f;
		rnd[0] = rnd[1] * rnd[1] + rnd[2] * rnd[2];
		if (rnd[0] > 0.0f && rnd[0] < 1.0f)
		{
			break;
		}
	}

	float k1 = (2 * randBoundsInclusive(state.rng, 0, 1) - 1) * rnd[1] *
	           std::sqrt(-2 * std::log(rnd[0]) / rnd[0]);
	float k2 = (2 * randBoundsInclusive(state.rng, 0, 1) - 1) * rnd[2] *
	           std::sqrt(-2 * std::log(rnd[0]) / rnd[0]);

	// Vertical misses only go down
	auto diffVertical =
	    Vec3<float>{length_vector * delta.x * delta.z, length_vector * delta.y * delta.z,
	                std::min(0.0f, -length_vector * (delta.x * delta.x + delta.y * delta.y))} *
	    k1;
	auto diffHorizontal = Vec3<float>{-delta.y, delta.x, 0.0f} * k2;
	auto diff = (diffVertical + diffHorizontal) *
	            (thrown ? Vec3<float>{3.0f, 3.0f, 0.0f} : Vec3<float>{1.0f, 1.0f, 0.33f});
	target += diff;
}

void Battle::beginTurn(GameState &state)
{
	if (mode != Mode::TurnBased)
	{
		LogError("beginTurn called in real time?");
		return;
	}

	// Cancel mind control
	for (auto &u : units)
	{
		if (u.second->owner != currentActiveOrganisation)
		{
			continue;
		}
		u.second->stopAttackPsi(state);
	}

	// Update everything related to this turn
	updateTBBegin(state);

	// Unit's begin turn routine
	for (auto &u : units)
	{
		if (u.second->owner != currentActiveOrganisation)
		{
			continue;
		}
		u.second->beginTurn(state);
	}

	aiBlock.beginTurnRoutine(state, currentActiveOrganisation);

	for (auto &p : participants)
	{
		ticksWithoutSeenAction[p] = TICKS_PER_TURN;
	}

	fw().pushEvent(new GameBattleEvent(GameEventType::NewTurn, shared_from_this()));
}

void Battle::endTurn(GameState &state)
{
	updateTBEnd(state);

	// Pass turn to next org, if final org - increment turn counter and pass to first org
	auto it = ++std::find(participants.begin(), participants.end(), currentActiveOrganisation);
	if (it == participants.end())
	{
		currentTurn++;
		it = participants.begin();
	}
	currentActiveOrganisation = *it;

	beginTurn(state);
}

void Battle::giveInterruptChanceToUnits(GameState &state, StateRef<BattleUnit> giver,
                                        int reactionValue)
{
	if (mode != Mode::TurnBased)
	{
		return;
	}
	for (auto &u : units)
	{
		if (u.second->visibleEnemies.find(giver) != u.second->visibleEnemies.end())
		{
			giveInterruptChanceToUnit(state, giver, {&state, u.second->id}, reactionValue);
		}
	}
}

void Battle::giveInterruptChanceToUnit(GameState &state, StateRef<BattleUnit> giver,
                                       StateRef<BattleUnit> receiver, int reactionValue)
{
	if (mode != Mode::TurnBased || receiver->owner == currentActiveOrganisation ||
	    receiver->getAIType() == AIType::None ||
	    interruptQueue.find(receiver) != interruptQueue.end())
	{
		return;
	}

	if (receiver->agent->getReactionValue() > reactionValue)
	{
		receiver->focusUnit = giver;
		auto decision = receiver->aiList.think(state, *receiver, true);
		if (decision.isEmpty())
		{
			receiver->focusUnit.clear();
		}
		else
		{
			LogWarning("Interrupting AI %s for unit %s decided to %s", decision.ai, receiver->id,
			           decision.getName());
			receiver->aiList.reset(state, *receiver);
			if (interruptQueue.empty())
			{
				fw().pushEvent(new GameLocationEvent(GameEventType::ZoomView, receiver->position));
			}
			interruptQueue.emplace(receiver, receiver->agent->getTULimit(reactionValue));
			receiver->experiencePoints.reactions++;
		}
	}
}

// To be called when battle must be started, before showing battle briefing screen
void Battle::beginBattle(GameState &state, bool hotseat, StateRef<Organisation> target_organisation,
                         std::list<StateRef<Agent>> &player_agents,
                         const std::map<StateRef<AgentType>, int> *aliens,
                         StateRef<Vehicle> player_craft, StateRef<Vehicle> target_craft)
{
	if (state.current_battle)
	{
		LogError("Battle::beginBattle called while another battle is in progress!");
		return;
	}
	auto b = BattleMap::createBattle(state, target_organisation, player_agents, aliens,
	                                 player_craft, target_craft);
	if (!b)
	{
		return;
	}
	b->targetOrganisation = target_organisation;
	b->hotseat = hotseat;
	state.current_battle = b;
}

// To be called when battle must be started, before showing battle briefing screen
void Battle::beginBattle(GameState &state, bool hotseat, StateRef<Organisation> target_organisation,
                         std::list<StateRef<Agent>> &player_agents,
                         const std::map<StateRef<AgentType>, int> *aliens, const int *guards,
                         const int *civilians, StateRef<Vehicle> player_craft,
                         StateRef<Building> target_building)
{
	if (state.current_battle)
	{
		LogError("Battle::beginBattle called while another battle is in progress!");
		return;
	}
	auto b = BattleMap::createBattle(state, target_organisation, player_agents, aliens, guards,
	                                 civilians, player_craft, target_building);
	if (!b)
	{
		return;
	}
	b->hotseat = hotseat;
	b->targetOrganisation = target_organisation;
	b->buildingCanBeDisabled = target_organisation == state.getAliens();
	state.current_battle = b;
}

// To be called when battle must be started, after the player has assigned agents to squads
void Battle::enterBattle(GameState &state)
{
	if (!state.current_battle)
	{
		LogError("Battle::enterBattle called with no battle!");
		return;
	}

	auto &b = state.current_battle;
	b->hotseat = b->hotseat && b->mode == Battle::Mode::TurnBased;

	state.current_battle->initialUnitSpawn(state);

	state.current_battle->initBattle(state, true);

	// Find first player unit
	sp<BattleUnit> firstPlayerUnit = nullptr;
	for (auto &f : state.current_battle->forces[state.getPlayer()].squads)
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
		state.current_battle->battleViewScreenCenter = state.current_battle->map->size / 2;
		state.current_battle->battleViewZLevel = state.current_battle->map->size.z / 2 + 1;
	}
	else
	{
		state.current_battle->battleViewScreenCenter = firstPlayerUnit->position;
		state.current_battle->battleViewZLevel = (int)ceilf(firstPlayerUnit->position.z);
	}
	state.current_battle->battleViewGroupMove = true;
}

// To be called when battle must be finished and before showing score screen
void Battle::finishBattle(GameState &state)
{
	if (!state.current_battle)
	{
		LogError("Battle::FinishBattle called with no battle!");
		return;
	}

	auto player = state.getPlayer();
	state.current_battle->unloadResources(state);

	// Remove active battle scanners
	for (auto &u : state.current_battle->units)
	{
		for (auto &e : u.second->agent->equipment)
		{
			e->battleScanner.clear();
		}
	}
	// Proces MCed units
	for (auto &u : state.current_battle->units)
	{
		if (u.second->owner != u.second->agent->owner)
		{
			if (u.second->owner == player)
			{
				// mind control by player = capped alive
				u.second->stunDamage = 9001;
			}
			else
			{
				// mind control by someone else = MIA
				u.second->agent->modified_stats.health = -1;
				if (u.second->agent->owner == player)
				{
					state.current_battle->score.casualtyPenalty -= u.second->agent->type->score;
				}
			}
			u.second->owner = u.second->agent->owner;
		}
	}
	//	- Prepare list of surviving aliens
	//	- (Success) Convert remaining unconscious and dead aliens into research items
	//  - Remove brainsucker pods from agent inventories and convert into research items
	//  - Calculate score for live captured aliens
	//  - (Failure) Handle remaining aliens (retreat them if ufo, put them in the building if
	//  building)
	//  - Handle retreated aliens (put them to random closest building)
	// Remove dead player agents and all enemy agents from the game and from vehicles
	std::list<sp<BattleUnit>> unitsToRemove;
	for (auto &u : state.current_battle->units)
	{
		if (u.second->owner != player || u.second->isDead())
		{
			unitsToRemove.push_back(u.second);
		}
	}
	for (auto &u : unitsToRemove)
	{
		u->agent->destroy();
		state.agents.erase(u->agent.id);
		u->destroy();
		state.current_battle->units.erase(u->id);
	}
	// Apply experience to stats of living agents + promotions
	// Create list of units ranked by combatRating
	std::map<int, std::list<sp<BattleUnit>>> unitsByRating;
	for (auto &u : state.current_battle->units)
	{
		u.second->processExperience(state);
		unitsByRating[-u.second->combatRating].push_back(u.second);
	}
	// Create count of ranks
	std::map<Rank, int> countRanks;
	for (auto &a : state.agents)
	{
		countRanks[a.second->rank]++;
	}
	// Rank up top 5 units from list that can accept promotion
	for (auto &l : unitsByRating)
	{
		for (auto &u : l.second)
		{
			switch (u->agent->rank)
			{
				case Rank::Rookie:
					if (u->combatRating < 9)
					{
						continue;
					}
					break;
				case Rank::Squaddie:
					if ((countRanks[Rank::Rookie] + countRanks[Rank::Squaddie]) /
					        (countRanks[Rank::SquadLeader] + 1) <=
					    4)
					{
						continue;
					}
					break;
				case Rank::SquadLeader:
					if (countRanks[Rank::SquadLeader] / (countRanks[Rank::Sergeant] + 1) <= 2)
					{
						continue;
					}
					break;
				case Rank::Sergeant:
					if (countRanks[Rank::Sergeant] / (countRanks[Rank::Captain] + 1) <= 2)
					{
						continue;
					}
					break;
				case Rank::Captain:
					if (countRanks[Rank::Captain] / (countRanks[Rank::Colonel] + 1) <= 2)
					{
						continue;
					}
					break;
				case Rank::Colonel:
					if (countRanks[Rank::Commander] > 0 || countRanks[Rank::Colonel] <= 1)
					{
						continue;
					}
					break;
				case Rank::Commander:
					continue;
			}
			u->agent->rank = (Rank)(((int)u->agent->rank) + 1);
			state.current_battle->unitsPromoted.push_back({&state, u->id});
			if (state.current_battle->unitsPromoted.size() >= 5)
			{
				break;
			}
		}
		if (state.current_battle->unitsPromoted.size() >= 5)
		{
			break;
		}
	}
	//  - (Failure) Kill all items on the battlefield
	//	- (Success) Prepare list of loot (including alien saucer equipment), give score for it
	//  - Move unresearched items from agent inventory into loot list
	//  - Convert living aliens to bodies if no bio trans
	//	- Calculate score for captured loot
	//  - Calculate score for captured live aliens
}

// To be called after battle was finished, score screen was shown and before returning to cityscape
void Battle::exitBattle(GameState &state)
{
	if (!state.current_battle)
	{
		LogError("Battle::ExitBattle called with no battle!");
		return;
	}

	//  - Apply score to player score
	// (UFO mission) Remove UFO crash
	if (state.current_battle->mission_type == Battle::MissionType::UfoRecovery)
	{
		StateRef<Vehicle> ufo = {&state, state.current_battle->mission_location_id};
		if (state.current_battle->playerWon)
		{
			LogWarning("FIXME: Give player score for captured ufo");
		}
		state.vehicles.erase(ufo.id);
	}
	//  - (If mod then give player choice of what to load and what to leave behind)
	//	- Load loot into vehicles
	//	- Load aliens into bio - trans
	// Restore X-Com relationship to organisations
	auto player = state.getPlayer();
	for (auto &o : state.organisations)
	{
		if (o.second == player.getSp())
		{
			continue;
		}
		player->current_relations[{&state, o.first}] = o.second->getRelationTo(player);
	}

	LogWarning(
	    "Checking item reference consistency, remove code in battle.exitBattleconfirmed correct");
	for (auto &a : state.agents)
	{
		for (auto &e : a.second->equipment)
		{
			if (e->ownerUnit)
			{
				LogError("Agent %s has item %s which is assigned to unit %s and will leak", a.first,
				         e->type.id, e->ownerUnit.id);
			}
		}
	}

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
	UString brainsucker = "bsk";
	bool brainsuckerFound = false;
	UString hyperworm = "hypr";
	UString multiworm = "multi";
	bool hyperwormFound = false;
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
				if (packName == hyperworm)
				{
					hyperwormFound = true;
				}
				if (!hyperwormFound && packName == multiworm)
				{
					imagePacks.insert(hyperworm);
					imagePacks.insert(hyperworm + "s");
					hyperwormFound = true;
				}
				if (packName == brainsucker)
				{
					brainsuckerFound = true;
				}
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
			if (!brainsuckerFound && ae->getPayloadType()->damage_type &&
			    ae->getPayloadType()->damage_type->effectType ==
			        DamageType::EffectType::Brainsucker)
			{
				imagePacks.insert(brainsucker);
				imagePacks.insert(brainsucker + "s");
				brainsuckerFound = true;
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
		auto imagePackPath = BattleUnitImagePack::getImagePackPath() + "/" + imagePackName;
		LogInfo("Loading image pack \"%s\" from \"%s\"", imagePackName, imagePackPath);
		auto imagePack = mksp<BattleUnitImagePack>();
		if (!imagePack->loadImagePack(state, imagePackPath))
		{
			LogError("Failed to load image pack \"%s\" from \"%s\"", imagePackName, imagePackPath);
			continue;
		}
		state.battle_unit_image_packs[format("%s%s", BattleUnitImagePack::getPrefix(),
		                                     imagePackName)] = imagePack;
		LogInfo("Loaded image pack \"%s\" from \"%s\"", imagePackName, imagePackPath);
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
	UString brainsucker = "bsk";
	bool brainsuckerFound = false;
	UString hyperworm = "hypr";
	UString multiworm = "multi";
	bool hyperwormFound = false;
	for (auto &u : units)
	{
		for (auto &ap : u.second->agent->type->animation_packs)
		{
			auto packName = BattleUnitAnimationPack::getNameFromID(ap.id);
			if (animationPacks.find(packName) == animationPacks.end())
			{
				animationPacks.insert(packName);
			}
			if (packName == hyperworm)
			{
				hyperwormFound = true;
			}
			if (!hyperwormFound && packName == multiworm)
			{
				animationPacks.insert(hyperworm);
				hyperwormFound = true;
			}
			if (packName == brainsucker)
			{
				brainsuckerFound = true;
			}
		}
		if (!brainsuckerFound)
		{
			for (auto &e : u.second->agent->equipment)
			{
				if (e->getPayloadType()->damage_type &&
				    e->getPayloadType()->damage_type->effectType ==
				        DamageType::EffectType::Brainsucker)
				{
					animationPacks.insert(brainsucker);
					brainsuckerFound = true;
					break;
				}
			}
		}
	}
	// Load all used animation packs
	for (auto &animationPackName : animationPacks)
	{
		auto animationPackPath =
		    BattleUnitAnimationPack::getAnimationPackPath() + "/" + animationPackName;
		LogInfo("Loading animation pack \"%s\" from \"%s\"", animationPackName, animationPackPath);
		auto animationPack = mksp<BattleUnitAnimationPack>();
		if (!animationPack->loadAnimationPack(state, animationPackPath))
		{
			LogError("Failed to load animation pack \"%s\" from \"%s\"", animationPackName,
			         animationPackPath);
			continue;
		}
		state.battle_unit_animation_packs[format("%s%s", BattleUnitAnimationPack::getPrefix(),
		                                         animationPackName)] = animationPack;
		LogInfo("Loaded animation pack \"%s\" from \"%s\"", animationPackName, animationPackPath);
	}
}

void Battle::unloadAnimationPacks(GameState &state)
{
	state.battle_unit_animation_packs.clear();
	LogInfo("Unloaded all animation packs.");
}

int BattleScore::getLeadershipBonus()
{
	return (100 + 3 * casualtyPenalty) * (combatRating + friendlyFire + liveAlienCaptured) / 100;
}

int BattleScore::getTotal()
{
	return combatRating + casualtyPenalty + getLeadershipBonus() + liveAlienCaptured +
	       equipmentCaptured + equipmentLost;
}

UString BattleScore::getText()
{
	auto total = getTotal();
	if (total > 500)
	{
		return tr("Very Good");
	}
	else if (total > 200)
	{
		return tr("Good");
	}
	else if (total > 0)
	{
		return tr("OK");
	}
	else if (total > -200)
	{
		return tr("Poor");
	}
	else
	{
		return tr("Very Poor");
	}
}

} // namespace OpenApoc

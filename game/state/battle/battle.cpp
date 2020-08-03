#include "game/state/battle/battle.h"
#include "framework/configfile.h"
#include "framework/framework.h"
#include "framework/sound.h"
#include "game/state/battle/ai/aitype.h"
#include "game/state/battle/battledoor.h"
#include "game/state/battle/battleexplosion.h"
#include "game/state/battle/battlehazard.h"
#include "game/state/battle/battleitem.h"
#include "game/state/battle/battlemappart.h"
#include "game/state/battle/battlescanner.h"
#include "game/state/battle/battleunit.h"
#include "game/state/city/agentmission.h"
#include "game/state/city/base.h"
#include "game/state/city/building.h"
#include "game/state/city/city.h"
#include "game/state/city/vehicle.h"
#include "game/state/city/vehiclemission.h"
#include "game/state/gameevent.h"
#include "game/state/gamestate.h"
#include "game/state/message.h"
#include "game/state/rules/aequipmenttype.h"
#include "game/state/rules/battle/battlecommonimagelist.h"
#include "game/state/rules/battle/battlecommonsamplelist.h"
#include "game/state/rules/battle/battlemap.h"
#include "game/state/rules/battle/battlemapparttype.h"
#include "game/state/rules/battle/battleunitanimationpack.h"
#include "game/state/rules/battle/battleunitimagepack.h"
#include "game/state/rules/battle/damage.h"
#include "game/state/rules/doodadtype.h"
#include "game/state/shared/aequipment.h"
#include "game/state/shared/doodad.h"
#include "game/state/shared/projectile.h"
#include "game/state/tilemap/collision.h"
#include "game/state/tilemap/tilemap.h"
#include "game/state/tilemap/tileobject_battlehazard.h"
#include "game/state/tilemap/tileobject_battleitem.h"
#include "game/state/tilemap/tileobject_battlemappart.h"
#include "game/state/tilemap/tileobject_battleunit.h"
#include "game/state/tilemap/tileobject_doodad.h"
#include "game/state/tilemap/tileobject_projectile.h"
#include "game/state/tilemap/tileobject_shadow.h"
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
	// Note due to backrefs to Tile*s etc. we need to destroy all tile objects
	// before the TileMap
	if (map)
	{
		map->ceaseUpdates = true;
	}
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
	if (forces.empty())
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

	// On first run, init support links and items, do visibility and pathfinding, reset AI
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
	updatePathfinding(state, 0);
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

	// No checks for base defense
	if (mission_type == MissionType::BaseDefense)
	{
		return true;
	}

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
	for (auto &a : agents)
	{
		if (a->owner != player && a->owner != civilian)
		{
			spawnPointsRequired += a->type->bodyType->large ? 4 : 1;
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
	std::ignore = state;
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
			LogWarning("MP %s SBT %d at %s is UNLINKED", mp->type.id,
			           (int)mp->type->getVanillaSupportedById(), pos);
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
			LogWarning("MP %s SBT %d at %s is going to fall", mp->type.id,
			           (int)mp->type->getVanillaSupportedById(), pos);
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

	// Spawn maps for specific units of specific orgs
	std::map<SpawnKey, std::list<sp<SpawnBlock>>> spawnMaps;
	// Spawn maps for units of different type (spawn aliens in civ spots etc.)
	std::map<SpawnKey, std::list<sp<SpawnBlock>>> spawnInverse;
	// Other blocks that have 0 spawn priority, to be used when all others are exhausted
	std::list<sp<SpawnBlock>> spawnOther;
	// Spawn types belonging to organisations
	// (X-Com -> Player, Security/Alien -> Enemy, Civ -> Civ)
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
	for (size_t i = 0; i < losBlocks.size(); i++)
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
		for (int j = 0; j < lb->spawn_priority; j++)
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

			if (lb->also_allow_civilians)
			{
				spawnMaps[{SpawnType::Civilian,
				           lb->spawn_large_units ? UnitSize::Large : UnitSize::Small,
				           lb->spawn_walking_units ? UnitMovement::Walking : UnitMovement::Flying,
				           lb->low_priority}]
				    .push_back(sb);
				spawnMaps[{SpawnType::Civilian,
				           lb->spawn_large_units ? UnitSize::Large : UnitSize::Small,
				           UnitMovement::Any, lb->low_priority}]
				    .push_back(sb);
				spawnMaps[{SpawnType::Civilian, UnitSize::Any,
				           lb->spawn_walking_units ? UnitMovement::Walking : UnitMovement::Flying,
				           lb->low_priority}]
				    .push_back(sb);
				spawnMaps[{SpawnType::Civilian, UnitSize::Any, UnitMovement::Any, lb->low_priority}]
				    .push_back(sb);
			}

			for (int k = 0; k < 3; k++)
			{
				auto st = (SpawnType)k;
				if (st == lb->spawn_type || (st == SpawnType::Civilian && lb->also_allow_civilians))
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
				spawnInverse[{st, UnitSize::Any,
				              lb->spawn_walking_units ? UnitMovement::Walking
				                                      : UnitMovement::Flying,
				              lb->low_priority}]
				    .push_back(sb);
				spawnInverse[{st, UnitSize::Any, UnitMovement::Any, lb->low_priority}].push_back(
				    sb);
			}
		}
	}

	// Spawn agents with spawn locations provided
	for (auto &u : units)
	{
		if (u.second->retreated || spawnLocations[u.second->agent->type].empty())
		{
			continue;
		}
		auto pos = pickRandom(state.rng, spawnLocations[u.second->agent->type]);
		spawnLocations[u.second->agent->type].remove(pos);
		auto tile = map->getTile(pos.x, pos.y, pos.z);
		u.second->position = tile->getRestingPosition(u.second->isLarge());
	}

	// Actually spawn agents
	for (auto &f : state.current_battle->forces)
	{
		// Note if we need to spawn non-combatants at civilian spots
		bool baseDefendingSide =
		    state.current_battle->mission_type == Battle::MissionType::BaseDefense &&
		    f.first == state.current_battle->currentPlayer;

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
				bool requestCivilian = false;
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
					if (baseDefendingSide && !u->agent->type->inventory)
					{
						requestCivilian = true;
					}
				}

				// Make a list of priorities, in which order will we try to find a block
				sp<SpawnBlock> block = nullptr;
				std::list<std::list<sp<SpawnBlock>> *> priorityList;
				auto spawnType = requestCivilian ? SpawnType::Civilian : spawnTypeMap[{f.first}];
				if (needWalker && needLarge)
				{
					priorityList.push_back(
					    &spawnMaps[{spawnType, UnitSize::Large, UnitMovement::Walking, false}]);
					priorityList.push_back(
					    &spawnMaps[{spawnType, UnitSize::Large, UnitMovement::Walking, true}]);
					priorityList.push_back(
					    &spawnInverse[{spawnType, UnitSize::Large, UnitMovement::Walking, false}]);
					priorityList.push_back(
					    &spawnInverse[{spawnType, UnitSize::Large, UnitMovement::Walking, true}]);

					priorityList.push_back(
					    &spawnMaps[{spawnType, UnitSize::Large, UnitMovement::Flying, false}]);
					priorityList.push_back(
					    &spawnMaps[{spawnType, UnitSize::Large, UnitMovement::Flying, true}]);
					priorityList.push_back(
					    &spawnInverse[{spawnType, UnitSize::Large, UnitMovement::Flying, false}]);
					priorityList.push_back(
					    &spawnInverse[{spawnType, UnitSize::Large, UnitMovement::Flying, true}]);

					priorityList.push_back(
					    &spawnMaps[{spawnType, UnitSize::Small, UnitMovement::Walking, false}]);
					priorityList.push_back(
					    &spawnMaps[{spawnType, UnitSize::Small, UnitMovement::Walking, true}]);
					priorityList.push_back(
					    &spawnInverse[{spawnType, UnitSize::Small, UnitMovement::Walking, false}]);
					priorityList.push_back(
					    &spawnInverse[{spawnType, UnitSize::Small, UnitMovement::Walking, true}]);

					priorityList.push_back(
					    &spawnMaps[{spawnType, UnitSize::Small, UnitMovement::Flying, false}]);
					priorityList.push_back(
					    &spawnMaps[{spawnType, UnitSize::Small, UnitMovement::Flying, true}]);
					priorityList.push_back(
					    &spawnInverse[{spawnType, UnitSize::Small, UnitMovement::Flying, false}]);
					priorityList.push_back(
					    &spawnInverse[{spawnType, UnitSize::Small, UnitMovement::Flying, true}]);
				}
				else if (needLarge)
				{
					priorityList.push_back(
					    &spawnMaps[{spawnType, UnitSize::Large, UnitMovement::Any, false}]);
					priorityList.push_back(
					    &spawnMaps[{spawnType, UnitSize::Large, UnitMovement::Any, true}]);
					priorityList.push_back(
					    &spawnInverse[{spawnType, UnitSize::Large, UnitMovement::Any, false}]);
					priorityList.push_back(
					    &spawnInverse[{spawnType, UnitSize::Large, UnitMovement::Any, true}]);

					priorityList.push_back(
					    &spawnMaps[{spawnType, UnitSize::Small, UnitMovement::Any, false}]);
					priorityList.push_back(
					    &spawnMaps[{spawnType, UnitSize::Small, UnitMovement::Any, true}]);
					priorityList.push_back(
					    &spawnInverse[{spawnType, UnitSize::Small, UnitMovement::Any, false}]);
					priorityList.push_back(
					    &spawnInverse[{spawnType, UnitSize::Small, UnitMovement::Any, true}]);
				}
				else if (needWalker)
				{
					priorityList.push_back(
					    &spawnMaps[{spawnType, UnitSize::Any, UnitMovement::Walking, false}]);
					priorityList.push_back(
					    &spawnMaps[{spawnType, UnitSize::Any, UnitMovement::Walking, true}]);
					priorityList.push_back(
					    &spawnInverse[{spawnType, UnitSize::Any, UnitMovement::Walking, false}]);
					priorityList.push_back(
					    &spawnInverse[{spawnType, UnitSize::Any, UnitMovement::Walking, true}]);

					priorityList.push_back(
					    &spawnMaps[{spawnType, UnitSize::Any, UnitMovement::Flying, false}]);
					priorityList.push_back(
					    &spawnMaps[{spawnType, UnitSize::Any, UnitMovement::Flying, true}]);
					priorityList.push_back(
					    &spawnInverse[{spawnType, UnitSize::Any, UnitMovement::Flying, false}]);
					priorityList.push_back(
					    &spawnInverse[{spawnType, UnitSize::Any, UnitMovement::Flying, true}]);
				}
				else
				{
					priorityList.push_back(
					    &spawnMaps[{spawnType, UnitSize::Any, UnitMovement::Any, false}]);
					priorityList.push_back(
					    &spawnMaps[{spawnType, UnitSize::Any, UnitMovement::Any, true}]);
					priorityList.push_back(
					    &spawnInverse[{spawnType, UnitSize::Any, UnitMovement::Any, false}]);
					priorityList.push_back(
					    &spawnInverse[{spawnType, UnitSize::Any, UnitMovement::Any, true}]);
				}
				priorityList.push_back(&spawnOther);

				// Select a block randomly
				for (auto &l : priorityList)
				{
					if (l->size() == 0)
						continue;
					block = pickRandom(state.rng, *l);
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
								if (unitsToSpawn.empty())
									break;
							}
							if (unitsToSpawn.empty())
								break;
						}
						if (unitsToSpawn.empty())
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
					auto spawnPos = pickRandom(state.rng, block->positions[needWalker]);
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
								if (unitsToSpawn.empty()
								    // This makes us spawn every civilian and loner individually
								    // Except X-Com scientists
								    || (numSpawned > 0 && !requestCivilian &&
								        (u->getAIType() == AIType::None ||
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
			u->facing = pickRandom(state.rng, *u->agent->getAllowedFacings());
		}
		// Stance
		u->setBodyState(state, u->agent->type->bodyType->getFirstAllowedState());
		if (config().getBool("OpenApoc.NewFeature.RunAndKneel") && u->owner == state.getPlayer())
		{
			u->setKneelingMode(KneelingMode::Kneeling);
			u->setMovementMode(MovementMode::Running);
		}
		else
		{
			if (u->current_body_state == BodyState::Kneeling ||
			    u->current_body_state == BodyState::Prone)
			{
				u->setMovementMode(MovementMode::Prone);
			}
			else
			{
				u->setMovementMode(MovementMode::Walking);
			}
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
	agent->city = state.current_city;
	auto unit = state.current_battle->placeUnit(state, agent, position);
	unit->falling = true;
	if (facing.x == 0 && facing.y == 0)
	{
		if (agentType->bodyType->allowed_facing.empty())
		{
			facing = {randBoundsInclusive(state.rng, 0, 1), randBoundsInclusive(state.rng, 0, 1)};
			if (facing.x == 0 && facing.y == 0)
			{
				facing.y = 1;
			}
		}
		else
		{
			facing =
			    pickRandom(state.rng, agentType->bodyType->allowed_facing.at(agent->appearance));
		}
	}
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
	// Doodad
	if (!doodadType)
	{
		doodadType = {&state, "DOODAD_30_EXPLODING_PAYLOAD"};
	}
	placeDoodad(doodadType, position);

	// Sound
	if (!damageType->explosionSounds.empty())
	{
		fw().soundBackend->playSample(pickRandom(state.rng, damageType->explosionSounds), position);
	}

	// Explosion
	auto explosion = mksp<BattleExplosion>(
	    position, damageType, power, depletionRate,
	    !config().getBool("OpenApoc.NewFeature.InstantExplosionDamage"), ownerOrg, ownerUnit);
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
	unit->cloakTicksAccumulated = CLOAK_TICKS_REQUIRED_UNIT;
	unit->initCryTimer(state);
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
				existingHazard->dieAndRemove(state, false);
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
	// Since projectiles can kill projectiles just kill everyone in the end
	std::set<std::tuple<sp<Projectile>, bool, bool>> deadProjectiles;
	for (auto &p : projectiles)
	{
		notifyAction(p->position);
		auto c = p->checkProjectileCollision(*map);
		if (c)
		{
			// Alert intended unit that he's on fire
			auto unit = c.projectile->trackedUnit;
			if (unit)
			{
				unit->notifyUnderFire(state, c.projectile->firerPosition,
				                      c.projectile->firerUnit->owner == unit->owner ||
				                          visibleUnits[unit->owner].find(c.projectile->firerUnit) !=
				                              visibleUnits[unit->owner].end());
			}
			// Handle collision
			bool playSound = true;
			bool displayDoodad = true;
			if (c.projectile->damageType->explosive)
			{
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
			deadProjectiles.emplace(c.projectile->shared_from_this(), displayDoodad, playSound);
		}
	}
	// Kill projectiles that collided
	for (auto &p : deadProjectiles)
	{
		std::get<0>(p)->die(state, std::get<1>(p), std::get<2>(p));
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

void Battle::updatePathfinding(GameState &, unsigned int ticks)
{
	// Throttling updates so that big explosions won't lag
	static const int LIMIT_PER_TICK = 10;

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

	int updatesRemaining = ticks > 0 ? LIMIT_PER_TICK * ticks : -1;

	// Now update all paths
	for (int i = 0; i < lbCount - 1; i++)
	{
		for (int j = i + 1; j < lbCount; j++)
		{
			if (!linkNeedsUpdate[i + j * lbCount])
			{
				continue;
			}
			if (updatesRemaining > 0)
			{
				updatesRemaining--;
				if (updatesRemaining == 0)
				{
					return;
				}
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
				    true, true, &cost, distance * 4 * PATH_COST_LIMIT_MULTIPLIER);

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
}

void Battle::update(GameState &state, unsigned int ticks)
{

	if (missionEndTimer > 0)
	{
		missionEndTimer++;
		ticksWithoutAction = 0;
		for (auto &p : participants)
		{
			ticksWithoutSeenAction[p] = 0;
		}
	}
	switch (mode)
	{
		case Mode::TurnBased:
		{
			updateTB(state);
			break;
		}
		case Mode::RealTime:
		{
			updateRT(state, ticks);
			break;
		}
	}
	updateProjectiles(state, ticks);
	for (auto &o : this->doors)
	{
		o.second->update(state, ticks);
	}
	for (auto it = this->doodads.begin(); it != this->doodads.end();)
	{
		auto d = *it++;
		d->update(state, ticks);
	}
	for (auto it = this->hazards.begin(); it != this->hazards.end();)
	{
		if ((*it)->update(state, ticks))
			it = hazards.erase(it);
		else
			++it;
	}
	for (auto it = this->explosions.begin(); it != this->explosions.end();)
	{
		auto d = *it++;
		d->update(state, ticks);
	}
	for (auto &o : this->map_parts)
	{
		o->update(state, ticks);
	}
	for (auto it = this->items.begin(); it != this->items.end();)
	{
		auto p = *it++;
		p->update(state, ticks);
	}
	for (auto &o : this->scanners)
	{
		o.second->update(state, ticks);
	}
	for (auto &o : this->units)
	{
		o.second->update(state, ticks);
	}
	{
		auto result = aiBlock.think(state);
		for (auto &entry : result)
		{
			BattleUnit::executeGroupAIDecision(state, entry.second, entry.first);
		}
	}

	// Now after we called update() for everything, we update what needs to be updated last

	// Update unit vision for units that see changes in terrain or hazards
	updateVision(state);
	updatePathfinding(state, ticks);
}

void Battle::updateTB(GameState &state)
{
	ticksWithoutAction++;
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
				fw().pushEvent(new GameLocationEvent(GameEventType::ZoomView, u.second->position));
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

void Battle::updateRT(GameState &state, unsigned int ticks)
{
	// Spawn reinforcements
	if (reinforcementsInterval > 0)
	{
		ticksUntilNextReinforcement -= ticks;
		while (ticksUntilNextReinforcement <= 0)
		{
			ticksUntilNextReinforcement += reinforcementsInterval;
			spawnReinforcements(state);
		}
	}
}

void Battle::updateTBBegin(GameState &state)
{
	notifyAction();
	turnEndAllowed = false;
	lowMoraleProcessed = false;

	if (reinforcementsInterval > 0 && currentActiveOrganisation == locationOwner)
	{
		ticksUntilNextReinforcement -= TICKS_PER_TURN;
		while (ticksUntilNextReinforcement <= 0)
		{
			ticksUntilNextReinforcement += reinforcementsInterval;
			spawnReinforcements(state);
		}
	}

	for (auto &o : this->units)
	{
		if (o.second->owner == currentActiveOrganisation)
		{
			o.second->updateTB(state);
		}
	}
}

void Battle::updateTBEnd(GameState &state)
{
	for (auto it = this->hazards.begin(); it != this->hazards.end();)
	{
		if ((*it)->ownerOrganisation == currentActiveOrganisation && (*it)->updateTB(state))
			it = hazards.erase(it);
		else
			++it;
	}
	for (auto it = this->items.begin(); it != this->items.end();)
	{
		auto p = *it++;
		if (p->owner == currentActiveOrganisation)
		{
			p->updateTB(state);
		}
	}
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

int Battle::killStrandedUnits(GameState &state, StateRef<Organisation> org, bool preview)
{
	int countKilled = 0;

	for (auto &u : units)
	{
		if (u.second->owner != org || u.second->isDead())
		{
			continue;
		}
		// Find closest exit
		float distanceToExit = FLT_MAX;
		for (auto &e : exits)
		{
			float distance = glm::length(u.second->position - (Vec3<float>)e);
			if (distance < distanceToExit)
			{
				distanceToExit = distance;
			}
		}
		// Find closest enemy from org that sees us
		float distanceToEnemy = FLT_MAX;
		for (auto &owner : participants)
		{
			if (owner == org || owner->isRelatedTo(org) != Organisation::Relation::Hostile)
			{
				continue;
			}
			for (auto &spotted : visibleUnits.at(owner))
			{
				if (spotted.id != u.first)
				{
					continue;
				}
				// Org sees this unit
				// Look for closest unit from this org
				for (auto &e : units)
				{
					if (e.second->owner != owner || e.second->isDead())
					{
						continue;
					}
					float distance = glm::length(u.second->position - e.second->position);
					if (distance < distanceToEnemy)
					{
						distanceToEnemy = distance;
					}
				}
				// No need to look further, we already processed this org
				break;
			}
		}
		// Exit must be three times closer than enemy for escape to be possible
		if (distanceToEnemy / 3.0f < distanceToExit)
		{
			countKilled++;
			if (!preview)
			{
				u.second->agent->modified_stats.health = 0;
				u.second->die(state);
			}
		}
	}
	return countKilled;
}

void Battle::abortMission(GameState &state)
{
	killStrandedUnits(state, currentPlayer);
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
				bool normalUnit = false;
				for (auto &mst : u.second->agent->type->bodyType->allowed_movement_states)
				{
					switch (mst)
					{
						case OpenApoc::MovementState::Normal:
						case OpenApoc::MovementState::Running:
						case OpenApoc::MovementState::Strafing:
						case OpenApoc::MovementState::Reverse:
						case OpenApoc::MovementState::Brainsuck:
							normalUnit = true;
							break;
						default:
							break;
					}
					if (normalUnit)
						break;
				}

				if (normalUnit)
				{
					orgsAlive.insert(p);
					break;
				}
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

void Battle::checkIfBuildingDisabled(GameState &state [[maybe_unused]])
{
	if (!buildingCanBeDisabled || buildingDisabled || !tryDisableBuilding())
	{
		return;
	}
	buildingDisabled = true;
	fw().pushEvent(new GameEvent(GameEventType::BuildingDisabled));
}

bool Battle::tryDisableBuilding()
{
	// Find a mission objective unit
	for (auto &u : units)
	{
		if (u.second->owner != locationOwner)
		{
			continue;
		}
		if (u.second->agent->type->missionObjective && !u.second->isDead())
		{
			// Mission objective unit found alive
			return false;
		}
	}
	// Find a mission objective object
	for (auto &mp : map_parts)
	{
		if (mp->type->missionObjective && !mp->destroyed)
		{
			// Mission objective unit found alive
			return false;
		}
	}
	// Found nothing, building disabled
	return true;
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

void Battle::spawnReinforcements(GameState &state)
{
	if (locationOwner->guard_types_reinforcements.empty())
	{
		return;
	}
	int countUnits = 0;
	for (auto &u : units)
	{
		if (u.second->owner != locationOwner || u.second->retreated || u.second->isDead())
		{
			continue;
		}
		countUnits++;
	}
	if (countUnits >= MAX_UNITS_PER_SIDE)
	{
		return;
	}
	std::set<Vec3<int>> reinforcementLocations;
	for (auto &mp : map_parts)
	{
		if (!mp->type->reinforcementSpawner || mp->destroyed)
		{
			continue;
		}
		Vec3<int> pos = mp->position;
		if (map->getTile(pos)->getUnitIfPresent(true, true))
		{
			continue;
		}
		reinforcementLocations.insert(pos);
	}
	if (reinforcementLocations.empty())
	{
		return;
	}
	// FIXME: Proper spawning algorithm for reinforcements, for now spawn 4 randoms
	for (int i = 0; i < 4; i++)
	{
		auto pos = pickRandom(state.rng, reinforcementLocations);
		reinforcementLocations.erase(pos);
		auto type = pickRandom(state.rng, locationOwner->guard_types_reinforcements);
		auto u = state.current_battle->spawnUnit(state, locationOwner, type,
		                                         {pos.x + 0.5f, pos.y + 0.5f, pos.z + 0.1f}, {0, 0},
		                                         type->bodyType->getFirstAllowedState());
		if (++countUnits >= MAX_UNITS_PER_SIDE)
		{
			break;
		}
	}
}

void Battle::handleProjectileHit(GameState &state, sp<Projectile> projectile, bool displayDoodad,
                                 bool playSound, bool expired)
{
	if (projectile->damageType->explosive)
	{
		auto explosion =
		    addExplosion(state, projectile->position, projectile->doodadType,
		                 projectile->damageType, projectile->damage, projectile->depletionRate,
		                 projectile->firerUnit->owner, projectile->firerUnit);
	}
	if (displayDoodad && projectile->doodadType)
	{
		this->placeDoodad(projectile->doodadType, projectile->position);
	}
	if (playSound && projectile->impactSfx &&
	    (!expired || projectile->splitIntoTypesBattle.empty()))
	{
		fw().soundBackend->playSample(projectile->impactSfx, projectile->position);
	}
	if (expired)
	{
		std::set<sp<Sample>> fireSounds;
		for (auto &p : projectile->splitIntoTypesBattle)
		{
			auto direction = (float)randBoundsInclusive(state.rng, 0, 628) / 100.0f;
			auto velocity = glm::normalize(
			    VehicleType::directionToVector(VehicleType::getDirectionLarge(direction)));
			velocity *= p->speed * PROJECTILE_VELOCITY_MULTIPLIER;
			auto newProj = mksp<Projectile>(
			    p->guided ? Projectile::Type::Missile : Projectile::Type::Beam,
			    projectile->firerUnit, projectile->trackedUnit, projectile->targetPosition,
			    projectile->position, velocity, p->turn_rate, p->ttl, p->damage, 0, 0, p->tail_size,
			    p->projectile_sprites, p->impact_sfx, p->explosion_graphic, p->damage_type);
			map->addObjectToMap(newProj);
			projectiles.insert(newProj);
			fireSounds.insert(p->fire_sfx);
		}
		for (auto &s : fireSounds)
		{
			fw().soundBackend->playSample(s, projectile->position);
		}
	}
	projectiles.erase(projectile);
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
	float dispersion = 100 - accuracy;
	// Introduce minimal dispersion?
	dispersion = std::max(0.0f, dispersion);

	if (cloaked)
	{
		dispersion *= dispersion;
		float cloakDispersion =
		    2000.0f / (BattleUnitTileHelper::getDistanceStatic(firePosition, target) / 4.0f + 3.0f);
		dispersion += cloakDispersion * cloakDispersion;
		dispersion = sqrtf(dispersion);
	}

	auto delta = (target - firePosition) * dispersion / 1000.0f;
	if (delta.x == 0.0f && delta.y == 0.0f && delta.z == 0.0f)
	{
		return;
	}

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
	    receiver->getAIType() == AIType::None || receiver->getAIType() == AIType::Civilian ||
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
// In case battle is in a craft (UFO)
void Battle::beginBattle(GameState &state, bool hotseat, StateRef<Organisation> opponent,
                         std::list<StateRef<Agent>> &player_agents,
                         const std::map<StateRef<AgentType>, int> *aliens,
                         StateRef<Vehicle> player_craft, StateRef<Vehicle> target_craft)
{
	if (state.current_battle)
	{
		LogError("Battle::beginBattle called while another battle is in progress!");
		return;
	}
	auto b =
	    BattleMap::createBattle(state, opponent, player_agents, aliens, player_craft, target_craft);
	if (!b)
	{
		return;
	}
	b->locationOwner = target_craft->owner;
	b->hotseat = hotseat;
	state.current_battle = b;
}

// To be called when battle must be started, before showing battle briefing screen
// In case battle is in a building
void Battle::beginBattle(GameState &state, bool hotseat, StateRef<Organisation> opponent,
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
	auto b = BattleMap::createBattle(state, opponent, player_agents, aliens, guards, civilians,
	                                 player_craft, target_building);
	if (!b)
	{
		return;
	}
	b->hotseat = hotseat;
	b->locationOwner = target_building->owner;
	b->buildingCanBeDisabled = !b->tryDisableBuilding();
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

	if (b->mission_type == MissionType::BaseDefense)
	{
		for (size_t i = 0; i < b->visibleTiles[b->locationOwner].size(); i++)
		{
			b->visibleTiles[b->locationOwner][i] = true;
		}
		for (size_t i = 0; i < b->visibleBlocks[b->locationOwner].size(); i++)
		{
			b->visibleBlocks[b->locationOwner][i] = true;
		}
	}

	for (auto &u : state.current_battle->units)
	{
		u.second->updateCheckBeginFalling(state);
	}

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

	// Remember time
	state.updateBeforeBattle();
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
	auto aliens = state.getAliens();
	state.current_battle->unloadResources(state);

	// Remove active battle scanners and deactivate medikits and motion scanners
	for (auto &u : state.current_battle->units)
	{
		for (auto &e : u.second->agent->equipment)
		{
			e->battleScanner.clear();
			e->inUse = false;
		}
	}

	// Process MCed units
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

	// Loot (temporary storage for items that become loot)
	std::list<sp<AEquipment>> loot;

	//	- Prepare list of surviving aliens
	std::list<sp<BattleUnit>> retreatedAliens;
	std::list<sp<BattleUnit>> liveAliens;
	std::list<sp<BattleUnit>> deadAliens;
	for (auto &u : state.current_battle->units)
	{
		if (u.second->owner != aliens)
		{
			continue;
		}
		if (u.second->retreated)
		{
			retreatedAliens.push_back(u.second);
		}
		else if (u.second->isDead())
		{
			deadAliens.push_back(u.second);
		}
		else
		{
			liveAliens.push_back(u.second);
		}
	}

	// Process player agents
	// - strip them of bio items and artifacts
	// - strip dead ones
	// - mark as fought
	// - give mission to return if need to
	for (auto &u : state.current_battle->units)
	{
		if (u.second->owner != player)
		{
			continue;
		}
		u.second->agent->recentlyFought = true;
		std::list<sp<AEquipment>> itemsToStrip;
		for (auto &e : u.second->agent->equipment)
		{
			if (u.second->isDead() || e->type->bioStorage || !e->type->canBeUsed(state, player) ||
			    (e->payloadType && !e->payloadType->canBeUsed(state, player)))
			{
				itemsToStrip.push_back(e);
			}
			else
			{
				// Reward for captured loot that stayed on agents
				if (e->ownerOrganisation != player)
				{
					state.current_battle->score.equipmentCaptured += e->type->score;
				}
			}
		}
		for (auto &e : itemsToStrip)
		{
			u.second->agent->removeEquipment(state, e);
			auto clip = e->unloadAmmo();
			if (clip)
			{
				loot.push_back(clip);
			}
			loot.push_back(e);
		}
		// Send home if not on vehicle. Note that some units may not have a home building
		if (!u.second->agent->currentVehicle && !state.current_battle->skirmish &&
		    u.second->agent->homeBuilding != nullptr)
		{
			u.second->agent->setMission(state, AgentMission::gotoBuilding(state, *u.second->agent));
		}
	}

	// If mission == Alien extermination, remove the red alien spotted circle
	if (state.current_battle->mission_type == Battle::MissionType::AlienExtermination)
	{
		StateRef<Building> location = {&state, state.current_battle->mission_location_id};
		location->detected = false;
	}

	// If player won and didn't retreat, player secures the area
	// - give him loot
	// - give him alien remains
	if (state.current_battle->playerWon && !state.current_battle->winnerHasRetreated)
	{
		bool playerHasBioStorage = state.current_battle->player_craft &&
		                           state.current_battle->player_craft->getMaxBio() > 0;
		// Live alien loot
		for (auto &u : liveAliens)
		{
			if (u->agent->type->liveSpeciesItem)
			{
				if (playerHasBioStorage)
				{
					state.current_battle->score.liveAlienCaptured +=
					    u->agent->type->liveSpeciesItem->score;
				}
				if (u->agent->type->liveSpeciesItem->bioStorage)
				{
					state.current_battle->bioLoot[u->agent->type->liveSpeciesItem] =
					    state.current_battle->bioLoot[u->agent->type->liveSpeciesItem] + 1;
				}
				else
				{
					state.current_battle->cargoLoot[u->agent->type->liveSpeciesItem] =
					    state.current_battle->cargoLoot[u->agent->type->liveSpeciesItem] + 1;
				}
			}
			else
			{
				// Maybe alien has only a dead option?
				deadAliens.push_back(u);
			}
		}
		// Dead alien loot
		for (auto &u : deadAliens)
		{
			if (u->agent->type->deadSpeciesItem && !u->destroyed)
			{
				if (u->agent->type->deadSpeciesItem->bioStorage)
				{
					state.current_battle->bioLoot[u->agent->type->deadSpeciesItem] =
					    state.current_battle->bioLoot[u->agent->type->deadSpeciesItem] + 1;
				}
				else
				{
					state.current_battle->cargoLoot[u->agent->type->deadSpeciesItem] =
					    state.current_battle->cargoLoot[u->agent->type->deadSpeciesItem] + 1;
				}
			}
		}
		// Item loot
		for (auto &e : state.current_battle->items)
		{
			loot.push_back(e->item);
		}
		LogWarning("Implement UFO parts loot");
	}
	// Player didn't secure the area
	// - doesn't get loot
	// - kill all items (and make player suffer penalties)
	// - deal with surviving aliens
	else
	{
		std::list<sp<BattleItem>> itemsToKill;
		for (auto &e : state.current_battle->items)
		{
			itemsToKill.push_back(e);
		}
		for (auto &e : itemsToKill)
		{
			e->die(state, false);
		}
		// If UFO - aliens retreat,
		if (state.current_battle->mission_type == Battle::MissionType::UfoRecovery)
		{
			for (auto &a : liveAliens)
			{
				retreatedAliens.push_back(a);
			}
		}
		// If non-alien building - aliens get back in
		// If alien building - all aliens vanish
		else
		{
			StateRef<Building> location = {&state, state.current_battle->mission_location_id};
			if (location->owner != aliens)
			{
				for (auto &a : liveAliens)
				{
					location->current_crew[a->agent->type] =
					    location->current_crew[a->agent->type] + 1;
				}
			}
			else
			{
				liveAliens.clear();
				retreatedAliens.clear();
			}
		}
	}
	// Regardless what happened, give player loot (if player retreated - will only get agent loot)
	for (auto &e : loot)
	{
		// Reward for captured loot
		if (e->ownerOrganisation != player)
		{
			state.current_battle->score.equipmentCaptured += e->type->score;
		}
		int mult = e->type->type == AEquipmentType::Type::Ammo ? e->ammo : 1;
		if (e->type->bioStorage)
		{
			state.current_battle->bioLoot[e->type] = state.current_battle->bioLoot[e->type] + mult;
		}
		else
		{
			state.current_battle->cargoLoot[e->type] =
			    state.current_battle->cargoLoot[e->type] + mult;
		}
	}
	// Regardless of what happened, retreated aliens go to a nearby building
	if (!retreatedAliens.empty())
	{
		// FIXME: Should find 15 closest buildings that are intact and within 15 tiles
		// (center to center) and pick one of them
		LogWarning("Properly find building to house retreated aliens");
		Vec2<int> battleLocation;
		StateRef<City> city;
		if (state.current_battle->mission_type == Battle::MissionType::UfoRecovery)
		{
			StateRef<Vehicle> location = {&state, state.current_battle->mission_location_id};
			city = location->city;
			battleLocation = {location->position.x, location->position.y};
		}
		else
		{
			StateRef<Building> location = {&state, state.current_battle->mission_location_id};
			city = location->city;
			battleLocation = location->bounds.p0;
		}
		StateRef<Building> closestBuilding;
		int closestDistance = INT_MAX;
		for (auto &b : city->buildings)
		{
			int distance = std::abs(b.second->bounds.p0.x - battleLocation.x) +
			               std::abs(b.second->bounds.p0.y - battleLocation.y);
			if (distance < closestDistance)
			{
				closestDistance = distance;
				closestBuilding = {&state, b.first};
			}
		}
		if (!closestBuilding)
		{
			LogError("WTF? No building in city closer than INT_MAX?");
			return;
		}
		for (auto &a : retreatedAliens)
		{
			closestBuilding->current_crew[a->agent->type] =
			    closestBuilding->current_crew[a->agent->type] + 1;
		}
	}
	// Remove dead player agents and all enemy agents from the game and from vehicles
	std::list<sp<BattleUnit>> unitsToRemove;
	for (auto &u : state.current_battle->units)
	{
		if (u.second->owner != player || u.second->isDead() || u.second->agent->destroyAfterBattle)
		{
			unitsToRemove.push_back(u.second);
		}
	}
	for (auto &u : unitsToRemove)
	{
		u->agent->die(state, true);
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
		if (!u.second->agent->type->allowsDirectControl)
		{
			continue;
		}
		u.second->processExperience(state);
		unitsByRating[-u.second->combatRating].push_back(u.second);
	}
	// Create count of ranks
	std::map<Rank, int> countRanks;
	for (auto &a : state.agents)
	{
		if (!a.second->type->allowsDirectControl ||
		    a.second->type->role != AgentType::Role::Soldier)
		{
			continue;
		}
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
}

// To be called after battle was finished, score screen was shown and before returning to cityscape
void Battle::exitBattle(GameState &state)
{
	if (!state.current_battle)
	{
		LogError("Battle::ExitBattle called with no battle!");
		return;
	}

	// Fake battle, remove fake stuff, restore relationships
	if (state.current_battle->skirmish)
	{
		// Erase agents
		std::list<UString> agentsToRemove;
		for (auto &a : state.agents)
		{
			if (!a.second->city)
			{
				if (a.second->currentBuilding)
				{
					a.second->currentBuilding->currentAgents.erase({&state, a.first});
				}
				if (a.second->currentVehicle)
				{
					a.second->currentVehicle->currentAgents.erase({&state, a.first});
				}
				agentsToRemove.push_back(a.first);
			}
		}
		for (auto &a : agentsToRemove)
		{
			state.agents.erase(a);
		}

		// Erase base and building
		StateRef<Base> fakeBase = {&state, "BASE_SKIRMISH"};
		auto city = fakeBase->building->city;
		fakeBase->building->currentAgents.clear();
		fakeBase->building->base.clear();
		fakeBase->building.clear();
		city->buildings.erase("BUILDING_SKIRMISH");
		state.player_bases.erase("BASE_SKIRMISH");

		// Erase vehicle
		if (state.current_battle->player_craft)
		{
			state.current_battle->player_craft->die(state, true);
		}

		// Restore relationships
		for (auto e : state.current_battle->relationshipsBeforeSkirmish)
		{
			auto org = e.first;
			org->current_relations[state.getPlayer()] = e.second;
			state.getPlayer()->current_relations[e.first] = e.second;
		}

		// Restore score
		state.totalScore.tacticalMissions = state.current_battle->scoreBeforeSkirmish;

		// That's it for fake battle, return
		state.current_battle = nullptr;
		return;
	}

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

	// Apply score to player score
	state.totalScore.tacticalMissions += state.current_battle->score.getTotal();
	state.weekScore.tacticalMissions += state.current_battle->score.getTotal();

	// LOOT!

	// Compile list of player vehicles
	std::list<StateRef<Vehicle>> playerVehicles;
	std::set<StateRef<Vehicle>> returningVehicles;
	if (state.current_battle->player_craft)
	{
		playerVehicles.push_back(state.current_battle->player_craft);
		returningVehicles.insert(state.current_battle->player_craft);
	}
	if (config().getBool("OpenApoc.NewFeature.AllowNearbyVehicleLootPickup"))
	{
		if (state.current_battle->mission_type == Battle::MissionType::UfoRecovery)
		{
			StateRef<City> city;
			StateRef<Vehicle> location = {&state, state.current_battle->mission_location_id};
			city = location->city;

			for (auto &v : state.vehicles)
			{
				// Check every player owned vehicle located in city
				if (v.second->owner != player || v.second->city != city ||
				    v.second->currentBuilding
				    // Player's vehicle was already added and has priority
				    || v.first == state.current_battle->player_craft.id)
				{
					continue;
				}
				if (glm::length(location->position - v.second->position) < VEHICLE_NEARBY_THRESHOLD)
				{
					playerVehicles.emplace_back(&state, v.first);
				}
			}
		}
		else
		{
			StateRef<Building> location = {&state, state.current_battle->mission_location_id};
			for (auto &v : location->currentVehicles)
			{
				// Player's vehicle was already added and has priority
				if (v->owner == player && v != state.current_battle->player_craft)
				{
					playerVehicles.push_back(v);
				}
			}
		}
	}

	// List of vehicle loot
	std::map<StateRef<VEquipmentType>, int> vehicleLoot;

	if (state.current_battle->mission_type == MissionType::UfoRecovery)
	{
		auto vehicle = StateRef<Vehicle>(&state, state.current_battle->mission_location_id);
		for (auto &e : vehicle->loot)
		{
			vehicleLoot[e]++;
		}
	}

	// List of bio-containment leftover loot
	std::map<StateRef<AEquipmentType>, int> leftoverBioLoot;
	// List of cargo bay leftover loot
	std::map<StateRef<AEquipmentType>, int> leftoverCargoLoot;
	// List of vehicle leftover loot
	std::map<StateRef<VEquipmentType>, int> leftoverVehicleLoot;

	// Check cargo limits (this can move loot into leftover loot)
	if (config().getBool("OpenApoc.NewFeature.EnforceCargoLimits"))
	{
		LogWarning("Implement feature: Enforce containment limits");

		// FIXME: Implement enforce cargo limits
		// Basically here we should open a window where we offer to leave behind
		// loot that won't fit combined cargo capacity and alien capacity of player craft
		// That loot is moved to leftover loot
	}

	// If player has vehicle with cargo capacity all cargo goes to leftover loot
	// If player has vehicle with bio capacity then all bio loot goes to leftover loot
	// This is regardless of "enforce limits" which only makes us enforce it
	// on vehicles that have capacity in the first place
	bool bioCarrierPresent = false;
	bool cargoCarrierPresent = false;
	for (auto &v : playerVehicles)
	{
		if (v->getMaxCargo() > 0)
		{
			cargoCarrierPresent = true;
		}
		if (v->getMaxBio() > 0)
		{
			bioCarrierPresent = true;
		}
	}
	if (!cargoCarrierPresent)
	{
		for (auto &e : state.current_battle->cargoLoot)
		{
			leftoverCargoLoot[e.first] = e.second;
		}
		for (auto &e : vehicleLoot)
		{
			leftoverVehicleLoot[e.first] = e.second;
		}
		state.current_battle->cargoLoot.clear();
	}
	if (!bioCarrierPresent)
	{
		for (auto &e : state.current_battle->bioLoot)
		{
			leftoverBioLoot[e.first] = e.second;
		}
		state.current_battle->bioLoot.clear();
	}

	// Bio loot remaining?
	if (!leftoverBioLoot.empty())
	{
		// Bio loot is wasted if can't be loaded on player craft
	}

	// Cargo loot remaining?
	if (leftoverCargoLoot.empty())
	{
		if (config().getBool("OpenApoc.NewFeature.AllowBuildingLootDeposit"))
		{
			if (state.current_battle->mission_type == Battle::MissionType::UfoRecovery)
			{
				// Still can't do anything if we're recovering UFO
			}
			else
			{
				// Deposit loot into building, call for pickup
				StateRef<Building> location = {&state, state.current_battle->mission_location_id};
				auto homeBuilding =
				    playerVehicles.empty() ? nullptr : playerVehicles.front()->homeBuilding;
				if (!homeBuilding)
				{
					homeBuilding = state.player_bases.begin()->second->building;
				}
				for (auto &e : leftoverCargoLoot)
				{
					int price = 0;
					location->cargo.emplace_back(state, e.first, e.second, price, nullptr,
					                             homeBuilding);
				}
				for (auto &e : leftoverVehicleLoot)
				{
					int price = 0;
					location->cargo.emplace_back(state, e.first, e.second, price, nullptr,
					                             homeBuilding);
				}
			}
		}
	}

	// Load cargo/bio into vehicles
	if (!playerVehicles.empty())
	{
		// Go through every vehicle loot position
		// Try to load into every vehicle until amount remaining is zero
		std::list<StateRef<VEquipmentType>> vehicleLootToRemove;
		for (auto &e : vehicleLoot)
		{
			for (auto &v : playerVehicles)
			{
				if (v->getMaxCargo() == 0)
				{
					continue;
				}
				if (e.second == 0)
				{
					continue;
				}
				int maxAmount = config().getBool("OpenApoc.NewFeature.EnforceCargoLimits")
				                    ? std::min(e.second, (v->getMaxCargo() - v->getCargo()) /
				                                             e.first->store_space)
				                    : e.second;
				if (maxAmount > 0)
				{
					e.second -= maxAmount;
					int price = 0;
					v->cargo.emplace_back(state, e.first, maxAmount, price, nullptr,
					                      v->homeBuilding);
					returningVehicles.insert(v);
				}
			}
			if (e.second == 0)
			{
				vehicleLootToRemove.push_back(e.first);
			}
		}
		// Remove stored loot
		for (auto &e : vehicleLootToRemove)
		{
			vehicleLoot.erase(e);
		}
		// Put remainder on first vehicle
		for (auto &e : vehicleLoot)
		{
			for (auto &v : playerVehicles)
			{
				if (v->getMaxCargo() == 0)
				{
					continue;
				}
				int maxAmount = e.second;
				if (maxAmount > 0)
				{
					e.second -= maxAmount;
					int price = 0;
					v->cargo.emplace_back(state, e.first, maxAmount, price, nullptr,
					                      v->homeBuilding);
					returningVehicles.insert(v);
				}
			}
		}
		// Go through every loot position
		// Try to load into every vehicle until amount remaining is zero
		std::list<StateRef<AEquipmentType>> cargoLootToRemove;
		for (auto &e : state.current_battle->cargoLoot)
		{
			for (auto &v : playerVehicles)
			{
				if (v->getMaxCargo() == 0)
				{
					continue;
				}
				if (e.second == 0)
				{
					continue;
				}
				int divisor = e.first->type == AEquipmentType::Type::Ammo ? e.first->max_ammo : 1;
				int maxAmount = config().getBool("OpenApoc.NewFeature.EnforceCargoLimits")
				                    ? std::min(e.second, (v->getMaxCargo() - v->getCargo()) /
				                                             e.first->store_space * divisor)
				                    : e.second;
				if (maxAmount > 0)
				{
					e.second -= maxAmount;
					int price = 0;
					v->cargo.emplace_back(state, e.first, maxAmount, price, nullptr,
					                      v->homeBuilding);
					returningVehicles.insert(v);
				}
			}
			if (e.second == 0)
			{
				cargoLootToRemove.push_back(e.first);
			}
		}
		// Remove stored loot
		for (auto &e : cargoLootToRemove)
		{
			state.current_battle->cargoLoot.erase(e);
		}
		// Put remainder on first vehicle
		for (auto &e : state.current_battle->cargoLoot)
		{
			for (auto &v : playerVehicles)
			{
				if (v->getMaxCargo() == 0)
				{
					continue;
				}
				int maxAmount = e.second;
				if (maxAmount > 0)
				{
					e.second -= maxAmount;
					int price = 0;
					v->cargo.emplace_back(state, e.first, maxAmount, price, nullptr,
					                      v->homeBuilding);
					returningVehicles.insert(v);
				}
			}
		}
		// Go through every bio loot position
		// Try to load into every vehicle until amount remaining is zero
		std::list<StateRef<AEquipmentType>> bioLootToRemove;
		for (auto &e : state.current_battle->bioLoot)
		{
			for (auto &v : playerVehicles)
			{
				if (v->getMaxBio() == 0)
				{
					continue;
				}
				if (e.second == 0)
				{
					continue;
				}
				int divisor = e.first->type == AEquipmentType::Type::Ammo ? e.first->max_ammo : 1;
				int maxAmount = config().getBool("OpenApoc.NewFeature.EnforceCargoLimits")
				                    ? std::min(e.second, (v->getMaxBio() - v->getBio()) /
				                                             e.first->store_space * divisor)
				                    : e.second;
				if (maxAmount > 0)
				{
					e.second -= maxAmount;
					int price = 0;
					v->cargo.emplace_back(state, e.first, maxAmount, price, nullptr,
					                      v->homeBuilding);
					returningVehicles.insert(v);
				}
			}
			if (e.second == 0)
			{
				bioLootToRemove.push_back(e.first);
			}
		}
		// Remove stored loot
		for (auto &e : bioLootToRemove)
		{
			state.current_battle->bioLoot.erase(e);
		}
		// Put remainder on first vehicle
		for (auto &e : state.current_battle->bioLoot)
		{
			for (auto &v : playerVehicles)
			{
				if (v->getMaxCargo() == 0)
				{
					continue;
				}
				int maxAmount = e.second;
				if (maxAmount > 0)
				{
					e.second -= maxAmount;
					int price = 0;
					v->cargo.emplace_back(state, e.first, maxAmount, price, nullptr,
					                      v->homeBuilding);
					returningVehicles.insert(v);
				}
			}
		}
	}

	// Give player vehicle a null cargo just so it comes back to base once
	for (auto v : returningVehicles)
	{
		v->cargo.emplace_front(
		    state, StateRef<AEquipmentType>(&state, state.agent_equipment.begin()->first), 0, 0,
		    nullptr, v->homeBuilding);
		if (v->city.id == "CITYMAP_HUMAN")
		{
			v->setMission(state, VehicleMission::gotoBuilding(state, *v));
			v->addMission(state, VehicleMission::offerService(state, *v), true);
		}
		else
		{
			v->setMission(state, VehicleMission::gotoPortal(state, *v));
		}
	}

	// Event and result
	// FIXME: IS there a better way to pass events? They get cleared if we just pushEvent() them!
	bool victory = false;
	switch (state.current_battle->mission_type)
	{
		case Battle::MissionType::RaidAliens:
		{
			if (state.current_battle->playerWon)
			{
				state.eventFromBattle = GameEventType::MissionCompletedBuildingAlien;
				auto building =
				    StateRef<Building>(&state, state.current_battle->mission_location_id);
				for (auto &u : building->researchUnlock)
				{
					u->forceComplete();
				}
				victory = building->victory;
				building->collapse(state);
				for (auto v : returningVehicles)
				{
					v->addMission(state, VehicleMission::snooze(state, *v, 3 * TICKS_PER_SECOND));
				}
			}
			break;
		}
		case Battle::MissionType::AlienExtermination:
		{
			state.eventFromBattle = GameEventType::MissionCompletedBuildingNormal;
			state.missionLocationBattle = state.current_battle->mission_location_id;
			break;
		}
		case Battle::MissionType::BaseDefense:
		{
			if (state.current_battle->playerWon)
			{
				state.eventFromBattle = GameEventType::MissionCompletedBase;
				state.missionLocationBattle = state.current_battle->mission_location_id;
			}
			else
			{
				auto building =
				    StateRef<Building>{&state, state.current_battle->mission_location_id};
				state.eventFromBattle = GameEventType::BaseDestroyed;
				state.missionLocationBattle = state.current_battle->mission_location_id;
				state.eventFromBattleText = building->base->name;
				building->base->die(state, false);
			}
			break;
		}
		case Battle::MissionType::RaidHumans:
		{
			state.eventFromBattle = GameEventType::MissionCompletedBuildingRaid;
			state.missionLocationBattle = state.current_battle->mission_location_id;
			if (state.current_battle->playerWon)
			{
				auto building =
				    StateRef<Building>(&state, state.current_battle->mission_location_id);
				for (auto &u : building->researchUnlock)
				{
					u->forceComplete();
				}
				victory = building->victory;
				if (config().getBool("OpenApoc.NewFeature.CollapseRaidedBuilding"))
				{
					building->collapse(state);
					for (auto v : returningVehicles)
					{
						v->addMission(state,
						              VehicleMission::snooze(state, *v, 3 * TICKS_PER_SECOND));
					}
				}
			}
			break;
		}
		case Battle::MissionType::UfoRecovery:
		{
			state.eventFromBattle = GameEventType::MissionCompletedVehicle;
			auto vehicle = StateRef<Vehicle>(&state, state.current_battle->mission_location_id);
			for (auto &u : vehicle->type->researchUnlock)
			{
				u->forceComplete();
			}
			vehicle->die(state, true);
			break;
		}
	}

	if (victory)
	{
		LogError("You won, but we have no screen for that yet LOL!");
	}

	state.current_battle = nullptr;

	// Remove all dead vehicles from the state
	state.cleanUpDeathNote();
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

	for (auto &o : participants)
	{
		for (auto &t : o->guard_types_reinforcements)
		{
			{
				auto packName = BattleUnitImagePack::getNameFromID(t->shadow_pack.id);
				if (imagePacks.find(packName) == imagePacks.end())
					imagePacks.insert(packName);
			}
			for (auto &pv : t->image_packs)
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
		}
	}

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
			if (!brainsuckerFound && ae->getPayloadType() && ae->getPayloadType()->damage_type &&
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
	for (auto &o : participants)
	{
		for (auto &t : o->guard_types_reinforcements)
		{
			for (auto &ap : t->animation_packs)
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
		}
	}
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
				if (e->getPayloadType() && e->getPayloadType()->damage_type &&
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

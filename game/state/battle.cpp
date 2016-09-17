#include "game/state/battle.h"
#include "framework/framework.h"
#include "framework/trace.h"
#include "game/state/battle/battleitem.h"
#include "game/state/battlemap.h"
#include "game/state/battle/battleunit.h"
#include "game/state/battlemappart.h"
#include "game/state/battlemappart_type.h"
#include "game/state/city/city.h"
#include "game/state/city/vehicle.h"
#include "game/state/city/doodad.h"
#include "game/state/city/projectile.h"
#include "game/state/gamestate.h"
#include "game/state/tileview/collision.h"
#include "game/state/tileview/tileobject_battleitem.h"
#include "game/state/tileview/tileobject_battleunit.h"
#include "game/state/tileview/tileobject_battlemappart.h"
#include "game/state/tileview/tileobject_doodad.h"
#include "game/state/tileview/tileobject_projectile.h"
#include "game/state/tileview/tileobject_shadow.h"
#include <functional>
#include <future>
#include <limits>
#include <unordered_map>

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
	}
	// FIXME: Due to tiles possibly being cross-supported we need to clear that sp<> to avoid leaks
	// Should this be pushed into a weak_ptr<> or some other ref?
	for (auto s : this->map_parts)
	{
		s->supportedParts.clear();
		s->supportedBy.clear();
	}
	this->map_parts.clear();
	for (auto &s : this->items)
	{
		if (s->tileObject)
			s->tileObject->removeFromMap();
		if (s->shadowObject)
			s->shadowObject->removeFromMap();
		s->tileObject = nullptr;
	}
	this->items.clear();
}

void Battle::initBattle(GameState &state)
{
	LoadResources(state);
	for (auto &s : this->map_parts)
	{
		s->battle = shared_from_this();
	}
	for (auto &o : this->items)
	{
		o->battle = shared_from_this();
		o->item->ownerItem = o->shared_from_this();
		o->strategy_icon_list = state.battle_strategy_icon_list;
	}
	for (auto &o : this->units)
	{
		o->battle = shared_from_this();
		o->strategy_icon_list = state.battle_strategy_icon_list;
	}
	if (forces.size() == 0)
	{
		// Init forces and fill squads with nullptrs so that we have where to place units
		for (auto &o : this->participants)
		{
			forces[o];
			for (int i = 0;i < 6;i++)
			{
				forces[o].squads[i].units = std::vector<sp<BattleUnit>>(6);
			}
		}
		// Place units into squads directly to their positions
		for (auto &u : this->units)
		{
			forces[u->owner].squads[u->squadNumber].units[u->squadPosition] = u;
		}
		// Trim nullptrs from squad units
		for (auto &o : this->participants)
		{
			for (int i = 0;i < 6;i++)
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
	                            {BATTLE_VOXEL_X, BATTLE_VOXEL_Y, BATTLE_VOXEL_Z}, layerMap));
	for (auto &s : this->map_parts)
	{
		this->map->addObjectToMap(s);
	}
	for (auto &o : this->items)
	{
		this->map->addObjectToMap(o);
	}
	for (auto &u : this->units)
	{
		this->map->addObjectToMap(u);
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

void Battle::update(GameState &state, unsigned int ticks)
{
	TRACE_FN_ARGS1("ticks", Strings::fromInteger(static_cast<int>(ticks)));

	Trace::start("Battle::update::projectiles->update");
	std::list<std::future<Collision>> collisions;
	for (auto it = this->projectiles.begin(); it != this->projectiles.end();)
	{
		auto p = *it++;
		p->update(state, ticks);
	}
	for (auto &p : this->projectiles)
	{
		auto func = std::bind(&Projectile::checkProjectileCollision, p, std::placeholders::_1);
		collisions.emplace_back(fw().threadPool->enqueue(func, std::ref(*map)));
	}
	for (auto &future : collisions)
	{
		// Make sure every user of the TileMap is finished before processing (as the tileobject/map
		// lists are not locked, so probably OK for read-only...)
		future.wait();
	}
	for (auto &future : collisions)
	{

		auto c = future.get();
		if (c)
		{
			// FIXME: Handle collision
			this->projectiles.erase(c.projectile);
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

					auto doodad = this->placeDoodad({&state, "DOODAD_EXPLOSION_2"},
					                                mapPartTile->getPosition());
					mapPartTile->getOwner()->handleCollision(state, c);
					break;
				}
				default:
					LogError("Collision with non-collidable object");
			}
		}
	}
	Trace::end("Battle::update::projectiles->update");
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
		o->update(state, ticks);
	}
	Trace::end("Battle::update::units->update");
	Trace::start("Battle::update::doodads->update");
	for (auto it = this->doodads.begin(); it != this->doodads.end();)
	{
		auto d = *it++;
		d->update(state, ticks);
	}
}

// To be called when battle must be started, before showing battle briefing screen
void Battle::EnterBattle(GameState &state, StateRef<Organisation> target_organisation,
	std::list<StateRef<Agent>> &player_agents,
	StateRef<Vehicle> player_craft, StateRef<Vehicle> target_craft)
{
	if (state.current_battle)
	{
		LogError("Battle::EnterBattle called while another battle is in progress!");
		return;
	}
	auto b = BattleMap::CreateBattle(state, target_organisation, player_agents, player_craft, target_craft);
	if (!b)
		return;
	state.current_battle = b;
}

// To be called when battle must be started, before showing battle briefing screen
void Battle::EnterBattle(GameState &state, StateRef<Organisation> target_organisation,
	std::list<StateRef<Agent>> &player_agents,
	StateRef<Vehicle> player_craft, StateRef<Building> target_building)
{
	if (state.current_battle)
	{
		LogError("Battle::EnterBattle called while another battle is in progress!");
		return;
	}
	auto b = BattleMap::CreateBattle(state, target_organisation, player_agents, player_craft, target_building);
	if (!b)
		return;
	state.current_battle = b;
}

// To be called when battle must be started, after the player has assigned agents to squads
void Battle::BeginBattle(GameState &state)
{
	if (!state.current_battle)
	{
		LogError("Battle::BeginBattle called with no battle!");
		return;
	}

	auto &b = state.current_battle;

	for (auto &o : b->participants)
	{
		// Create spawn maps
		std::list<sp<BattleMapSector::LineOfSightBlock>> spawnMapWalker;
		std::list<sp<BattleMapSector::LineOfSightBlock>> spawnMapFlying;
		std::list<sp<BattleMapSector::LineOfSightBlock>> spawnMapLargeFlying;
		std::list<sp<BattleMapSector::LineOfSightBlock>> spawnMapLargeWalker;

		// Fill spawn maps
		
		// FIXME: 
		// Choose side based on what org it is
		// For each LOS block that belongs to side, add it to list several number of times (based on priority)

		// Spawn agents
		for (auto &f : state.current_battle->forces)
		{
			for (auto &s : f.second.squads)
			{
				if (s.getNumUnits() == 0)
					continue;

				// FIXME:
				// Find out what units are in the squad.
				// Pick a spawn map
				// Do a list randomiser on the spawn map, picking a block
				// Spawn units within a block
			}
		}
	}

	state.current_battle->initBattle(state);
}

// To be called when battle must be finished and before showing score screen
void Battle::FinishBattle(GameState &state)
{
	if (!state.current_battle)
	{
		LogError("Battle::FinishBattle called with no battle!");
		return;
	}
	//  - Identify how battle ended(if enemies present then Failure, otherwise Success)
	//	- (Failure) Determine surviving player agents(kill all player agents that are too far from exits)
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
void Battle::ExitBattle(GameState &state)
{
	if (!state.current_battle)
	{
		LogError("Battle::ExitBattle called with no battle!");
		return;
	}

	state.current_battle->UnloadResources(state);
	//  - Apply score
	//	- (UFO mission) Clear UFO crash
	//	- Load loot into vehicles
	//	- Load aliens into bio - trans
	//	- Put surviving aliens back in the building (?or somewhere else if UFO?)
	
	state.current_battle = nullptr;
}

void Battle::LoadResources(GameState &state)
{
	battle_map->LoadTilesets(state);
	LoadImagePacks(state);
	LoadAnimationPacks(state);
}

void Battle::UnloadResources(GameState &state)
{
	BattleMap::UnloadTilesets(state);
	UnloadImagePacks(state);
	UnloadAnimationPacks(state);
}

void Battle::LoadImagePacks(GameState &state)
{
	if (state.battle_unit_image_packs.size() > 0)
	{
		LogInfo("Image packs are already loaded.");
		return;
	}
	// Find out all image packs used by map's units and items
	std::set<UString> imagePacks;
	for (auto &bu : units)
	{
		if (bu->agent->type->shadow_pack)
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
			if (ae->type->image_pack)
			{
				auto packName = BattleUnitImagePack::getNameFromID(ae->type->image_pack.id);
				if (imagePacks.find(packName) == imagePacks.end())
					imagePacks.insert(packName);
			}
		}
	}
	for (auto &bi : items)
	{
		if (bi->item->type->image_pack)
		{
			auto packName = BattleUnitImagePack::getNameFromID(bi->item->type->image_pack.id);
			if (imagePacks.find(packName) == imagePacks.end())
				imagePacks.insert(packName);
		}
	}
	// Load all used image packs
	for (auto &imagePackName : imagePacks)
	{
		unsigned count = 0;
		auto imagePackPath = BattleUnitImagePack::imagePackPath + "/" + imagePackName;
		LogInfo("Loading image pack \"%s\" from \"%s\"", imagePackName.cStr(), imagePackPath.cStr());
		auto imagePack = mksp<BattleUnitImagePack>();
		if (!imagePack->loadImagePack(state, imagePackPath))
		{
			LogError("Failed to load image pack \"%s\" from \"%s\"", imagePackName.cStr(), imagePackPath.cStr());
			continue;
		}
		state.battle_unit_image_packs[UString::format("%s%s", BattleUnitImagePack::getPrefix(), imagePackName)] = imagePack;
		LogInfo("Loaded image pack \"%s\" from \"%s\"", imagePackName.cStr(), imagePackPath.cStr());
	}
}

void Battle::UnloadImagePacks(GameState &state)
{
	state.battle_unit_image_packs.clear();
	LogInfo("Unloaded all image packs.");
}

void Battle::LoadAnimationPacks(GameState &state)
{
	if (state.battle_unit_animation_packs.size() > 0)
	{
		LogInfo("Animation packs are already loaded.");
		return;
	}
	// Find out all animation packs used by units
	std::set<UString> animationPacks;
	for (auto &bu : units)
	{
		for (auto &ap : bu->agent->type->animation_packs)
		{
			auto packName = BattleUnitAnimationPack::getNameFromID(ap.id);
			if (animationPacks.find(packName) == animationPacks.end())
				animationPacks.insert(packName);
		}
	}
	// Load all used animation packs
	for (auto &animationPackName : animationPacks)
	{
		unsigned count = 0;
		auto animationPackPath = BattleUnitAnimationPack::animationPackPath + "/" + animationPackName;
		LogInfo("Loading animation pack \"%s\" from \"%s\"", animationPackName.cStr(), animationPackPath.cStr());
		auto animationPack = mksp<BattleUnitAnimationPack>();
		if (!animationPack->loadAnimationPack(state, animationPackPath))
		{
			LogError("Failed to load animation pack \"%s\" from \"%s\"", animationPackName.cStr(), animationPackPath.cStr());
			continue;
		}
		state.battle_unit_animation_packs[UString::format("%s%s", BattleUnitAnimationPack::getPrefix(), animationPackName)] = animationPack;
		LogInfo("Loaded animation pack \"%s\" from \"%s\"", animationPackName.cStr(), animationPackPath.cStr());
	}
}

void Battle::UnloadAnimationPacks(GameState &state)
{
	state.battle_unit_animation_packs.clear();
	LogInfo("Unloaded all animation packs.");
}

} // namespace OpenApoc

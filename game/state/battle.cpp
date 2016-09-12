#include "game/state/battle.h"
#include "framework/framework.h"
#include "framework/trace.h"
#include "game/state/battle/battleitem.h"
#include "game/state/battle/battleitem.h"
#include "game/state/battle/battleunit.h"
#include "game/state/battlemappart.h"
#include "game/state/battlemappart_type.h"
#include "game/state/city/city.h"
#include "game/state/city/doodad.h"
#include "game/state/city/projectile.h"
#include "game/state/gamestate.h"
#include "game/state/tileview/collision.h"
#include "game/state/tileview/tileobject_battleitem.h"
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

// FIXME: Bullshit, need to make it properly but it won't work with shared_from_this
void Battle::initBattle()
{
	for (auto &s : this->map_parts)
	{
		s->battle = shared_from_this();
	}
	for (auto &o : this->items)
	{
		o->battle = shared_from_this();
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
	for (auto &o : this->units)
	{
		std::ignore = o;
		// this->map->addObjectToMap(o);
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
				/*case TileObject::Type::Vehicle:
				{
				    auto vehicle = std::static_pointer_cast<TileObjectVehicle>(c.obj)->getVehicle();
				    vehicle->handleCollision(state, c);
				    LogWarning("Vehicle collision");
				    break;
				}*/
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
		std::ignore = o;
		// o->update(state, ticks);
	}
	Trace::end("Battle::update::units->update");
	Trace::start("Battle::update::doodads->update");
	for (auto it = this->doodads.begin(); it != this->doodads.end();)
	{
		auto d = *it++;
		d->update(state, ticks);
	}
}

} // namespace OpenApoc

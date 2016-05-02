#include "game/state/city/city.h"
#include "framework/framework.h"
#include "framework/trace.h"
#include "game/state/city/building.h"
#include "game/state/city/doodad.h"
#include "game/state/city/projectile.h"
#include "game/state/city/scenery.h"
#include "game/state/city/vehicle.h"
#include "game/state/city/vehiclemission.h"
#include "game/state/city/vequipment.h"
#include "game/state/gamestate.h"
#include "game/state/rules/scenery_tile_type.h"
#include "game/state/rules/vequipment.h"
#include "game/state/tileview/tileobject_projectile.h"
#include "game/state/tileview/tileobject_scenery.h"
#include "game/state/tileview/voxel.h"

#include <functional>
#include <future>
#include <limits>

namespace OpenApoc
{

// An ordered list of the types drawn in each layer
// Within the same layer these are ordered by a calculated z based on the 'center' position
static std::vector<std::set<TileObject::Type>> layerMap = {
    // Draw all scenery first, then put stuff on top of that
    {TileObject::Type::Scenery, TileObject::Type::Doodad},
    {TileObject::Type::Projectile, TileObject::Type::Vehicle, TileObject::Type::Shadow},
};

City::~City()
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
	for (auto &s : this->scenery)
	{
		if (s->tileObject)
			s->tileObject->removeFromMap();
		s->tileObject = nullptr;
	}
	// FIXME: Due to tiles possibly being cross-supported we need to clear that sp<> to avoid leaks
	// Should this be pushed into a weak_ptr<> or some other ref?
	for (auto s : this->scenery)
	{
		s->supports.clear();
		s->supportedBy.clear();
	}

	for (auto &b : this->buildings)
	{
		b.second->landed_vehicles.clear();
	}
}

void City::initMap()
{
	if (this->map)
	{
		LogError("Called on city with existing map");
		return;
	}
	this->map.reset(new TileMap(this->size, layerMap));
	for (auto &s : this->scenery)
	{
		this->map->addObjectToMap(s);
		if (s->type->isLandingPad)
		{
			Vec2<int> pos = {s->initialPosition.x, s->initialPosition.y};

			for (auto &b : this->buildings)
			{
				if (b.second->bounds.within(pos))
				{
					b.second->landingPadLocations.push_back(s->initialPosition);
					LogInfo("Pad {%d,%d} is within building %s {%d,%d},{%d,%d}", pos.x, pos.y,
					        b.first.c_str(), b.second->bounds.p0.x, b.second->bounds.p0.y,
					        b.second->bounds.p1.x, b.second->bounds.p1.y);
				}
			}
		}
	}
	for (auto &b : this->buildings)
	{
		if (b.second->landingPadLocations.empty())
		{
			LogError("Building %s has no landing pads", b.first.c_str());
		}
		LogInfo("Building %s has %u landing pads:", b.first.c_str(),
		        (unsigned)b.second->landingPadLocations.size());

		for (auto &loc : b.second->landingPadLocations)
		{
			LogInfo("Pad: {%d,%d,%d}", loc.x, loc.y, loc.z);
		}
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

void City::update(GameState &state, unsigned int ticks)
{
	TRACE_FN_ARGS1("ticks", Strings::FromInteger(static_cast<int>(ticks)));
	/* FIXME: Temporary 'get something working' HACK
	 * Every now and then give a landed vehicle a new 'goto random building' mission, so there's
	 * some activity in the city*/
	std::uniform_int_distribution<int> bld_distribution(0, this->buildings.size() - 1);

	// Need to use a 'safe' iterator method (IE keep the next it before calling ->update)
	// as update() calls can erase it's object from the lists

	Trace::start("City::update::buildings->landed_vehicles");
	for (auto it = this->buildings.begin(); it != this->buildings.end();)
	{
		auto b = it->second;
		it++;
		for (auto v : b->landed_vehicles)
		{
			for (auto &e : v->equipment)
			{
				if (e->type->type != VEquipmentType::Type::Weapon)
					continue;
				e->reload(std::numeric_limits<int>::max());
			}
			if (v->owner == state.getPlayer())
				continue;
			if (v->missions.empty())
			{
				auto bldIt = this->buildings.begin();
				auto count = bld_distribution(state.rng);
				while (count--)
					bldIt++;
				StateRef<Building> dest = {&state, bldIt->first};
				v->missions.emplace_back(VehicleMission::gotoBuilding(*v, dest));
				v->missions.front()->start(state, *v);

				// FIXME: Make snoozetime bounds/distribution readable from serialized GameState
				std::uniform_int_distribution<unsigned int> snoozeTimeDist(10, 10000);
				v->missions.emplace_back(VehicleMission::snooze(*v, snoozeTimeDist(state.rng)));
			}
		}
	}
	Trace::end("City::update::buildings->landed_vehicles");
	Trace::start("City::update::projectiles->update");
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
				case TileObject::Type::Vehicle:
				{
					LogWarning("Vehicle collision");
					break;
				}
				case TileObject::Type::Scenery:
				{
					auto sceneryTile = std::static_pointer_cast<TileObjectScenery>(c.obj);
					// FIXME: Don't just explode scenery, but damaged tiles/falling stuff? Different
					// explosion doodads? Not all weapons instantly destory buildings too

					auto doodad = this->placeDoodad({&state, "DOODAD_EXPLOSION_2"},
					                                sceneryTile->getPosition());
					sceneryTile->getOwner()->handleCollision(state, c);
					break;
				}
				default:
					LogError("Collision with non-collidable object");
			}
		}
	}
	Trace::end("City::update::projectiles->update");
	Trace::start("City::update::scenery->update");
	for (auto &s : this->scenery)
	{
		s->update(state, ticks);
	}
	Trace::end("City::update::scenery->update");
	Trace::start("City::update::doodads->update");
	for (auto it = this->doodads.begin(); it != this->doodads.end();)
	{
		auto d = *it++;
		d->update(state, ticks);
	}
	Trace::end("City::update::doodads->update");
}

sp<Doodad> City::placeDoodad(StateRef<DoodadType> type, Vec3<float> position)
{
	auto doodad = mksp<Doodad>(position, type);
	map->addObjectToMap(doodad);
	this->doodads.insert(doodad);
	return doodad;
}

template <> sp<City> StateObject<City>::get(const GameState &state, const UString &id)
{
	auto it = state.cities.find(id);
	if (it == state.cities.end())
	{
		LogError("No citymap matching ID \"%s\"", id.c_str());
		return nullptr;
	}
	return it->second;
}

template <> const UString &StateObject<City>::getPrefix()
{
	static UString prefix = "CITYMAP_";
	return prefix;
}
template <> const UString &StateObject<City>::getTypeName()
{
	static UString name = "City";
	return name;
}

template <> const UString &StateObject<City>::getId(const GameState &state, const sp<City> ptr)
{
	static const UString emptyString = "";
	for (auto &c : state.cities)
	{
		if (c.second == ptr)
		{
			return c.first;
		}
	}
	LogError("No city matching pointer %p", ptr.get());
	return emptyString;
}

} // namespace OpenApoc

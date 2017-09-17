#include "game/state/city/city.h"
#include "framework/framework.h"
#include "framework/sound.h"
#include "framework/trace.h"
#include "game/state/city/building.h"
#include "game/state/city/doodad.h"
#include "game/state/city/projectile.h"
#include "game/state/city/scenery.h"
#include "game/state/city/vehicle.h"
#include "game/state/city/vehiclemission.h"
#include "game/state/city/vequipment.h"
#include "game/state/gamestate.h"
#include "game/state/rules/doodad_type.h"
#include "game/state/rules/scenery_tile_type.h"
#include "game/state/rules/vequipment_type.h"
#include "game/state/tileview/collision.h"
#include "game/state/tileview/tile.h"
#include "game/state/tileview/tileobject_projectile.h"
#include "game/state/tileview/tileobject_scenery.h"
#include "game/state/tileview/tileobject_vehicle.h"
#include <functional>
#include <future>
#include <glm/glm.hpp>
#include <limits>

namespace OpenApoc
{

// An ordered list of the types drawn in each layer
// Within the same layer these are ordered by a calculated z based on the 'center' position
static std::vector<std::set<TileObject::Type>> layerMap = {
    // Draw all scenery first, then put stuff on top of that
    {TileObject::Type::Scenery, TileObject::Type::Doodad, TileObject::Type::Vehicle},
    {TileObject::Type::Projectile, TileObject::Type::Shadow},
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
	for (auto &s : this->scenery)
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
	this->map.reset(new TileMap(this->size, VELOCITY_SCALE_CITY,
	                            {VOXEL_X_CITY, VOXEL_Y_CITY, VOXEL_Z_CITY}, layerMap));
	for (auto &s : this->scenery)
	{
		// FIXME: Should we really add all scenery to the map? What if it's destroyed?
		this->map->addObjectToMap(s);
		if (s->type->isLandingPad)
		{
			Vec2<int> pos = {s->initialPosition.x, s->initialPosition.y};

			for (auto &b : this->buildings)
			{
				if (b.second->bounds.within(pos))
				{
					b.second->landingPadLocations.push_back(s->initialPosition);
					LogInfo("Pad %s is within building %s bounds %s", pos, b.first,
					        b.second->bounds);
				}
			}
		}
	}
	for (auto &b : this->buildings)
	{
		if (b.second->landingPadLocations.empty())
		{
			LogError("Building %s has no landing pads", b.first);
		}
		LogInfo("Building %s has %u landing pads:", b.first,
		        (unsigned)b.second->landingPadLocations.size());

		for (auto &loc : b.second->landingPadLocations)
		{
			LogInfo("Pad: %s", loc);
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
	for (auto &p : this->portals)
	{
		this->map->addObjectToMap(p);
	}
}

void City::update(GameState &state, unsigned int ticks)
{
	TRACE_FN_ARGS1("ticks", Strings::fromInteger(static_cast<int>(ticks)));
	/* FIXME: Temporary 'get something working' HACK
	 * Every now and then give a landed vehicle a new 'goto random building' mission, so there's
	 * some activity in the city*/
	std::uniform_int_distribution<int> bld_distribution(0, (int)this->buildings.size() - 1);

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
				if (e->type->type != EquipmentSlotType::VehicleWeapon)
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
				v->missions.emplace_back(VehicleMission::gotoBuilding(state, *v, dest));
				v->missions.front()->start(state, *v);

				// FIXME: Make snoozetime bounds/distribution readable from serialized GameState
				std::uniform_int_distribution<unsigned int> snoozeTimeDist(10, 10000);
				v->missions.emplace_back(
				    VehicleMission::snooze(state, *v, snoozeTimeDist(state.rng)));
			}
		}
	}
	Trace::end("City::update::buildings->landed_vehicles");
	Trace::start("City::update::projectiles->update");
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

			auto doodadType = c.projectile->doodadType;
			if (doodadType)
			{
				auto doodad = this->placeDoodad(doodadType, c.position);
			}

			switch (c.obj->getType())
			{
				case TileObject::Type::Vehicle:
				{
					auto vehicle = std::static_pointer_cast<TileObjectVehicle>(c.obj)->getVehicle();
					vehicle->handleCollision(state, c);
					LogWarning("Vehicle collision");
					break;
				}
				case TileObject::Type::Scenery:
				{
					auto sceneryTile = std::static_pointer_cast<TileObjectScenery>(c.obj);
					// FIXME: Don't just explode scenery, but damaged tiles/falling stuff? Different
					// explosion doodads? Not all weapons instantly destory buildings too

					auto doodad =
					    this->placeDoodad({&state, "DOODAD_3_EXPLOSION"}, sceneryTile->getCenter());
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

	for (auto it = this->portals.begin(); it != this->portals.end();)
	{
		auto p = *it++;
		p->update(state, ticks);
	}
	Trace::end("City::update::doodads->update");
}

void City::hourlyLoop(GameState &state) { updateInfiltration(state); }

void City::dailyLoop(GameState &state)
{
	// FIXME: Repair buildings, update stocks

	generatePortals(state);
}

void City::generatePortals(GameState &state)
{
	static const int iterLimit = 1000;
	for (auto &p : portals)
	{
		p->remove(state);
	}
	this->portals.clear();

	std::uniform_int_distribution<int> xyPos(10, 130);
	std::uniform_int_distribution<int> zPos(2, 8);
	for (int p = 0; p < 3; p++)
	{
		for (int i = 0; i < iterLimit; i++)
		{
			Vec3<float> pos(xyPos(state.rng), xyPos(state.rng), zPos(state.rng));

			if (map->tileIsValid(pos) && map->getTile(pos)->ownedObjects.empty())
			{
				auto doodad =
				    mksp<Doodad>(pos, StateRef<DoodadType>{&state, "DOODAD_6_DIMENSION_GATE"});
				map->addObjectToMap(doodad);
				this->portals.push_back(doodad);
				break;
			}
		}
	}
}

void City::updateInfiltration(GameState &state)
{
	for (auto &b : buildings)
	{
		b.second->alienGrowth(state);
	}
}

sp<Doodad> City::placeDoodad(StateRef<DoodadType> type, Vec3<float> position)
{
	auto doodad = mksp<Doodad>(position, type);
	map->addObjectToMap(doodad);
	this->doodads.push_back(doodad);
	return doodad;
}

sp<City> City::get(const GameState &state, const UString &id)
{
	auto it = state.cities.find(id);
	if (it == state.cities.end())
	{
		LogError("No citymap matching ID \"%s\"", id);
		return nullptr;
	}
	return it->second;
}

const UString &City::getPrefix()
{
	static UString prefix = "CITYMAP_";
	return prefix;
}
const UString &City::getTypeName()
{
	static UString name = "City";
	return name;
}

const UString &City::getId(const GameState &state, const sp<City> ptr)
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

void City::accuracyAlgorithmCity(GameState &state, Vec3<float> firePosition, Vec3<float> &target,
                                 int accuracy, bool cloaked)
{
	float dispersion = 100 - accuracy;
	// Introduce minimal dispersion?
	dispersion = std::max(0.0f, dispersion);

	if (cloaked)
	{
		dispersion *= dispersion;
		float cloakDispersion = 2000.0f / (glm::length(firePosition - target) + 3.0f);
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

	// Misses can go both ways on the axis
	auto diffVertical =
	    Vec3<float>{length_vector * delta.x * delta.z, length_vector * delta.y * delta.z,
	                -length_vector * (delta.x * delta.x + delta.y * delta.y)} *
	    k1;
	auto diffHorizontal = Vec3<float>{-delta.y, delta.x, 0.0f} * k2;
	auto diff = (diffVertical + diffHorizontal) * Vec3<float>{1.0f, 1.0f, 0.33f};

	target += diff;
}

} // namespace OpenApoc

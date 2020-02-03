#include "game/state/city/city.h"
#include "framework/framework.h"
#include "framework/sound.h"
#include "framework/trace.h"
#include "game/state/city/base.h"
#include "game/state/city/building.h"
#include "game/state/city/scenery.h"
#include "game/state/city/vehicle.h"
#include "game/state/city/vehiclemission.h"
#include "game/state/city/vequipment.h"
#include "game/state/gamestate.h"
#include "game/state/rules/city/citycommonimagelist.h"
#include "game/state/rules/city/scenerytiletype.h"
#include "game/state/rules/city/vequipmenttype.h"
#include "game/state/rules/doodadtype.h"
#include "game/state/shared/doodad.h"
#include "game/state/shared/projectile.h"
#include "game/state/tilemap/collision.h"
#include "game/state/tilemap/tilemap.h"
#include "game/state/tilemap/tileobject_projectile.h"
#include "game/state/tilemap/tileobject_scenery.h"
#include "game/state/tilemap/tileobject_vehicle.h"
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
		s->overlayDoodad = nullptr;
		s->city.clear();
		s->building.clear();
		s->clearSupportedParts();
	}
	for (auto &b : this->buildings)
	{
		b.second->currentVehicles.clear();
		b.second->currentAgents.clear();
		b.second->city.clear();
		b.second->base.clear();
	}
	for (auto &t : this->tile_types)
	{
		// Some damaged tile links can loop, causing a leak if they're not broken
		t.second->damagedTile.clear();
	}
}

void City::initMap(GameState &state)
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
		s->city = {&state, id};
		if (!s->destroyed)
		{
			this->map->addObjectToMap(s);
		}
		if (!s->building)
		{
			continue;
		}
		if (s->type->isLandingPad)
		{
			s->building->landingPadLocations.insert(s->initialPosition);
		}
		if ((s->type->connection[0] || s->type->connection[1] || s->type->connection[2] ||
		     s->type->connection[3]) &&
		    s->type->road_type == SceneryTileType::RoadType::Terminal)
		{
			if (s->building->carEntranceLocation.x != -1)
			{
				LogWarning("Building has multiple car entrances? %s", s->building->name);
			}
			s->building->carEntranceLocation = s->initialPosition;
			// crew quarters is the closest to camera spot with vehicle access
			if (s->initialPosition.z > s->building->crewQuarters.z ||
			    (s->initialPosition.z == s->building->crewQuarters.z &&
			     s->initialPosition.y > s->building->crewQuarters.y) ||
			    (s->initialPosition.z == s->building->crewQuarters.z &&
			     s->initialPosition.y == s->building->crewQuarters.y &&
			     s->initialPosition.x > s->building->crewQuarters.x))
			{
				s->building->crewQuarters = s->initialPosition;
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
		LogInfo("Car: %s", b.second->carEntranceLocation);
		if (b.second->crewQuarters == Vec3<int>{-1, -1, -1})
		{
			LogWarning("Building %s has no car exit?", b.first);
			b.second->crewQuarters = {(b.second->bounds.p0.x + b.second->bounds.p1.x) / 2,
			                          (b.second->bounds.p0.y + b.second->bounds.p1.y) / 2, 2};
		}
		LogInfo("Crew Quarters: %s", b.second->crewQuarters);
		if (b.second->function.id == "BUILDINGFUNCTION_SPACE_PORT")
		{
			spaceports.emplace_back(&state, b.first);
		}
		b.second->owner->buildings.emplace_back(&state, b.first);
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

int City::getRoadSegmentID(const Vec3<int> &position) const
{
	return tileToRoadSegmentMap.at(position.z * map->size.x * map->size.y +
	                               position.y * map->size.x + position.x);
}

const RoadSegment &City::getRoadSegment(const Vec3<int> &position) const
{
	return roadSegments.at(tileToRoadSegmentMap.at(position.z * map->size.x * map->size.y +
	                                               position.y * map->size.x + position.x));
}

void City::notifyRoadChange(const Vec3<int> &position, bool intact)
{
	auto segId = getRoadSegmentID(position);
	if (segId != -1)
	{
		roadSegments.at(segId).notifyRoadChange(position, intact);
	}
}

void City::handleProjectileHit(GameState &state, sp<Projectile> projectile, bool displayDoodad,
                               bool playSound, bool expired)
{
	if (displayDoodad && projectile->doodadType)
	{
		placeDoodad(projectile->doodadType, projectile->position);
	}

	if (playSound && projectile->impactSfx && (!expired || projectile->splitIntoTypesCity.empty()))
	{
		fw().soundBackend->playSample(projectile->impactSfx, projectile->position);
	}
	if (expired)
	{
		std::set<sp<Sample>> fireSounds;
		for (auto &p : projectile->splitIntoTypesCity)
		{
			auto direction = (float)randBoundsInclusive(state.rng, 0, 628) / 100.0f;
			auto velocity = glm::normalize(
			    VehicleType::directionToVector(VehicleType::getDirectionLarge(direction)));
			velocity *= p->speed * PROJECTILE_VELOCITY_MULTIPLIER;
			auto newProj = mksp<Projectile>(
			    p->guided ? Projectile::Type::Missile : Projectile::Type::Beam,
			    projectile->firerVehicle, projectile->trackedVehicle, projectile->targetPosition,
			    projectile->position, velocity, p->turn_rate, p->ttl, p->damage, 0, 0, p->tail_size,
			    p->projectile_sprites, p->impact_sfx, p->explosion_graphic,
			    state.city_common_image_list->projectileVoxelMap, p->stunTicks, p->splitIntoTypes,
			    projectile->manualFire);
			map->addObjectToMap(newProj);
			projectiles.insert(newProj);
			if (p->fire_sfx)
			{
				fireSounds.insert(p->fire_sfx);
			}
		}
		for (auto &s : fireSounds)
		{
			fw().soundBackend->playSample(s, projectile->position);
		}
	}
	projectiles.erase(projectile);
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

	Trace::start("City::update::projectiles->update");
	for (auto it = this->projectiles.begin(); it != this->projectiles.end();)
	{
		auto p = *it++;
		// Projectile can die here
		p->update(state, ticks);
	}
	// Since projectiles can kill projectiles just kill everyone in the end
	std::set<std::tuple<sp<Projectile>, bool, bool>> deadProjectiles;
	for (auto &p : projectiles)
	{
		auto c = p->checkProjectileCollision(*map);
		if (c)
		{
			bool displayDoodad = true;
			bool playSound = true;
			switch (c.obj->getType())
			{
				case TileObject::Type::Vehicle:
				{
					auto vehicle = std::static_pointer_cast<TileObjectVehicle>(c.obj)->getVehicle();
					displayDoodad = !vehicle->handleCollision(state, c, playSound);
					break;
				}
				case TileObject::Type::Scenery:
				{
					auto sceneryTile = std::static_pointer_cast<TileObjectScenery>(c.obj);
					displayDoodad = !sceneryTile->getOwner()->handleCollision(state, c);
					playSound = displayDoodad;
					break;
				}
				case TileObject::Type::Projectile:
				{
					deadProjectiles.emplace(
					    std::static_pointer_cast<TileObjectProjectile>(c.obj)->getProjectile(),
					    true, true);
					break;
				}
				default:
					LogError("Collision with non-collidable object");
			}
			deadProjectiles.emplace(c.projectile->shared_from_this(), displayDoodad, playSound);
		}
	}
	// Kill projectiles that collided
	for (auto &p : deadProjectiles)
	{
		std::get<0>(p)->die(state, std::get<1>(p), std::get<2>(p));
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

void City::hourlyLoop(GameState &state)
{
	repairVehicles(state);
	updateInfiltration(state);
}

void City::dailyLoop(GameState &state)
{
	// Alien city is never repaired
	if (state.cities["CITYMAP_ALIEN"] != shared_from_this())
	{
		repairScenery(state);
	}
	generatePortals(state);
}

void City::generatePortals(GameState &state)
{
	if (portals.empty())
	{
		if (!initial_portals.empty())
		{
			for (auto &p : initial_portals)
			{
				auto doodad = mksp<Doodad>((Vec3<float>)p + Vec3<float>{0.5f, 0.5f, 0.5f},
				                           StateRef<DoodadType>{&state, "DOODAD_6_DIMENSION_GATE"});
				doodad->voxelMap = state.city_common_image_list->portalVoxelMap;
				map->addObjectToMap(doodad);
				this->portals.push_back(doodad);
			}
			auto pos = pickRandom(state.rng, initial_portals);
			cityViewScreenCenter = pos;
		}
		else
		{
			// FIXME: Implement proper portals
			// According to skin36, portals must have empty 4x4x4 around them
			// and spawn within 100x100 around city center

			// FIXME: Implement portals in alien city staying where they are
			// and starting where they should

			static const int iterLimit = 1000;
			for (auto &p : portals)
			{
				p->remove(state);
			}
			this->portals.clear();

			std::uniform_int_distribution<int> xyPos(20, 120);
			std::uniform_int_distribution<int> zPos(2, 8);
			for (int p = 0; p < 3; p++)
			{
				for (int i = 0; i < iterLimit; i++)
				{
					Vec3<float> pos(xyPos(state.rng), xyPos(state.rng), zPos(state.rng));

					if (map->tileIsValid(pos) && map->getTile(pos)->ownedObjects.empty())
					{
						auto doodad =
						    mksp<Doodad>(pos + Vec3<float>{0.5f, 0.5f, 0.5f},
						                 StateRef<DoodadType>{&state, "DOODAD_6_DIMENSION_GATE"});
						doodad->voxelMap = state.city_common_image_list->portalVoxelMap;
						map->addObjectToMap(doodad);
						this->portals.push_back(doodad);
						break;
					}
				}
			}
		}
	}
	else
	{
		// FIXME: Implement moving portals
		// According to skin36, portal is moved by +-(2*week + 15) on each coordinate
		// and must stay within -5..105 which means within 15 from map border in our coords
	}
}

void City::updateInfiltration(GameState &state)
{
	for (auto &b : buildings)
	{
		b.second->alienGrowth(state);
	}
}

void City::repairScenery(GameState &state)
{
	// Step 01: Repair damaged scenery one by one
	std::list<sp<Scenery>> sceneryToUndamage;
	for (auto &s : scenery)
	{
		if (s->damaged && s->isAlive())
		{
			sceneryToUndamage.push_back(s);
		}
	}
	for (auto &s : sceneryToUndamage)
	{
		auto initialType = initial_tiles[s->initialPosition];
		auto owner = s->building && !initialType->commonProperty ? s->building->owner
		                                                         : state.getGovernment();
		if (owner->balance > initialType->value)
		{
			owner->balance -= initialType->value;
			s->damaged = false;
			s->type = initialType;
		}
	}
	// Step 02: Repair destroyed scenery
	std::set<sp<Scenery>> sceneryToRepair;
	for (auto &s : scenery)
	{
		if (!s->isAlive())
		{
			sceneryToRepair.insert(s);
		}
	}
	// Pick one scenery, add all scenery that must be repaired together, try to repair them
	while (!sceneryToRepair.empty())
	{
		std::set<sp<Scenery>> repairedTogether;
		std::set<Vec3<int>> addedPositions;
		auto nextScenery = *sceneryToRepair.begin();
		repairedTogether.insert(nextScenery);
		sceneryToRepair.erase(nextScenery);
		// Keep adding until we added everything
		bool addedMore = false;
		do
		{
			addedMore = false;
			for (auto &s : repairedTogether)
			{
				// Check if all supportedBy scenery is intact or added
				for (auto &p : s->supportedBy)
				{
					// If no scenery or dead
					auto support = map->getTile(p)->presentScenery;
					if (!support || !support->isAlive())
					{
						// If we haven't added it already
						if (addedPositions.find(p) == addedPositions.end())
						{
							// Need to find it by its initial position
							for (auto &deadScenery : scenery)
							{
								if (deadScenery->initialPosition == p)
								{
									repairedTogether.insert(deadScenery);
									break;
								}
							}
						}
					}
				} // for every supportedBy position
			}     // for every repairedTogether scenery
		} while (addedMore);
		// Try to repair all or none
		std::map<StateRef<Organisation>, int> repairCost;
		for (auto &deadScenery : repairedTogether)
		{
			auto initialType = initial_tiles[deadScenery->initialPosition];
			auto owner = deadScenery->building && !initialType->commonProperty
			                 ? deadScenery->building->owner
			                 : state.getGovernment();
			repairCost[owner] += initialType->value;
		}
		bool canAfford = true;
		for (auto &entry : repairCost)
		{
			if (entry.first->balance < entry.second)
			{
				canAfford = false;
				break;
			}
		}
		if (canAfford)
		{
			// pay
			for (auto &entry : repairCost)
			{
				auto org = entry.first;
				org->balance -= entry.second;
			}
			// repair
			for (auto &deadScenery : repairedTogether)
			{
				// remove from scenery to repair
				if (sceneryToRepair.find(deadScenery) != sceneryToRepair.end())
				{
					sceneryToRepair.erase(deadScenery);
				}
				// repair actually
				deadScenery->repair(state);
			}
		}
	}
}

void City::repairVehicles(GameState &state [[maybe_unused]])
{
	for (auto &b : buildings)
	{
		// Players get repaired according to facilities
		if (b.second->base)
		{
			int repairPoints = b.second->base->getCapacityTotal(FacilityType::Capacity::Repair);
			std::list<StateRef<Vehicle>> vehiclesToRepair;
			for (auto &v : b.second->currentVehicles)
			{
				if (v->homeBuilding == v->currentBuilding && v->getHealth() < v->getMaxHealth())
				{
					vehiclesToRepair.push_back(v);
				}
			}
			if (!vehiclesToRepair.empty())
			{
				int repairPerVehicle = std::max(1, repairPoints / (int)vehiclesToRepair.size());
				// Twice since we can have a situation like 1 repair bay and 7 vehicles,
				// in this case we repair them for 1 and we have 5 points remaining
				// which we assign again
				for (int i = 0; i < 2; i++)
				{
					for (auto &v : vehiclesToRepair)
					{
						if (repairPoints == 0)
						{
							break;
						}
						int repair = std::min(std::min(repairPoints, repairPerVehicle),
						                      v->getMaxHealth() - v->getHealth());
						auto veh = v;
						veh->health += repair;
						repairPoints -= repair;
					}
				}
			}
		}
		// Corps get repaired by 12 flat
		else
		{
			for (auto &v : b.second->currentVehicles)
			{
				if (v->homeBuilding == v->currentBuilding)
				{
					int repair = std::min(12, v->getMaxHealth() - v->getHealth());
					auto veh = v;
					veh->health += repair;
				}
			}
		}
	}
}

void City::initialSceneryLinkUp()
{
	LogWarning("Begun scenery link up!");
	auto &mapref = *map;

	for (auto &s : this->scenery)
	{
		if (!s->destroyed)
		{
			s->queueCollapse();
		}
	}

	for (int z = 0; z < mapref.size.z; z++)
	{
		for (auto &s : this->scenery)
		{
			if ((int)s->currentPosition.z == z && !s->destroyed && s->findSupport())
			{
				s->cancelCollapse();
			}
		}
	}
	LogWarning("Begun scenery link up cycle!");
	bool foundSupport;
	// First support without clinging to establish proper links
	do
	{
		foundSupport = false;
		for (auto &s : this->scenery)
		{
			if (!s->willCollapse())
			{
				continue;
			}
			if (s->findSupport(false))
			{
				s->cancelCollapse();
				foundSupport = true;
			}
		}
	} while (foundSupport);
	// Then cling remaining items
	do
	{
		foundSupport = false;
		for (auto &s : this->scenery)
		{
			if (!s->willCollapse())
			{
				continue;
			}
			if (s->findSupport(true))
			{
				s->cancelCollapse();
				foundSupport = true;
			}
		}
	} while (foundSupport);

	// Report unlinked parts
	for (auto &mp : this->scenery)
	{
		if (mp->willCollapse())
		{
			auto pos = mp->tileObject->getOwningTile()->position;
			LogWarning("SC %s at %s is UNLINKED", mp->type.id, pos);
		}
	}

	LogWarning("Attempting link up of unlinked parts");
	do
	{
		foundSupport = false;
		for (auto &s : this->scenery)
		{
			if (!s->willCollapse())
			{
				continue;
			}
			if (s->attachToSomething())
			{
				s->cancelCollapse();
				foundSupport = true;
			}
		}
	} while (foundSupport);

	// Report unlinked parts
	for (auto &mp : this->scenery)
	{
		if (mp->willCollapse())
		{
			auto pos = mp->tileObject->getOwningTile()->position;
			LogWarning("SC %s at %s is going to fall", mp->type.id, pos);
		}
	}

	mapref.updateAllCityInfo();
	LogWarning("Link up finished!");
}

sp<Doodad> City::placeDoodad(StateRef<DoodadType> type, Vec3<float> position)
{
	auto doodad = mksp<Doodad>(position, type);
	map->addObjectToMap(doodad);
	this->doodads.push_back(doodad);
	return doodad;
}

sp<Vehicle> City::createVehicle(GameState &state, StateRef<VehicleType> type,
                                StateRef<Organisation> owner)
{
	auto v = mksp<Vehicle>();
	v->type = type;
	v->name = format("%s %d", type->name, ++type->numCreated);
	v->city = {&state, id};
	v->owner = owner;
	v->health = type->health;
	v->strategyImages = state.city_common_image_list->strategyImages;
	v->owner = owner;
	v->setupMover();

	// Vehicle::equipDefaultEquipment uses the state reference from itself, so make sure the
	// vehicle table has the entry before calling it
	UString vID = Vehicle::generateObjectID(state);
	state.vehicles[vID] = v;

	return v;
}
sp<Vehicle> City::createVehicle(GameState &state, StateRef<VehicleType> type,
                                StateRef<Organisation> owner, StateRef<Building> building)
{
	if (building->city.id != id)
	{
		LogError("Adding vehicle to a building in a different city?");
		return nullptr;
	}
	auto v = createVehicle(state, type, owner);

	v->enterBuilding(state, building);

	return v;
}

sp<Vehicle> City::placeVehicle(GameState &state, StateRef<VehicleType> type,
                               StateRef<Organisation> owner)
{
	auto v = createVehicle(state, type, owner);
	v->equipDefaultEquipment(state);
	return v;
}

sp<Vehicle> City::placeVehicle(GameState &state, StateRef<VehicleType> type,
                               StateRef<Organisation> owner, StateRef<Building> building)
{
	if (building->city.id != id)
	{
		LogError("Adding vehicle to a building in a different city?");
		return nullptr;
	}
	auto v = placeVehicle(state, type, owner);

	v->enterBuilding(state, building);

	return v;
}

sp<Vehicle> City::placeVehicle(GameState &state, StateRef<VehicleType> type,
                               StateRef<Organisation> owner, Vec3<float> position, float facing)
{
	auto v = placeVehicle(state, type, owner);

	v->leaveBuilding(state, position, facing);

	return v;
}

template <> sp<City> StateObject<City>::get(const GameState &state, const UString &id)
{
	auto it = state.cities.find(id);
	if (it == state.cities.end())
	{
		LogError("No citymap matching ID \"%s\"", id);
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

void RoadSegment::notifyRoadChange(const Vec3<int> &position, bool newIntact)
{
	for (size_t i = 0; i < tilePosition.size(); i++)
	{
		if (tilePosition.at(i) == position)
		{
			tileIntact.at(i) = newIntact;
			break;
		}
	}
	intact = true;
	for (size_t i = 0; i < tileIntact.size(); i++)
	{
		if (!tileIntact[i])
		{
			intact = false;
			break;
		}
	}
}

void RoadSegment::finalizeStats()
{
	length = (int)tilePosition.size();
	intact = true;
	tileIntact.resize(length, true);
	if (empty())
	{
		return;
	}
	middle = tilePosition.at(length / 2);
}

bool RoadSegment::empty() const { return tilePosition.empty(); }

const Vec3<int> &RoadSegment::getFirst() const { return tilePosition[0]; }

const Vec3<int> &RoadSegment::getLast() const { return tilePosition[length - 1]; }

const Vec3<int> &RoadSegment::getByConnectID(int id) const
{
	return id == 0 ? getFirst() : getLast();
}

bool RoadSegment::getIntactFirst() const { return tileIntact[0]; }

bool RoadSegment::getIntactLast() const { return tileIntact[length - 1]; }

bool RoadSegment::getIntactByConnectID(int id) const
{
	return id == 0 ? getIntactFirst() : getIntactLast();
}

bool RoadSegment::getIntactByTile(const Vec3<int> &position) const
{
	for (size_t i = 0; i < tilePosition.size(); i++)
	{
		if (tilePosition[i] == position)
		{
			return tileIntact[i];
		}
	}
	LogError("Invalid position supplied to getIntactByTile");
	return false;
}

RoadSegment::RoadSegment(Vec3<int> tile) { tilePosition.emplace_back(tile); }

RoadSegment::RoadSegment(Vec3<int> tile, int connection) : RoadSegment(tile)
{
	connections.emplace_back(connection);
}

} // namespace OpenApoc

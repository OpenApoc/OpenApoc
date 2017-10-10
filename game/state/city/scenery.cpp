#include "game/state/city/scenery.h"
#include "framework/configfile.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "framework/sound.h"
#include "game/state/city/agentmission.h"
#include "game/state/city/building.h"
#include "game/state/city/city.h"
#include "game/state/city/citycommonsamplelist.h"
#include "game/state/city/doodad.h"
#include "game/state/city/projectile.h"
#include "game/state/gamestate.h"
#include "game/state/rules/scenery_tile_type.h"
#include "game/state/tileview/collision.h"
#include "game/state/tileview/tile.h"
#include "game/state/tileview/tileobject_scenery.h"
#include "game/state/tileview/tileobject_vehicle.h"

namespace OpenApoc
{
Scenery::Scenery() : damaged(false), falling(false), destroyed(false) {}

void Scenery::ceaseSupportProvision()
{
	attemptReLinkSupports(getSupportedParts());
	supportedParts.clear();
}

void Scenery::queueCollapse(unsigned additionalDelay)
{
	ticksUntilCollapse = TICKS_MULTIPLIER + additionalDelay;
}

void Scenery::cancelCollapse() { ticksUntilCollapse = 0; }

sp<std::set<SupportedMapPart *>> Scenery::getSupportedParts()
{
	sp<std::set<SupportedMapPart *>> supportedParts = mksp<std::set<SupportedMapPart *>>();
	auto &map = tileObject->map;
	// Since we reference supported parts by type we have to find each in it's tile by type
	for (auto &p : this->supportedParts)
	{
		auto tile = map.getTile(p);
		for (auto &obj : tile->ownedObjects)
		{
			if (obj->getType() == TileObject::Type::Scenery)
			{
				auto mp = std::static_pointer_cast<TileObjectScenery>(obj)->getOwner();
				if (mp->destroyed)
				{
					continue;
				}
				supportedParts->insert(mp.get());
			}
		}
	}
	return supportedParts;
}

void Scenery::clearSupportedParts() { supportedParts.clear(); }

bool Scenery::findSupport()
{
	// Initial setup and quick checks
	/*if (type->floating)
	{
	    return true;
	}*/
	auto pos = tileObject->getOwningTile()->position;
	if (pos.z <= 1)
	{
		return true;
	}
	auto &map = tileObject->map;
	/*if (pos.z == map.size.z - 1 && type->supportedByAbove)
	{
	    return true;
	}*/
	auto tileType = tileObject->getType();
	auto sft = shared_from_this();

	// There are several ways scenery an get supported:
	//
	// (list is not final and will improve)
	//  - "NONE" Scenery below
	//  - "OVER" Scenery below
	//  - "NONE" Adjacent (only if there was originally no "NONE" below) // FIX THIS!
	//
	// (following conditions provide "soft" support)
	//
	// Finally, every UNDAMAGED map part can be supported if it has established support lines
	// on both sides that connect to an object providing support
	//  - Object "shoots" a line in both directions and as long as there is an object on every tile
	//    the line continues, and if an object providing hard support is reached,
	//	  then "soft" support can be attained
	//
	// Implementation:
	//  - We will first check for special conditions
	//  - Then we will gather information about adjacent map parts and check for "Supported By"
	//  - Finally we will try to cling to two objects of the same type
	//  - If all fails, we will scan on axes in search of distant support

	// Step 01: Check for special conditions

	// Bounds to check for special conditions in
	int startX = pos.x - 1;
	int endX = pos.x + 1;
	int startY = pos.y - 1;
	int endY = pos.y + 1;
	int startZ = pos.z - 1;
	int endZ = pos.z;
	// Do the check
	for (int x = startX; x <= endX; x++)
	{
		for (int y = startY; y <= endY; y++)
		{
			for (int z = startZ; z <= endZ; z++)
			{
				if (x < 0 || x >= map.size.x || y < 0 || y >= map.size.y || z < 0 ||
				    z >= map.size.z)
				{
					continue;
				}
				// Cannot support diagonally
				if (std::abs(x - pos.x) + std::abs(y - pos.y) + std::abs(z - pos.z) > 1)
				{
					continue;
				}
				auto tile = map.getTile(x, y, z);
				for (auto &o : tile->ownedObjects)
				{
					if (o->getType() == TileObject::Type::Scenery)
					{
						auto mp = std::static_pointer_cast<TileObjectScenery>(o)->getOwner();

						if (mp == sft || !mp->isAlive() || mp->damaged)
						{
							continue;
						}
						if (mp->type->walk_mode == SceneryTileType::WalkMode::Into)
						{
							continue;
						}

						mp->supportedParts.emplace_back(currentPosition);
						return true;
					}
				}
			}
		}
	}

	// Step 04: Shoot "support lines" and try to find something

	/*
	// Scan on X
	if (type->type != BattleMapPartType::Type::LeftWall && !damaged)
	{
	    int y = pos.y;
	    int z = pos.z;

	    bool found;
	    for (int increment = -1; increment <= 1; increment += 2)
	    {
	        found = false;
	        int x = pos.x + increment;
	        do
	        {
	            if (x < 0 || x >= map.size.x)
	            {
	                found = true;
	                break;
	            }
	            sp<BattleMapPart> mp = nullptr;
	            auto tile = map.getTile(x, y, z);
	            for (auto &o : tile->ownedObjects)
	            {
	                if (o->getType() == tileType)
	                {
	                    mp = std::static_pointer_cast<TileObjectBattleMapPart>(o)->getOwner();
	                }
	            }
	            // Could not find map part of this type or it cannot provide support
	            // We ignore those that have positive "ticksUntilFalling" as those can be saved yet
	            if (!mp || mp->destroyed || mp->damaged || mp->falling)
	            {
	                // fail
	                break;
	            }
	            // Found map part that provides hard support and won't collapse
	            if (mp->providesHardSupport && !mp->willCollapse())
	            {
	                // success
	                found = true;
	                break;
	            }
	            // continue
	            x += increment;
	        } while (true);
	        if (!found)
	        {
	            break;
	        }
	    }
	    // If found both ways - cling to neighbours on X
	    if (found)
	    {
	        for (int increment = -1; increment <= 1; increment += 2)
	        {
	            int x = pos.x + increment;
	            if (x < 0 || x >= map.size.x)
	            {
	                continue;
	            }
	            sp<BattleMapPart> mp = nullptr;
	            auto tile = map.getTile(x, y, z);
	            for (auto &o : tile->ownedObjects)
	            {
	                if (o->getType() == tileType)
	                {
	                    mp = std::static_pointer_cast<TileObjectBattleMapPart>(o)->getOwner();
	                }
	            }
	            if (!mp)
	            {
	                LogError("Map part disappeared? %d %d %d", x, y, z);
	                return false;
	            }
	            mp->supportedParts.emplace_back(position, type->type);
	        }
	        return true;
	    }
	}

	// Scan on Y
	if (type->type != BattleMapPartType::Type::RightWall && !damaged)
	{
	    int x = pos.x;
	    int z = pos.z;

	    bool found;
	    for (int increment = -1; increment <= 1; increment += 2)
	    {
	        found = false;
	        int y = pos.y + increment;
	        do
	        {
	            if (y < 0 || y >= map.size.y)
	            {
	                found = true;
	                break;
	            }
	            sp<BattleMapPart> mp = nullptr;
	            auto tile = map.getTile(x, y, z);
	            for (auto &o : tile->ownedObjects)
	            {
	                if (o->getType() == tileType)
	                {
	                    mp = std::static_pointer_cast<TileObjectBattleMapPart>(o)->getOwner();
	                }
	            }
	            // Could not find map part of this type or it cannot provide support
	            // We ignore those that have positive "ticksUntilFalling" as those can be saved yet
	            if (!mp || mp->destroyed || mp->damaged || mp->falling)
	            {
	                // fail
	                break;
	            }
	            // Found map part that provides hard support and won't collapse
	            if (mp->providesHardSupport && !mp->willCollapse())
	            {
	                // success
	                found = true;
	                break;
	            }
	            // continue
	            y += increment;
	        } while (true);
	        if (!found)
	        {
	            break;
	        }
	    }
	    // If found both ways - cling to neighbours on Y
	    if (found)
	    {
	        for (int increment = -1; increment <= 1; increment += 2)
	        {
	            int y = pos.y + increment;
	            if (y < 0 || y >= map.size.y)
	            {
	                continue;
	            }
	            sp<BattleMapPart> mp = nullptr;
	            auto tile = map.getTile(x, y, z);
	            for (auto &o : tile->ownedObjects)
	            {
	                if (o->getType() == tileType)
	                {
	                    mp = std::static_pointer_cast<TileObjectBattleMapPart>(o)->getOwner();
	                }
	            }
	            if (!mp)
	            {
	                LogError("Map part disappeared? %d %d %d", x, y, z);
	                return false;
	            }
	            mp->supportedParts.emplace_back(position, type->type);
	        }
	        return true;
	    }
	}
	*/

	return false;
}

void Scenery::ceaseBeingSupported()
{

	auto pos = tileObject->getOwningTile()->position;
	auto &map = tileObject->map;

	// Clean support providers for this map part
	for (int x = pos.x - 1; x <= pos.x + 1; x++)
	{
		for (int y = pos.y - 1; y <= pos.y + 1; y++)
		{
			for (int z = pos.z - 1; z <= pos.z + 1; z++)
			{
				if (x < 0 || x >= map.size.x || y < 0 || y >= map.size.y || z < 0 ||
				    z >= map.size.z)
				{
					continue;
				}
				auto tile = map.getTile(x, y, z);
				for (auto &o : tile->ownedObjects)
				{
					if (o->getType() == TileObject::Type::Scenery)
					{
						auto mp = std::static_pointer_cast<TileObjectScenery>(o)->getOwner();
						auto it = mp->supportedParts.begin();
						while (it != mp->supportedParts.end())
						{
							auto &p = *it;
							if (p == pos)
							{
								it = supportedParts.erase(it);
							}
							else
							{
								it++;
							}
						}
					}
				}
			}
		}
	}
}

Vec3<int> Scenery::getTilePosition() const { return tileObject->getOwningTile()->position; }

const TileMap &Scenery::getMap() const { return tileObject->map; }

UString Scenery::getId() const { return type.id; }

int Scenery::getType() const { return (int)0; }

UString Scenery::getSupportString() const { return "NORMAL"; }

bool Scenery::handleCollision(GameState &state, Collision &c)
{
	// Adjust relationships
	if (building && c.projectile->firerVehicle)
	{
		auto attackerOrg = c.projectile->firerVehicle->owner;
		auto ourOrg = building->owner;
		// Lose 5 points
		ourOrg->adjustRelationTo(state, attackerOrg, -5.0f);
		// Our allies lose 2.5 points, enemies gain 1 point
		for (auto &org : state.organisations)
		{
			if (org.first != attackerOrg.id && org.first != state.getCivilian().id)
			{
				if (org.second->isRelatedTo(ourOrg) == Organisation::Relation::Hostile)
				{
					org.second->adjustRelationTo(state, attackerOrg, 1.0f);
				}
				else if (org.second->isRelatedTo(ourOrg) == Organisation::Relation::Allied)
				{
					org.second->adjustRelationTo(state, attackerOrg, -2.5f);
				}
			}
		}
	}

	return applyDamage(state, c.projectile->damage);
}

bool Scenery::applyDamage(GameState &state, int power)
{
	if (!this->tileObject)
	{
		// It's possible multiple projectiles hit the same tile in the same
		// tick, so if the object has already been destroyed just NOP this.
		// The projectile will still 'hit' this tile though.
		return false;
	}
	if (this->falling)
	{
		// Already falling, just continue
		return false;
	}

	int damage = randDamage050150(state.rng, power);

	if (type->tile_type == SceneryTileType::TileType::Road)
	{
		if (config().getBool("OpenApoc.Mod.InvulnerableRoads"))
		{
			return false;
		}
		if (config().getBool("OpenApoc.NewFeature.ArmoredRoads"))
		{
			damage -= ROAD_ARMOR;
		}
	}

	if (damage <= type->constitution)
	{
		return false;
	}

	die(state);
	return false;
}

void Scenery::die(GameState &state)
{
	if (falling)
	{
		auto doodad = city->placeDoodad({&state, "DOODAD_3_EXPLOSION"}, tileObject->getCenter());
		fw().soundBackend->playSample(state.city_common_sample_list->sceneryExplosion,
		                              tileObject->getCenter());
		LogWarning("Spawn smoke doodad");
		LogWarning("Deal damage to ships");
		this->tileObject->removeFromMap();
		this->tileObject.reset();
		this->destroyed = true;
		return;
	}
	if (type->damagedTile)
	{
		this->damaged = true;
		if (this->overlayDoodad)
		{
			this->overlayDoodad->remove(state);
			this->overlayDoodad = nullptr;
		}
		type = type->damagedTile;
		if (type->overlaySprite)
		{
			this->overlayDoodad =
			    mksp<Doodad>(this->getPosition(), type->imageOffset, false, 1, type->overlaySprite);
			city->map->addObjectToMap(this->overlayDoodad);
		}
		// Reapply tile params
		tileObject->setPosition(currentPosition);
	}
	else
	{
		// Don't destroy bottom tiles, else everything will leak out
		if (this->initialPosition.z <= 1)
		{
			this->damaged = true;
			// Reapply tile params
			tileObject->setPosition(currentPosition);
		}
		else
		{
			auto doodad =
			    city->placeDoodad({&state, "DOODAD_3_EXPLOSION"}, tileObject->getCenter());
			fw().soundBackend->playSample(state.city_common_sample_list->vehicleExplosion,
			                              tileObject->getCenter());

			this->destroyed = true;
		}
	}

	// Queue updates
	// state.current_battle->queueVisionRefresh(position);
	// state.current_battle->queuePathfindingRefresh(position);

	// Cease functioning
	ceaseBeingSupported();
	ceaseSupportProvision();

	// Re-establish support for this if still alive
	if (isAlive())
	{
		if (!findSupport())
		{
			queueCollapse();
		}
	}
	// Destroy if destroyed
	else if (destroyed)
	{
		if (this->overlayDoodad)
		{
			this->overlayDoodad->remove(state);
			this->overlayDoodad = nullptr;
		}
		this->tileObject->removeFromMap();
		this->tileObject.reset();
	}
}

void Scenery::collapse(GameState &state)
{
	// If it's already falling or destroyed or supported do nothing
	if (falling || !tileObject)
	{
		return;
	}
	// Level 0 can't collapse
	if (this->initialPosition.z <= 1)
	{
		this->damaged = true;
		LogWarning("Scenery at %s can't fall as below 2", currentPosition);
	}
	else
	{
		LogWarning("Scenery at %s now falling", currentPosition);
		falling = true;
		// state.current_battle->queueVisionRefresh(position);
		// state.current_battle->queuePathfindingRefresh(position);
		// Note: Pathfinding refresh relies on tile's battlescape parameters being updated
		// before it happens, so that battlescape parameters already account for the
		// now disfunctional map part. Pathfinding update will only happen
		// after we call setPosition() on the map part, which will
		// call update to the battlescape parameters of the tile, which will
		// in turn make us ignore the falling map part properly
		// If we would somehow call collapse() in a way that would set falling to true but
		// would not trigger the setPosition() afterwards, this logic would fail
	}
	ceaseSupportProvision();
}

void Scenery::update(GameState &state, unsigned int ticks)
{
	if (!tileObject)
	{
		return;
	}

	if (ticksUntilCollapse > 0)
	{
		if (ticksUntilCollapse > ticks)
		{
			ticksUntilCollapse -= ticks;
		}
		else
		{
			ticksUntilCollapse = 0;
			collapse(state);
		}
	}

	// Process falling
	if (falling)
	{
		updateFalling(state, ticks);
		return;
	}
}

void Scenery::updateFalling(GameState &state, unsigned int ticks)
{
	auto fallTicksRemaining = ticks;
	auto newPosition = currentPosition;
	while (fallTicksRemaining-- > 0)
	{
		fallingSpeed += FALLING_ACCELERATION_MAP_PART;
		newPosition -= Vec3<float>{0.0f, 0.0f, (fallingSpeed / TICK_SCALE)} / VELOCITY_SCALE_CITY;
	}

	// Collision with this tile happens when map part moves from this tile to the next
	if (newPosition.z < 0 || floorf(newPosition.z) != floorf(currentPosition.z))
	{
		for (auto &obj : tileObject->getOwningTile()->ownedObjects)
		{
			switch (obj->getType())
			{
				// If there's a live ground or map mart of our type here - die
				case TileObject::Type::Scenery:
				{
					auto mp = std::static_pointer_cast<TileObjectScenery>(obj)->getOwner();

					// Find if we collide into it
					if (tileObject && mp->isAlive())
					{
						destroyed = true;
					}

					break;
				}
				case TileObject::Type::Vehicle:
				// FIXME: Cause damage to vehicles we hit?
				default:
					// Ignore other object types?
					break;
			}
		}
		// Spawn smoke, more intense if we land here
		{
			/*StateRef<DamageType> dtSmoke = { &state, "DAMAGETYPE_SMOKE" };
			auto hazard = state.current_battle->placeHazard(
			    state, owner, nullptr, dtSmoke, position, dtSmoke->hazardType->getLifetime(state),
			    2, destroyed ? 6 : 12);
			if (hazard)
			{
			    hazard->ticksUntilVisible = 0;
			}*/
		}
		// Cease to exist if destroyed
		if (destroyed)
		{
			die(state);
			return;
		}
	}

	currentPosition = newPosition;
	tileObject->setPosition(newPosition);
}

bool Scenery::canRepair() const
{
	// Don't fix it if it ain't broken

	if (this->isAlive() && !damaged)
		return false;
	// FIXME: Check how apoc repairs stuff, for now disallow repair

	return false;
}

void Scenery::repair(GameState &state)
{
	auto &map = *this->city->map;
	if (this->isAlive())
		LogError("Trying to fix something that isn't broken");
	this->damaged = false;
	this->falling = false;
	if (this->tileObject)
		this->tileObject->removeFromMap();
	this->tileObject = nullptr;

	if (this->overlayDoodad)
		this->overlayDoodad->remove(state);
	this->overlayDoodad = nullptr;
	map.addObjectToMap(shared_from_this());
	if (type->overlaySprite)
	{
		this->overlayDoodad =
		    mksp<Doodad>(this->getPosition(), type->imageOffset, false, 1, type->overlaySprite);
		map.addObjectToMap(this->overlayDoodad);
	}
	map.clearPathCaches();
}

bool Scenery::isAlive() const
{
	if (this->falling || this->destroyed || willCollapse())
		return false;
	return true;
}

} // namespace OpenApoc

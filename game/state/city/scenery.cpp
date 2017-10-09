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
	// Landing pads are immortal (else this completely destroys pathing)
	if (this->type->isLandingPad)
	{
		return;
	}

	if (falling)
	{
		this->tileObject->removeFromMap();
		this->tileObject.reset();
		this->destroyed = true;
		return;
	}
	if (!this->damaged && type->damagedTile)
	{
		this->damaged = true;
		if (this->overlayDoodad)
			this->overlayDoodad->remove(state);
		this->overlayDoodad = nullptr;
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
		if (this->initialPosition.z != 1)
		{
			auto doodad =
			    city->placeDoodad({&state, "DOODAD_3_EXPLOSION"}, tileObject->getCenter());
			fw().soundBackend->playSample(state.city_common_sample_list->sceneryExplosion,
			                              tileObject->getCenter());

			this->tileObject->removeFromMap();
			this->tileObject.reset();
			this->destroyed = true;
		}
	}
	if (this->overlayDoodad)
	{
		this->overlayDoodad->remove(state);
	}
	this->overlayDoodad = nullptr;
	for (auto &s : this->supports)
	{
		s->collapse(state);
	}
}

void Scenery::collapse(GameState &state)
{
	// IF it's already falling or destroyed do nothing
	if (this->falling || !this->tileObject)
		return;
	// Landing pads can't collapse, and may magically float, otherwise it screws up pathing (same
	// reason why they can't be destroyed when hit)
	if (this->type->isLandingPad)
		return;
	this->falling = true;

	for (auto &s : this->supports)
		s->collapse(state);
}

void Scenery::update(GameState &state, unsigned int ticks)
{
	if (!this->falling)
		return;
	if (!this->tileObject)
	{
		LogError("Falling scenery with no object?");
	}

	auto currentPos = this->tileObject->getPosition();
	// FIXME: gravity acceleration?
	currentPos.z -= static_cast<float>(ticks) / 16.0f;
	this->tileObject->setPosition(currentPos);
	if (this->overlayDoodad)
		this->overlayDoodad->setPosition(this->tileObject->getPosition());

	for (auto &obj : this->tileObject->getOwningTile()->ownedObjects)
	{
		switch (obj->getType())
		{
			case TileObject::Type::Scenery:
			{
				auto sceneryTile = std::static_pointer_cast<TileObjectScenery>(obj);
				// Skip stuff already falling (wil include this)
				if (sceneryTile->getOwner()->falling)
					continue;
				// Any other scenery
				// FIXME: Cause damage to scenery we hit?
				this->falling = false;
				auto doodad = city->placeDoodad(StateRef<DoodadType>{&state, "DOODAD_EXPLOSION_3"},
				                                currentPos);
				this->tileObject->removeFromMap();
				this->tileObject.reset();
				if (this->overlayDoodad)
					this->overlayDoodad->remove(state);
				this->overlayDoodad = nullptr;
				// return as we can't be destroyed more than once
				return;
			}
			case TileObject::Type::Vehicle:
				// FIXME: Cause damage to vehicles we hit?
				break;
			default:
				// Ignore other object types?
				break;
		}
	}
}

bool Scenery::canRepair() const
{
	// Don't fix it if it ain't broken

	if (this->isAlive())
		return false;
	// FIXME: Check how apoc repairs stuff, for now allow repair if at least one support is
	// available
	//(IE it's attached to /something/)
	if (this->supportedBy.size() == 0)
	{
		/* Tiles at z == 1 can get damaged (But not destroyed!) but have no support */
		if (this->damaged)
			return true;
		LogWarning("Scenery not supported but destroyed?");
		return true;
	}
	for (auto &s : this->supportedBy)
	{
		if (s->isAlive())
			return true;
	}
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
	if (this->damaged || this->falling || this->destroyed)
		return false;
	return true;
}

} // namespace OpenApoc

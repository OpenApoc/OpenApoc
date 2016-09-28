#include "game/state/city/scenery.h"
#include "framework/logger.h"
#include "game/state/city/city.h"
#include "game/state/city/doodad.h"
#include "game/state/gamestate.h"
#include "game/state/rules/scenery_tile_type.h"
#include "game/state/tileview/tile.h"
#include "game/state/tileview/tileobject_scenery.h"

namespace OpenApoc
{
Scenery::Scenery() : damaged(false), falling(false), destroyed(false) {}

void Scenery::handleCollision(GameState &state, Collision &c)
{
	// FIXME: Proper damage
	std::ignore = c;
	// If this tile has a damaged tile, replace it with that. If it's already damaged, destroy as
	// normal
	if (!this->tileObject)
	{
		// It's possible multiple projectiles hit the same tile in the same
		// tick, so if the object has already been destroyed just NOP this.
		// The projectile will still 'hit' this tile though.
		return;
	}
	if (this->falling)
	{
		// Already falling, just continue
		return;
	}
	// Landing pads are immortal (else this completely destroys pathing)
	if (this->type->isLandingPad)
	{
		return;
	}
	if (!this->damaged && type->damagedTile)
	{
		this->damaged = true;
	}
	else
	{
		// Don't destroy bottom tiles, else everything will leak out
		if (this->initialPosition.z != 1)
		{
			this->tileObject->removeFromMap();
			this->tileObject.reset();
		}
	}
	if (this->overlayDoodad)
		this->overlayDoodad->remove(state);
	this->overlayDoodad = nullptr;
	for (auto &s : this->supports)
		s->collapse(state);
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
	auto &map = this->city->map;
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
	map->addObjectToMap(shared_from_this());
	if (type->overlaySprite)
	{
		this->overlayDoodad =
		    mksp<Doodad>(this->getPosition(), type->imageOffset, false, 1, type->overlaySprite);
		map->addObjectToMap(this->overlayDoodad);
	}
}

bool Scenery::isAlive() const
{
	if (this->damaged || this->falling || this->destroyed)
		return false;
	return true;
}


} // namespace OpenApoc

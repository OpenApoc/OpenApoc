#include "game/state/battlemappart.h"
#include "game/state/battle/battleitem.h"
#include "game/state/battlemappart_type.h"
#include "game/state/gamestate.h"
#include "game/state/rules/scenery_tile_type.h"
#include "game/state/tileview/tile.h"
#include "game/state/tileview/tile.h"
#include "game/state/tileview/tileobject_battlemappart.h"
#include "game/state/tileview/tileobject_scenery.h"

namespace OpenApoc
{

void BattleMapPart::handleCollision(GameState &state, Collision &c)
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
	if (!this->damaged && type->damaged_map_part)
	{
		this->damaged = true;
		this->type = type->damaged_map_part;
	}
	else
	{
		// Don't destroy bottom tiles, else everything will leak out
		if (this->initialPosition.z != 0 || this->type->type != BattleMapPartType::Type::Ground)
		{
			this->tileObject->removeFromMap();
			this->tileObject.reset();
		}
		else
		{
			auto b = battle.lock();
			if (!b)
				LogError("Battle disappeared!");
			this->type = b->battle_map->destroyed_ground_tile;
		}
	}
	for (auto &s : this->supportedParts)
		s->collapse(state);
	for (auto &s : this->supportedItems)
	{
		auto i = s.lock();
		if (i)
			i->supported = false;
	}
}

void BattleMapPart::collapse(GameState &state)
{
	// IF it's already falling or destroyed do nothing
	if (this->falling || !this->tileObject)
		return;
	this->falling = true;

	for (auto &s : this->supportedParts)
		s->collapse(state);
}

void BattleMapPart::update(GameState &, unsigned int ticks)
{
	if (!this->falling)
		return;
	if (!this->tileObject)
	{
		LogError("Falling map part with no object?");
	}

	auto currentPos = this->tileObject->getPosition();
	// FIXME: gravity acceleration?
	currentPos.z -= static_cast<float>(ticks) / 16.0f;
	this->tileObject->setPosition(currentPos);

	for (auto &obj : this->tileObject->getOwningTile()->ownedObjects)
	{
		switch (obj->getType())
		{
			case TileObject::Type::Ground:
				// FIXME: do something?
				break;
			default:
				// Ignore other object types?
				break;
		}
	}
}

bool BattleMapPart::isAlive() const
{
	if (this->damaged || this->falling || this->destroyed)
		return false;
	return true;
}
}

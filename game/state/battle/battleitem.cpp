#include "game/state/battle/battleitem.h"
#include "framework/logger.h"
#include "game/state/battle/battle.h"
#include "game/state/aequipment.h"
#include "game/state/gamestate.h"
#include "game/state/tileview/collision.h"
#include "game/state/tileview/tile.h"
#include "game/state/tileview/tileobject_battleitem.h"
#include "game/state/tileview/tileobject_battlemappart.h"
#include "game/state/tileview/tileobject_shadow.h"

namespace OpenApoc
{
void BattleItem::die(GameState &, bool violently)
{
	if (violently)
	{
		// FIXME: Explode if nessecary
	}
	auto this_shared = shared_from_this();
	auto b = battle.lock();
	if (b)
		b->items.remove(this_shared);
	this->tileObject->removeFromMap();
	this->shadowObject->removeFromMap();
	this->tileObject.reset();
	this->shadowObject.reset();
}

void BattleItem::handleCollision(GameState &state, Collision &c)
{
	// FIXME: Proper damage
	std::ignore = c;
	die(state, true);
}

void BattleItem::setPosition(const Vec3<float> &pos)
{
	this->position = pos;
	if (!this->tileObject)
	{
		LogError("setPosition called on item with no tile object");
	}
	else
	{
		this->tileObject->setPosition(pos);
	}

	if (!this->shadowObject)
	{
		LogError("setPosition called on item with no shadow object");
	}
	else
	{
		this->shadowObject->setPosition(pos);
	}
}

void BattleItem::update(GameState &state, unsigned int ticks)
{
	static const std::set<TileObject::Type> mapPartSet = {
	    TileObject::Type::Ground, TileObject::Type::LeftWall, TileObject::Type::RightWall,
	    TileObject::Type::Feature};

	if (supported)
		return;

	this->velocity.z -= (float)item->type->weight * static_cast<float>(ticks) / TICK_SCALE;
	// FIXME: Limit velocity after applying gravity?

	auto previousPosition = position;
	auto newPosition =
	    previousPosition +
	    ((static_cast<float>(ticks) / TICK_SCALE) * this->velocity) / VELOCITY_SCALE_BATTLE;

	// Check if new position is valid
	auto c = tileObject->map.findCollision(previousPosition, newPosition, mapPartSet);
	// If colliding and moving somewhere other than down, re-try with no sideways movement
	if (c && (velocity.x != 0.0f || velocity.y != 0.0f))
	{
		velocity.x = 0.0f;
		velocity.y = 0.0f;
		newPosition =
		    previousPosition +
		    ((static_cast<float>(ticks) / TICK_SCALE) * this->velocity) / VELOCITY_SCALE_BATTLE;
		c = tileObject->map.findCollision(previousPosition, newPosition, mapPartSet);
	}
	// If colliding and moving down - totally cancel movement
	if (c)
	{
		// Place object on ground, in correct place, and establish link to it
		supported = true;
		velocity.z = 0.0f;
		newPosition = previousPosition;

		auto obj = std::static_pointer_cast<TileObjectBattleMapPart>(c.obj)->getOwner();
		obj->supportedItems.push_back(shared_from_this());
		Vec3<float> proper_position = tileObject->getOwningTile()->getRestingPosition();
		if (position != proper_position)
			setPosition(proper_position);
	}

	// If moved - check if within level bounds
	if (newPosition != previousPosition)
	{
		auto mapSize = this->tileObject->map.size;

		// Remove if it fell off the end of the world
		if (newPosition.x < 0 || newPosition.x >= mapSize.x || newPosition.y < 0 ||
		    newPosition.y >= mapSize.y || newPosition.z < 0 || newPosition.z >= mapSize.z)
		{
			die(state, false);
		}
		else
		{
			setPosition(newPosition);
		}
	}
}
} // namespace OpenApoc

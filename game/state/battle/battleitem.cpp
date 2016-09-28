#include "game/state/battle/battleitem.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "game/state/aequipment.h"
#include "game/state/battle/battle.h"
#include "game/state/gamestate.h"
#include "game/state/tileview/collision.h"
#include "game/state/tileview/tile.h"
#include "game/state/tileview/tileobject_battleitem.h"
#include "game/state/tileview/tileobject_battlemappart.h"
#include "game/state/tileview/tileobject_shadow.h"
#include <cmath>

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
	{
		return;
	}

	velocity.z -= (float)item->type->weight * static_cast<float>(ticks) / TICK_SCALE;
	// FIXME: Limit velocity after applying gravity?
	// Disabled until we figure out starting throwing item vectors properly
	// velocity = std::min(glm::length(velocity), FALLING_SPEED_CAP) * glm::normalize(velocity);

	auto previousPosition = position;
	auto newPosition =
	    previousPosition +
	    ((static_cast<float>(ticks) / TICK_SCALE) * this->velocity) / VELOCITY_SCALE_BATTLE;

	// Check if new position is valid
	auto c = tileObject->map.findCollision(previousPosition, newPosition, mapPartSet);
	if (c)
	{
		// If colliding and moving somewhere other than down, stop item (it will resume falling shortly)
		if (velocity.x != 0.0f || velocity.y != 0.0f)
		{
			velocity = { 0.0f, 0.0f, 0.0f };
			newPosition = previousPosition;
		}
		// If colliding and moving down
		else
		{
			setPosition({ c.position.x, c.position.y, floorf(c.position.z) });
			if (!findSupport(true, true))
			{
				return;
			}
			// Some objects have buggy voxelmaps and items collide with them but no support is given
			// In this case, jusst ignore the collision
		}
	}

	// If moved but did not find support - check if within level bounds and set position
	if (newPosition != previousPosition)
	{
		auto mapSize = this->tileObject->map.size;

		// Collision with ceiling
		if (newPosition.z >= mapSize.z)
		{
			newPosition.z = mapSize.z - 0.01f;
			velocity = { 0.0f, 0.0f, 0.0f };
		}
		// Remove if it fell off the end of the world
		if (newPosition.x < 0 || newPosition.x >= mapSize.x || newPosition.y < 0 ||
		    newPosition.y >= mapSize.y || newPosition.z < 0 || newPosition.z >= mapSize.z)
		{
			die(state, false);
		}
		else
		{
			setPosition(newPosition);
			findSupport();
		}
	}
}

bool BattleItem::findSupport(bool emitSound, bool forced)
{
	if (supported)
		return true;
	auto tile = tileObject->getOwningTile();
	auto obj = tile->getItemSupportingObject();
	if (!obj)
	{
		return false;
	}
	auto restingPosition = obj->getPosition() + Vec3<float>{0.0f, 0.0f, (float)obj->type->height / 40.0f};
	if (!forced && position.z > restingPosition.z)
	{
		return false;
	}

	supported = true;
	velocity = { 0.0f,0.0f,0.0f };
	obj->supportedItems.push_back(shared_from_this());
	if (position != restingPosition)
	{
		setPosition(restingPosition);
	}

	// Emit sound
	if (emitSound && tile->objectDropSfx)
	{
		fw().soundBackend->playSample(tile->objectDropSfx, getPosition(), 0.25f);
	}
	return true;
}

} // namespace OpenApoc

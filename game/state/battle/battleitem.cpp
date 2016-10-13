#include "game/state/battle/battleitem.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "framework/sound.h"
#include "game/state/aequipment.h"
#include "game/state/battle/battle.h"
#include "game/state/battle/battlemappart.h"
#include "game/state/gamestate.h"
#include "game/state/rules/aequipment_type.h"
#include "game/state/rules/damage.h"
#include "game/state/tileview/collision.h"
#include "game/state/tileview/tile.h"
#include "game/state/tileview/tileobject_battleitem.h"
#include "game/state/tileview/tileobject_battlemappart.h"
#include "game/state/tileview/tileobject_battleunit.h"
#include "game/state/tileview/tileobject_shadow.h"
#include <cmath>

namespace OpenApoc
{

void BattleItem::die(GameState &state, bool violently)
{
	if (violently)
	{
		item->explode(state);
	}
	auto this_shared = shared_from_this();
	state.current_battle->items.remove(this_shared);
	this->tileObject->removeFromMap();
	this->shadowObject->removeFromMap();
	this->tileObject.reset();
	this->shadowObject.reset();
}

// Returns true if sound and doodad were handled by it
bool BattleItem::applyDamage(GameState &state, int power, StateRef<DamageType> damageType)
{
	if (damageType->explosive != true || damageType->effectType != DamageType::EffectType::None)
	{
		return false;
	}
	if (item->type->armor <= power)
	{
		die(state);
	}
	return false;
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

Collision BattleItem::checkItemCollision(Vec3<float> previousPosition, Vec3<float> nextPosition)
{
	Collision c = tileObject->map.findCollision(
	    previousPosition, nextPosition, {},
	    ownerInvulnerableTicks > 0 ? item->ownerAgent->unit->tileObject : nullptr);
	return c;
}

void BattleItem::update(GameState &state, unsigned int ticks)
{
	item->update(state, ticks);

	if (ticksUntilCollapse > 0)
	{
		if (ticksUntilCollapse > ticks)
		{
			ticksUntilCollapse -= ticks;
		}
		else
		{
			ticksUntilCollapse = 0;
			collapse();
		}
	}

	if (!falling)
	{
		return;
	}

	if (ownerInvulnerableTicks > 0)
	{
		if (ownerInvulnerableTicks > ticks)
		{
			ownerInvulnerableTicks -= ticks;
		}
		else
		{
			ownerInvulnerableTicks = 0;
		}
	}
	int remainingTicks = ticks;

	auto previousPosition = position;
	auto newPosition = position;

	while (remainingTicks-- > 0)
	{
		velocity.z -= FALLING_ACCELERATION_ITEM;
		newPosition += this->velocity / (float)TICK_SCALE / VELOCITY_SCALE_BATTLE;
	}

	// Check if new position is valid
	// FIXME: Collide with units but not with us
	bool collision = false;
	auto c = checkItemCollision(previousPosition, newPosition);
	if (c)
	{
		collision = true;
		// If colliding with anything but ground, bounce back once
		switch (c.obj->getType())
		{
			case TileObject::Type::Unit:
			case TileObject::Type::LeftWall:
			case TileObject::Type::RightWall:
			case TileObject::Type::Feature:
				if (!bounced)
				{
					// If bounced do not try to find support this time
					collision = false;
					bounced = true;
					newPosition = previousPosition;
					velocity.x = -velocity.x / 4;
					velocity.y = -velocity.y / 4;
					velocity.z = std::abs(velocity.z / 4);
					break;
				}
			// Intentional fall-through
			case TileObject::Type::Ground:
				// Let item fall so that it can collide with scenery or ground if falling on top of
				// it
				newPosition = {previousPosition.x, previousPosition.y,
				               std::min(newPosition.z, previousPosition.z)};
				break;
			default:
				LogError("What the hell is this item colliding with? Type is %d",
				         (int)c.obj->getType());
				break;
		}
	}

	// If moved but did not find support - check if within level bounds and set position
	if (newPosition != previousPosition)
	{
		auto mapSize = this->tileObject->map.size;

		// Collision with ceiling
		if (newPosition.z >= mapSize.z)
		{
			collision = true;
			newPosition.z = mapSize.z - 0.01f;
			velocity = {0.0f, 0.0f, 0.0f};
		}
		// Collision with map edge
		if (newPosition.x < 0 || newPosition.y < 0 || newPosition.y >= mapSize.y ||
		    newPosition.x >= mapSize.x || newPosition.y >= mapSize.y)
		{
			collision = true;
			velocity.x = -velocity.x / 4;
			velocity.y = -velocity.y / 4;
			velocity.z = 0;
			newPosition = previousPosition;
		}
		// Fell below 0???
		if (newPosition.z < 0)
		{
			LogError("Item at %f %f fell off the end of the world!?", newPosition.x, newPosition.y);
			die(state, false);
			return;
		}
		setPosition(newPosition);
	}

	if (collision)
	{
		if (findSupport())
		{
			getSupport();
			auto tile = tileObject->getOwningTile();
			if (tile->objectDropSfx)
			{
				fw().soundBackend->playSample(tile->objectDropSfx, getPosition(), 0.25f);
			}
		}
	}
}

void BattleItem::getSupport()
{
	auto tile = tileObject->getOwningTile();
	auto obj = tile->getItemSupportingObject();
	auto restingPosition =
	    obj->getPosition() + Vec3<float>{0.0f, 0.0f, (float)obj->type->height / 40.0f};

	bounced = false;
	falling = false;
	velocity = {0.0f, 0.0f, 0.0f};
	obj->supportedItems = true;
	if (position != restingPosition)
	{
		setPosition(restingPosition);
	}
}

bool BattleItem::findSupport()
{
	auto tile = tileObject->getOwningTile();
	auto obj = tile->getItemSupportingObject();
	if (!obj)
	{
		return false;
	}
	auto restingPosition =
	    obj->getPosition() + Vec3<float>{0.0f, 0.0f, (float)obj->type->height / 40.0f};

	if (position.z > restingPosition.z)
	{
		return false;
	}

	return true;
}

void BattleItem::tryCollapse()
{
	if (falling)
	{
		return;
	}
	if (!findSupport())
	{
		ticksUntilCollapse = TICKS_MULTIPLIER;
	}
}

void BattleItem::collapse() { falling = true; }

} // namespace OpenApoc

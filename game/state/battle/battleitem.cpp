#include "game/state/battle/battleitem.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "framework/sound.h"
#include "game/state/battle/battle.h"
#include "game/state/battle/battlemappart.h"
#include "game/state/battle/battleunit.h"
#include "game/state/gamestate.h"
#include "game/state/rules/aequipmenttype.h"
#include "game/state/rules/battle/battlecommonsamplelist.h"
#include "game/state/rules/battle/damage.h"
#include "game/state/shared/aequipment.h"
#include "game/state/tilemap/collision.h"
#include "game/state/tilemap/tilemap.h"
#include "game/state/tilemap/tileobject_battleitem.h"
#include "game/state/tilemap/tileobject_battlemappart.h"
#include "game/state/tilemap/tileobject_battleunit.h"
#include "game/state/tilemap/tileobject_shadow.h"
#include <cmath>
#include <glm/glm.hpp>

namespace OpenApoc
{

void BattleItem::die(GameState &state, bool violently)
{
	if (violently)
	{
		item->explode(state);
	}
	// Lose score if item that dies and it's not a primed grenade
	if (!item->primed && item->ownerOrganisation && item->ownerOrganisation == state.getPlayer())
	{
		state.current_battle->score.equipmentLost -= item->type->score;
		if (item->payloadType)
		{
			state.current_battle->score.equipmentLost -= item->payloadType->score;
		}
	}
	auto this_shared = shared_from_this();
	state.current_battle->items.remove(this_shared);
	this->tileObject->removeFromMap();
	this->shadowObject->removeFromMap();
	this->tileObject.reset();
	this->shadowObject.reset();
}

void BattleItem::hopTo(GameState &state, Vec3<float> targetPosition)
{
	if (falling)
	{
		return;
	}
	// It was observed that boomeroids hop 1 to 4 tiles away (never overshooting)
	int distance =
	    std::min(16.0f, BattleUnitTileHelper::getDistanceStatic(position, targetPosition)) / 4.0f;
	distance = distance > 1 ? randBoundsInclusive(state.rng, 1, distance) : distance;
	auto targetVector = targetPosition - position;
	float velXY = 0.0f;
	float velZ = 0.0f;
	while (distance > 1)
	{
		// Try to hop this distance towards target
		velXY = 0.0f;
		velZ = 0.0f;
		Vec3<float> target = position + glm::normalize(targetVector) * (float)distance;
		if (item->getVelocityForThrow(tileObject->map, 100, position, target, velXY, velZ))
		{
			break;
		}
		else
		{
			distance--;
		}
	}
	if (distance > 1)
	{
		falling = true;
		velocity = (glm::normalize(Vec3<float>{targetVector.x, targetVector.y, 0.0f}) * velXY +
		            Vec3<float>{0.0f, 0.0f, velZ}) *
		           VELOCITY_SCALE_BATTLE;
		// Enough to leave our home cell
		collisionIgnoredTicks =
		    (int)ceilf(36.0f / glm::length(velocity / VELOCITY_SCALE_BATTLE)) + 1;
	}
}

// Returns true if sound and doodad were handled by it
bool BattleItem::applyDamage(GameState &state, int power, StateRef<DamageType> damageType)
{
	if (damageType->explosive != true)
	{
		return false;
	}
	switch (damageType->effectType)
	{
		case DamageType::EffectType::None:
			if (item->armor <= power)
			{
				die(state);
			}
			break;
		case DamageType::EffectType::Fire:
			// Armor resists fire damage accordingly
			if (item->type->type == AEquipmentType::Type::Armor)
			{
				power = damageType->dealDamage(power, item->type->damage_modifier);
			}
			item->armor -= power;
			if (item->armor <= 0)
			{
				die(state);
			}
			break;
		default:
			return false;
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
	if (collisionIgnoredTicks > 0)
		return {};
	Collision c = tileObject->map.findCollision(
	    previousPosition, nextPosition, {},
	    ownerInvulnerableTicks > 0 ? item->ownerUnit->tileObject : nullptr);
	return c;
}

void BattleItem::updateTB(GameState &state) { item->updateTB(state); }

void BattleItem::update(GameState &state, unsigned int ticks)
{
	item->update(state, ticks);
	// May have exploded
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

	if (collisionIgnoredTicks > 0)
	{
		if (collisionIgnoredTicks > ticks)
		{
			collisionIgnoredTicks -= ticks;
		}
		else
		{
			collisionIgnoredTicks = 0;
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

#include "game/state/battle/battlemappart.h"
#include "game/state/battle/battledoor.h"
#include "game/state/battle/battleitem.h"
#include "game/state/battle/battlemappart_type.h"
#include "game/state/city/projectile.h"
#include "game/state/gamestate.h"
#include "game/state/rules/damage.h"
#include "game/state/tileview/collision.h"
#include "game/state/tileview/tile.h"
#include "game/state/tileview/tileobject_battlemappart.h"

namespace OpenApoc
{

int BattleMapPart::getMaxFrames()
{
	return alternative_type ? (int)alternative_type->animation_frames.size()
	                        : (int)type->animation_frames.size();
}

int BattleMapPart::getAnimationFrame()
{
	if (door)
	{
		return std::min(getMaxFrames() - 1, door->getAnimationFrame());
	}
	else
	{
		return type->animation_frames.size() == 0 ? -1 : animation_frame_ticks /
		                                                     TICKS_PER_FRAME_MAP_PART;
	}
}

bool BattleMapPart::handleCollision(GameState &state, Collision &c)
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

	// Calculate damage (hmm, apparently Apoc uses 50-150 damage model for terrain, unlike UFO1&2
	// which used 25-75
	int damage = randDamage050150(state.rng, c.projectile->damageType->dealDamage(
	                                             c.projectile->damage, type->damageModifier));
	if (damage <= type->constitution)
	{
		return false;
	}

	// If we came this far, map part has been damaged and must cease to be
	
	// Cease functioning if door
	if (door)
	{
		if (alternative_type)
			type = alternative_type;
		// Remove from door's map parts
		wp<BattleMapPart> sft = shared_from_this();
		door->mapParts.remove_if([sft](wp<BattleMapPart> p) {
			auto swp = sft.lock();
			auto sp = p.lock();
			if (swp && sp)
				return swp == sp;
			return false;
		});
		door.clear();
	}
	
	// Doodad
	auto doodad = state.current_battle->placeDoodad({&state, "DOODAD_29_EXPLODING_TERRAIN"},
	                                                tileObject->getCenter());
	// Replace with damaged
	if (!this->damaged && type->damaged_map_part)
	{
		this->damaged = true;
		this->type = type->damaged_map_part;
	}
	else
	{
		// Don't destroy bottom tiles, else everything will leak out
		// Replace ground with destroyed
		if (this->initialPosition.z == 0 && this->type->type == BattleMapPartType::Type::Ground)
		{
			this->type = type->destroyed_map_parts.front();
		}
		// Destroy map part
		else
		{
			this->tileObject->removeFromMap();
			this->tileObject.reset();

			ceaseSupportProvision();
		}
	}
	return false;
}

void BattleMapPart::findSupport()
{
	// If it's already falling or destroyed or supported do nothing
	if (this->falling || !this->tileObject || supported)
		return;

	supported = false;
	// FIXME: Implement
	//
	// - Check every neighbouring map part of our type
	//		- If map part fits our supportedBy criteria then add it to a temp list
	// - If our temp list fits our criteria then we are supported
	supported = true;
}

void BattleMapPart::ceaseSupportProvision()
{
	for (auto &s : this->supportedParts)
	{
		auto i = s.lock();
		if (i)
		{
			i->findSupport();
			i->tryCollapse();
		}
	}
	this->supportedParts.clear();
	for (auto &s : this->supportedItems)
	{
		auto i = s.lock();
		if (i)
		{
			i->supported = false;
			i->findSupport(false);
		}
	}
	this->supportedItems.clear();
}

void BattleMapPart::tryCollapse(bool force)
{
	// If it's already falling or destroyed or supported do nothing
	if (this->falling || !this->tileObject || (!force && supported))
		return;
	this->falling = true;
	ceaseSupportProvision();
	if (door)
	{
		door->collapse();
	}
}

void BattleMapPart::update(GameState &, unsigned int ticks)
{
	// Process falling
	if (this->falling)
	{
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
		return;
	}
	else // !this -> falling
	{
		// Animate non-doors
		if (!door && type->animation_frames.size() > 0)
		{
			animation_frame_ticks += ticks;
			animation_frame_ticks %= TICKS_PER_FRAME_MAP_PART * type->animation_frames.size();
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

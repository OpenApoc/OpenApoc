#include "game/state/battle/battlemappart.h"
#include "game/state/battle/battledoor.h"
#include "game/state/battle/battleitem.h"
#include "game/state/battle/battlemappart_type.h"
#include "game/state/gamestate.h"
#include "game/state/tileview/tile.h"
#include "game/state/tileview/tileobject_battlemappart.h"

namespace OpenApoc
{

int BattleMapPart::getMaxFrames()
{
	return alternative_type ? alternative_type->animation_frames.size()
	                        : type->animation_frames.size();
}

int BattleMapPart::getAnimationFrame()
{
	if (isDoor())
	{
		return std::min(getMaxFrames() - 1, getDoor()->getAnimationFrame());
	}
	else
	{
		return type->animation_frames.size() == 0 ? -1 : animation_frame_ticks /
		                                                     TICKS_PER_FRAME_MAP_PART;
	}
}

sp<BattleDoor> BattleMapPart::getDoor()
{
	auto b = battle.lock();
	if (!b) //|| doorID == -1 ||  b->doors.size() >= doorID // <-- need to check this too?
	{
		LogError("getDoor - Battle disappeared!");
		return nullptr;
	}
	return b->doors[doorID];
}

void BattleMapPart::handleCollision(GameState &state, Collision &c)
{
	// FIXME: Proper damage application
	//
	// - if (not enough damage to type) then return

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
		if (this->initialPosition.z == 0 && this->type->type == BattleMapPartType::Type::Ground)
		{
			auto b = battle.lock();
			if (!b)
			{
				LogError("handleCollision - Battle disappeared!");
				return;
			}
			this->type = b->battle_map->destroyed_ground_tile;
		}
		// Destroy map part
		else
		{
			this->tileObject->removeFromMap();
			this->tileObject.reset();

			ceaseSupportProvision();
		}
	}
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
	if (isDoor())
	{
		getDoor()->collapse();
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
		if (!isDoor() && type->animation_frames.size() > 0)
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

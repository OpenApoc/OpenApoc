#include "game/state/battle/battledoor.h"

#include "framework/framework.h"
#include "framework/sound.h"
#include "game/state/battle/battle.h"
#include "game/state/battle/battlemappart.h"
#include "game/state/battle/battleunit.h"
#include "game/state/gamestate.h"
#include "game/state/rules/battle/battlemapparttype.h"
#include "game/state/tilemap/tilemap.h"
#include "game/state/tilemap/tileobject_battlemappart.h"
#include "game/state/tilemap/tileobject_battleunit.h"

namespace OpenApoc
{

template <> sp<BattleDoor> StateObject<BattleDoor>::get(const GameState &state, const UString &id)
{
	auto it = state.current_battle->doors.find(id);
	if (it == state.current_battle->doors.end())
	{
		LogError("No agent_type matching ID \"%s\"", id);
		return nullptr;
	}
	return it->second;
}

template <> const UString &StateObject<BattleDoor>::getPrefix()
{
	static UString prefix = "BATTLEDOOR_";
	return prefix;
}
template <> const UString &StateObject<BattleDoor>::getTypeName()
{
	static UString name = "BattleDoor";
	return name;
}
template <>
const UString &StateObject<BattleDoor>::getId(const GameState &state, const sp<BattleDoor> ptr)
{
	static const UString emptyString = "";
	for (auto &a : state.current_battle->doors)
	{
		if (a.second == ptr)
			return a.first;
	}
	LogError("No BattleDoor matching pointer %p", static_cast<void *>(ptr.get()));
	return emptyString;
}

void BattleDoor::setDoorState(GameState &state, bool open)
{
	animationTicksRemaining = 0;
	this->open = open;
	for (auto &s : mapParts)
	{
		auto i = s.lock();
		if (!i)
		{
			continue;
		}
		if (i->alternative_type)
		{
			i->type = i->alternative_type;
		}
		if (open)
		{
			if (i->type->alternative_map_part)
			{
				i->alternative_type = i->type;
				i->type = i->type->alternative_map_part;
			}
			openTicksRemaining = TICKS_TO_STAY_OPEN;
		}
		else
		{
			// did everything already?
		}
	}

	// Trigger tile's parameters update
	for (auto &s : mapParts)
	{
		auto mp = s.lock();
		if (!mp || !mp->tileObject)
		{
			continue;
		}
		mp->tileObject->getOwningTile()->updateBattlescapeParameters();
		state.current_battle->queueVisionRefresh(mp->position);
	}
}

void BattleDoor::update(GameState &state, unsigned int ticks)
{
	if (!operational)
	{
		return;
	}

	// Update animation
	if (animationTicksRemaining > 0)
	{
		if (animationTicksRemaining > (int)ticks)
		{
			animationTicksRemaining -= (int)ticks;
		}
		else
		{
			setDoorState(state, !open);
		}
	}

	// Should this door stay open
	bool shouldStayOpen = false;
	for (auto &s : mapParts)
	{
		auto i = s.lock();
		if (!i)
		{
			continue;
		}
		int xDiff = right ? 0 : 1;
		int yDiff = right ? 1 : 0;
		auto tile = i->tileObject->getOwningTile();
		if (tile->doorOpeningUnitPresent ||
		    (tile->position.x - xDiff > 0 && tile->position.y - yDiff > 0 &&
		     tile->map
		         .getTile(tile->position.x - xDiff, tile->position.y - yDiff, tile->position.z)
		         ->doorOpeningUnitPresent))
		{
			shouldStayOpen = true;
			break;
		}
	}

	// Update open door timer
	if (shouldStayOpen)
	{
		if (open)
		{
			openTicksRemaining = TICKS_TO_STAY_OPEN;
		}
	}
	else
	{
		if (openTicksRemaining > 0)
		{
			openTicksRemaining = std::max(0, openTicksRemaining - (int)ticks);
		}
	}

	// Start closing door if need to
	if (open && openTicksRemaining == 0 && animationTicksRemaining == 0 && !shouldStayOpen)
	{
		animationTicksRemaining = animationFrameCount * TICKS_PER_FRAME_MAP_PART;
		playDoorSound();
	}

	// Start opening door if need to
	if (!open && animationTicksRemaining == 0 && shouldStayOpen)
	{
		animationTicksRemaining = animationFrameCount * TICKS_PER_FRAME_MAP_PART;
		playDoorSound();
	}
}

int BattleDoor::getAnimationFrame()
{
	if (animationTicksRemaining == 0)
		return -1;
	if (open)
		return (animationTicksRemaining + TICKS_PER_FRAME_MAP_PART - 1) / TICKS_PER_FRAME_MAP_PART -
		       1;
	else
		return animationFrameCount -
		       (animationTicksRemaining + TICKS_PER_FRAME_MAP_PART - 1) / TICKS_PER_FRAME_MAP_PART;
}

void BattleDoor::playDoorSound() { fw().soundBackend->playSample(doorSound, position); }
} // namespace OpenApoc

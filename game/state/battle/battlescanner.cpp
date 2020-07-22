#include "game/state/battle/battlescanner.h"
#include "game/state/battle/battleunit.h"
#include "game/state/gamestate.h"

namespace OpenApoc
{

BattleScanner::BattleScanner()
{
	movementTicks = std::vector<int>(MOTION_SCANNER_X * MOTION_SCANNER_Y, 0);
}

template <> const UString &StateObject<BattleScanner>::getPrefix()
{
	static UString prefix = "BATTLESCANNER_";
	return prefix;
}

template <> const UString &StateObject<BattleScanner>::getTypeName()
{
	static UString name = "BattleScanner";
	return name;
}

template <>
sp<BattleScanner> StateObject<BattleScanner>::get(const GameState &state, const UString &id)
{
	auto it = state.current_battle->scanners.find(id);
	if (it == state.current_battle->scanners.end())
	{
		LogError("No scanner type matching ID \"%s\"", id);
		return nullptr;
	}
	return it->second;
}

void BattleScanner::update(GameState &state, unsigned int ticks)
{
	static const Vec3<int> midPos = {MOTION_SCANNER_X / 2, MOTION_SCANNER_Y / 2, 0};

	updateTicksAccumulated += ticks;
	bool changed = false;
	while (updateTicksAccumulated >= TICKS_PER_SCANNER_UPDATE)
	{
		updateTicksAccumulated -= TICKS_PER_SCANNER_UPDATE;
		for (size_t i = 0; i < movementTicks.size(); i++)
		{
			if (movementTicks[i] > TICKS_PER_SCANNER_UPDATE)
			{
				movementTicks[i] -= TICKS_PER_SCANNER_UPDATE;
				changed = true;
			}
			else if (movementTicks[i] > 0)
			{
				movementTicks[i] = 0;
				changed = true;
			}
		}
	}

	Vec3<int> newPosition = holder->position;

	if (newPosition.x != lastPosition.x || newPosition.y != lastPosition.y)
	{
		// Shift everything
		changed = true;
		auto lastMovementTicks = movementTicks;
		/*movementTicks = std::vector<int>(MOTION_SCANNER_X * MOTION_SCANNER_Y, 0);
		auto posShift = newPosition - lastPosition;
		for (int x = 0; x < MOTION_SCANNER_X; x++)
		{
		    int oX = x + posShift.x;
		    for (int y = 0; y < MOTION_SCANNER_Y; y++)
		    {
		        int oY = y + posShift.y;
		        if (oX < 0 || oY < 0 || oX >= MOTION_SCANNER_X || oY >= MOTION_SCANNER_Y)
		        {
		            continue;
		        }
		        movementTicks[y * MOTION_SCANNER_X + x] = lastMovementTicks[oY * MOTION_SCANNER_X +
		oX];
		    }
		}*/

		// Introduce movement ticks for every unit
		for (auto &u : state.current_battle->units)
		{
			if (u.second->destroyed)
			{
				continue;
			}
			auto pos = (Vec3<int>)u.second->position - newPosition + midPos;
			if (pos.x < 0 || pos.y < 0 || pos.x >= MOTION_SCANNER_X || pos.y >= MOTION_SCANNER_Y)
			{
				continue;
			}
			movementTicks[pos.y * MOTION_SCANNER_X + pos.x] = TICKS_SCANNER_REMAIN_LIT;
		}

		lastPosition = holder->position;
	}

	if (changed)
	{
		version++;
	}
}

void BattleScanner::notifyMovement(Vec3<int> position)
{
	static const Vec3<int> midPos = {MOTION_SCANNER_X / 2, MOTION_SCANNER_Y / 2, 0};

	// FIXME: Fix display of big units
	auto pos = position - lastPosition + midPos;
	if (pos.x < 0 || pos.y < 0 || pos.x >= MOTION_SCANNER_X || pos.y >= MOTION_SCANNER_Y)
	{
		return;
	}
	version++;
	movementTicks[pos.y * MOTION_SCANNER_X + pos.x] = TICKS_SCANNER_REMAIN_LIT;
}
} // namespace OpenApoc

#include "game/state/city/economyinfo.h"
#include "game/state/gamestate.h"
#include "library/xorshift.h"
#include <algorithm>
#include <cmath>

namespace
{
// Match Lua's `math.round(x) = math.floor(x + 0.5)`: for the non-negative values this
// code deals with, round-half-away-from-zero (std::lround) is equivalent.
inline int roundToInt(float v) { return static_cast<int>(std::lround(v)); }
} // namespace

namespace OpenApoc
{
bool EconomyInfo::update(GameState &state, const bool xcom)
{
	if (currentPrice == 0)
	{
		currentPrice = basePrice;
	}
	// According to Wong's Guide, let's hope he's not wong here as he often is, lol
	const int week = static_cast<int>(state.gameTime.getWeek());
	if (weekAvailable > week)
	{
		return false;
	}
	// Depends if X-Com produced or not
	if (xcom)
	{
		// Stock update
		const int soldThisWeek = std::max(0, currentStock - lastStock);
		lastStock = currentStock;
		const int rnd = randBoundsExclusive(state.rng, 0, 100);
		if (rnd < 30)
		{
			currentStock = roundToInt(lastStock * 80.0f / 100.0f);
		}
		else if (rnd < 60)
		{
			currentStock = roundToInt(lastStock * 66.0f / 100.0f);
		}
		// Price update: multiplier is a percentage, computed in float to match Lua.
		if (soldThisWeek > 2 * maxStock)
		{
			currentPrice =
			    roundToInt(currentPrice * randBoundsInclusive(state.rng, 85, 95) / 100.0f);
		}
		else if (soldThisWeek > maxStock)
		{
			currentPrice =
			    roundToInt(currentPrice * randBoundsInclusive(state.rng, 90, 95) / 100.0f);
		}
		else if (soldThisWeek > maxStock / 2)
		{
			currentPrice =
			    roundToInt(currentPrice * randBoundsInclusive(state.rng, 95, 97) / 100.0f);
		}
		currentPrice = roundToInt(clamp(static_cast<float>(currentPrice), basePrice / 2.0f,
		                                static_cast<float>(basePrice)));
	}
	// Produced by someone else
	else if (weekAvailable != 0)
	{
		// Stock update
		lastStock = currentStock;
		const int averageStock = roundToInt((minStock + maxStock) / 2.0f);
		currentStock =
		    clamp(randBoundsInclusive(state.rng, 0, averageStock + lastStock), minStock, maxStock);
		// Price update
		if (week > 1)
		{
			if (currentStock > averageStock)
			{
				currentPrice =
				    roundToInt(currentPrice * randBoundsInclusive(state.rng, 97, 100) / 100.0f);
			}
			if (currentStock < averageStock)
			{
				currentPrice =
				    roundToInt(currentPrice * randBoundsInclusive(state.rng, 100, 103) / 100.0f);
			}
			currentPrice = roundToInt(
			    clamp(static_cast<float>(currentPrice), basePrice * 0.5f, basePrice * 2.0f));
		}
	}
	return week != 1 && week == weekAvailable;
}
} // namespace OpenApoc
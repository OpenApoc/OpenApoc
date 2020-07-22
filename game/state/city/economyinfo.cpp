#include "game/state/city/economyinfo.h"
#include "game/state/gamestate.h"
#include "library/xorshift.h"
#include <algorithm>

namespace OpenApoc
{
bool EconomyInfo::update(GameState &state, bool xcom)
{
	if (currentPrice == 0)
	{
		currentPrice = basePrice;
	}
	// According to Wong's Guide, let's hope he's not wong here as he often is, lol
	int week = state.gameTime.getWeek();
	if (weekAvailable > week)
	{
		return false;
	}
	// Depends if X-Com produced or not
	if (xcom)
	{
		// Stock update
		int soldThisWeek = std::max(0, currentStock - lastStock);
		lastStock = currentStock;
		int rnd = randBoundsExclusive(state.rng, 0, 100);
		if (rnd < 30)
		{
			currentStock = lastStock * 80 / 100;
		}
		else if (rnd < 60)
		{
			currentStock = lastStock * 66 / 100;
		}
		// Price update
		if (soldThisWeek > 2 * maxStock)
		{
			currentPrice = currentPrice * randBoundsInclusive(state.rng, 85, 5);
		}
		else if (soldThisWeek > maxStock)
		{
			currentPrice = currentPrice * randBoundsInclusive(state.rng, 90, 95);
		}
		else if (soldThisWeek > maxStock / 2)
		{
			currentPrice = currentPrice * randBoundsInclusive(state.rng, 95, 97);
		}
		currentPrice = clamp(currentPrice, basePrice / 2, basePrice);
	}
	// Produced by someone else
	else if (weekAvailable != 0)
	{
		// Stock update
		lastStock = currentStock;
		int averageStock = (minStock + maxStock) / 2;
		currentStock =
		    clamp(randBoundsInclusive(state.rng, 0, averageStock + lastStock), minStock, maxStock);
		// Price update
		if (week > 1)
		{
			if (currentStock > averageStock)
			{
				currentPrice = currentPrice * randBoundsInclusive(state.rng, 97, 100);
			}
			if (currentStock < averageStock)
			{
				currentPrice = currentPrice * randBoundsInclusive(state.rng, 100, 103);
			}
			currentPrice = clamp(currentPrice, basePrice / 2, basePrice * 2);
		}
	}
	return week != 1 && week == weekAvailable;
}
} // namespace OpenApoc
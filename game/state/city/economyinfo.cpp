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
		std::uniform_real_distribution<double> dist(1.0, 1.0);
		if (soldThisWeek > 2 * maxStock)
		{
			dist.param(decltype(dist)::param_type{0.85, 0.95});
		}
		else if (soldThisWeek > maxStock)
		{
			dist.param(decltype(dist)::param_type{0.9, 0.95});
		}
		else if (soldThisWeek > maxStock / 2)
		{
			dist.param(decltype(dist)::param_type{0.95, 0.97});
		}
		currentPrice = clamp(static_cast<int>(std::round(currentPrice * dist(state.rng))),
		                     basePrice / 2, basePrice);
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
		std::uniform_real_distribution<double> dist(1.0, 1.0);
		if (week > 1)
		{
			if (currentStock > averageStock)
			{
				dist.param(decltype(dist)::param_type{0.97, 1.0});
			}
			if (currentStock < averageStock)
			{
				dist.param(decltype(dist)::param_type{1.0, 1.03});
			}
			currentPrice = clamp(static_cast<int>(std::round(currentPrice * dist(state.rng))),
			                     basePrice / 2, basePrice * 2);
		}
	}
	return week != 1 && week == weekAvailable;
}
} // namespace OpenApoc

#pragma once

namespace OpenApoc
{
class GameState;

class EconomyInfo
{
  public:
	EconomyInfo() = default;
	int weekAvailable = 0;
	int basePrice = 0;
	int minStock = 0;
	int maxStock = 0;
	int currentPrice = 0;
	int currentStock = 0;
	int lastStock = 0;

	// Returns if new item
	bool update(GameState &state, bool xcom);
};
} // namespace OpenApoc
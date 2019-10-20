#pragma once
#include "game/state/battle/ai/unitai.h"

namespace OpenApoc
{

// AI that handles unit's behavior (Aggressive, Normal, Cautious)
class UnitAIBehavior : public UnitAI
{
  public:
	UnitAIBehavior();

	void reset(GameState &state, BattleUnit &u) override;
	std::tuple<AIDecision, bool> think(GameState &state, BattleUnit &u, bool interrupt) override;
};
} // namespace OpenApoc
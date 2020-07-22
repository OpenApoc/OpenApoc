#pragma once
#include "game/state/battle/ai/tacticalai.h"

namespace OpenApoc
{

// Vanilla AI that tries to replicate how aliens/civilians/security moved around the map
class TacticalAIVanilla : public TacticalAI
{
  public:
	TacticalAIVanilla();

	void beginTurnRoutine(GameState &state, StateRef<Organisation> o) override;
	std::list<std::pair<std::list<StateRef<BattleUnit>>, AIDecision>>
	think(GameState &state, StateRef<Organisation> o) override;

	std::tuple<std::list<StateRef<BattleUnit>>, sp<AIMovement>> getPatrolMovement(GameState &state,
	                                                                              BattleUnit &u);
};
} // namespace OpenApoc
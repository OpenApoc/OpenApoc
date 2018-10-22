#include "game/state/battle/ai/unitaibehavior.h"
#include "game/state/battle/ai/aidecision.h"
#include "game/state/gamestate.h"

namespace OpenApoc
{

namespace
{
static const std::tuple<AIDecision, bool> NULLTUPLE2 = std::make_tuple(AIDecision(), false);
}

UnitAIBehavior::UnitAIBehavior() { type = Type::Behavior; }

void UnitAIBehavior::reset(GameState &, BattleUnit &) {}

std::tuple<AIDecision, bool> UnitAIBehavior::think(GameState &state, BattleUnit &u, bool interrupt)
{
	std::ignore = state;
	std::ignore = u;
	std::ignore = interrupt;
	active = false;

	if (!active)
	{
		return NULLTUPLE2;
	}

	LogError("Implement Behavior AI");
	return NULLTUPLE2;
}
} // namespace OpenApoc
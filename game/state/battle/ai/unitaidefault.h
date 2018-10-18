#pragma once
#include "game/state/battle/ai/unitai.h"

namespace OpenApoc
{

// AI that handles unit's automatic actions (turning to attacker, visible enemy, firing held
// weapons)
class UnitAIDefault : public UnitAI
{
  public:
	UnitAIDefault();

	uint64_t ticksAutoTurnAvailable = 0;
	uint64_t ticksAutoTargetAvailable = 0;

	// Position of a person who attacked us since last think()
	Vec3<int> attackerPosition = {-1, -1, -1};

	void reset(GameState &state, BattleUnit &u) override;
	std::tuple<AIDecision, bool> think(GameState &state, BattleUnit &u, bool interrupt) override;

	void notifyUnderFire(Vec3<int> position) override;
	void notifyHit(Vec3<int> position) override;
};
} // namespace OpenApoc
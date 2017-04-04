#pragma once
#include "game/state/battle/ai/aidecision.h"
#include "library/sp.h"
#include "library/strings.h"
#include "library/vec.h"

namespace OpenApoc
{

class GameState;
class BattleUnit;

// Represents a logic that unit uses to decide it's automatic actions
class UnitAI
{
  public:
	enum class Type
	{
		LowMorale,
		Default,
		Behavior,
		Vanilla,
		Hardcore
	};
	Type type; // cannot hide because serializer won't work
	virtual const UString getName();

	// Wether AI is currently active
	bool active = false;

	virtual void reset(GameState &, BattleUnit &){};
	// Returns decision that was made, and wether we should stop going forward on the AI chain
	virtual std::tuple<AIDecision, bool> think(GameState &, BattleUnit &, bool interrupt)
	{
		return {};
	};

	virtual void notifyUnderFire(Vec3<int>){};
	virtual void notifyHit(Vec3<int>){};
	virtual void notifyEnemySpotted(Vec3<int>){};
};

class AIBlockUnit
{
  public:
	std::vector<sp<UnitAI>> aiList;

	uint64_t ticksLastThink = 0;
	uint64_t ticksLastOutOfOrderThink = 0;
	uint64_t ticksUntilReThink = 0;

	AIDecision think(GameState &state, BattleUnit &u, bool forceInterrupt = false);

	void init(GameState &state, BattleUnit &u);
	void reset(GameState &state, BattleUnit &u);

	void notifyUnderFire(Vec3<int> position);
	void notifyHit(Vec3<int> position);
	void notifyEnemySpotted(Vec3<int> position);
};
}
#pragma once

#include "float.h"
#include "game/state/battle/ai/aidecision.h"
#include "game/state/gametime.h"
#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/strings.h"
#include "library/vec.h"
#include <list>

namespace OpenApoc
{

class GameState;
class Organisation;

// Represents an AI that controls all units on the tactical as a whole
class TacticalAI
{
  public:
	enum class Type
	{
		Vanilla
	};
	Type type; // cannot hide because serializer won't work
	virtual ~TacticalAI() = default;
	const UString getName();

	virtual void reset(GameState &, StateRef<Organisation>){};
	virtual void beginTurnRoutine(GameState &, StateRef<Organisation>){};
	virtual std::list<std::pair<std::list<StateRef<BattleUnit>>, AIDecision>>
	think(GameState &, StateRef<Organisation>)
	{
		return {};
	};
};

// Represents tactical AI for every AI-controlled org in battle
class AIBlockTactical
{
  public:
	void init(GameState &state);
	void reset(GameState &state);
	std::list<std::pair<std::list<StateRef<BattleUnit>>, AIDecision>> think(GameState &state);
	void beginTurnRoutine(GameState &state, StateRef<Organisation> org);

	std::map<StateRef<Organisation>, sp<TacticalAI>> aiList;

	uint64_t ticksLastThink = 0;
	uint64_t ticksUntilReThink = 0;
};
} // namespace OpenApoc
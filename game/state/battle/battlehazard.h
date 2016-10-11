#pragma once

#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/vec.h"


namespace OpenApoc
{
class TileObjectBattleHazard;
class DoodadType;
class GameState;
class TileMap;

class BattleHazard
{

  public:
	Vec3<float> getPosition() const { return this->position; }

	Vec3<float> position;
	StateRef<DoodadType> doodadType;
	unsigned lifetime = 0;
	unsigned age = 0;

	void update(GameState &state, unsigned int ticks);

	BattleHazard() = default;
	~BattleHazard() = default;

	// Following members are not serialized, but rather are set up in the initBattle method

	sp<TileObjectBattleHazard> tileObject;

};
} // namespace OpenApoc

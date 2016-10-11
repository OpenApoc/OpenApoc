#pragma once

#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/vec.h"
#include <set>


namespace OpenApoc
{
class GameState;
class TileMap;
class Battle;
class BattleUnit;
class DamageType;
class BattleUnit;

class BattleExplosion
{
  public:
	
	Vec3<int> getPosition() const { return this->position; }
	Vec3<int> position;
	
	int ticksUntilExpansion = 0;
	std::set<Vec3<int>> locationsToExpand;

	StateRef<DamageType> damageType;
	int power = 0;
	int depletionRate = 0;
	StateRef<BattleUnit> ownerUnit;

	std::set<StateRef<BattleUnit>> affectedUnits;

	void update(GameState &state, unsigned int ticks);

	BattleExplosion(Vec3<int> position, StateRef<DamageType> damageType, int power, int depletionRate, StateRef<BattleUnit> ownerUnit = nullptr);
	BattleExplosion() = default;
	~BattleExplosion() = default;

};
} // namespace OpenApoc

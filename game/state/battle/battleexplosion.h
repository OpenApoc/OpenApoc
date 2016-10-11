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

class BattleExplosion : public std::enable_shared_from_this<BattleExplosion>
{
  public:
	void die(GameState &state);

	Vec3<int> getPosition() const { return this->position; }
	Vec3<int> position;

	bool nextExpandLimited = true;
	int ticksUntilExpansion = 0;
	std::set<std::pair<Vec3<int>, int>> locationsToExpand;
	std::set<Vec3<int>> locationsVisited;

	StateRef<DamageType> damageType;
	int depletionRate = 0;
	StateRef<BattleUnit> ownerUnit;

	std::set<StateRef<BattleUnit>> affectedUnits;

	void grow(GameState &state);
	void damage(GameState &state, const TileMap &map, Vec3<int> pos, int power);
	void expand(GameState &state, const TileMap &map, const Vec3<int> &from, const Vec3<int> &to,
	            int power);

	void update(GameState &state, unsigned int ticks);

	BattleExplosion(Vec3<int> position, StateRef<DamageType> damageType, int power,
	                int depletionRate, StateRef<BattleUnit> ownerUnit = nullptr);
	BattleExplosion() = default;
	~BattleExplosion() = default;
};
} // namespace OpenApoc

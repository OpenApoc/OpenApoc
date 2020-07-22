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
class Organisation;
class BattleUnit;

class BattleExplosion : public std::enable_shared_from_this<BattleExplosion>
{
  public:
	Vec3<int> getPosition() const { return this->position; }
	Vec3<int> position;

	int power = 0;
	int ticksUntilExpansion = 0;
	// Vector of sets of locations to expand.
	// Position in the vector determines how long must pass until expansion will happen
	// This depends on distance (if we went diagonally - we delay by 3, if linearly - by 2)
	// One value contains a point (where to deal damage and from where to expand) and two values
	// First value is how much damage (power) must be applied to the tile
	// Second value is how much damage must be propagated further
	// These differ because feature blocks damage going further but not applied to the tile
	std::vector<std::set<std::pair<Vec3<int>, Vec2<int>>>> locationsToExpand;
	bool damageInTheEnd = false;
	std::set<std::pair<Vec3<int>, int>> locationsToDamage;

	std::set<Vec3<int>> locationsVisited;

	StateRef<DamageType> damageType;
	int depletionRate = 0;
	StateRef<BattleUnit> ownerUnit;
	StateRef<Organisation> ownerOrganisation;

	std::set<StateRef<BattleUnit>> affectedUnits;

	void grow(GameState &state);
	void damage(GameState &state, const TileMap &map, Vec3<int> pos, int power);
	void expand(GameState &state, const TileMap &map, const Vec3<int> &from, const Vec3<int> &to,
	            int power);
	void die(GameState &state);

	void update(GameState &state, unsigned int ticks);

	BattleExplosion(Vec3<int> position, StateRef<DamageType> damageType, int power,
	                int depletionRate, bool damageInTheEnd, StateRef<Organisation> ownerOrg,
	                StateRef<BattleUnit> ownerUnit = nullptr);
	BattleExplosion() = default;
	~BattleExplosion() = default;
};
} // namespace OpenApoc

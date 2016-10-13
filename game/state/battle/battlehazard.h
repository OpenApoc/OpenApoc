#pragma once

#include "game/state/gametime.h"
#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/vec.h"

#define TICKS_PER_HAZARD_EFFECT 4 * TICKS_PER_SECOND
#define HAZARD_FRAME_COUNT 3
// FIXME: This is a MADE UP VALUE!
// Study the algorithm of spreading the hazards
#define HAZARD_SPREAD_CHANCE 10 // out of 100

namespace OpenApoc
{
class TileObjectBattleHazard;
class DamageType;
class HazardType;
class GameState;
class TileMap;

class BattleHazard : public std::enable_shared_from_this<BattleHazard>
{
  public:
	Vec3<float> getPosition() const { return this->position; }

	Vec3<float> position;
	StateRef<DamageType> damageType;
	StateRef<HazardType> hazardType;
	int power = 0;
	unsigned lifetime = 0;
	unsigned age = 0;
	unsigned int frame = 0;
	unsigned ticksUntilVisible = 0;
	unsigned ticksUntilNextFrameChange = 0;
	unsigned ticksUntilNextEffect = 0;

	bool expand(GameState &state, const TileMap &map, const Vec3<int> &to, int ttl);
	void grow(GameState &state);
	void applyEffect(GameState &state);
	void die(GameState &state, bool violently = true);

	void update(GameState &state, unsigned int ticks);

	BattleHazard() = default;
	BattleHazard(GameState &state, StateRef<DamageType> damageType);
	~BattleHazard() = default;

	// Following members are not serialized, but rather are set up in the initBattle method

	sp<TileObjectBattleHazard> tileObject;
};
} // namespace OpenApoc

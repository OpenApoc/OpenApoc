#pragma once

#include "game/state/aequipment.h"
#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/vec.h"
#include <set>

#define FALLING_ACCELERATION_ITEM 0.14285714f // 1/7th

namespace OpenApoc
{
class TileObjectShadow;
class TileObjectBattleItem;
class Collision;
class GameState;
class TileMap;
class Battle;
class BattleUnit;

class BattleItem : public std::enable_shared_from_this<BattleItem>
{

  public:
	sp<AEquipment> item;

	Vec3<float> getPosition() const { return this->position; }

	Vec3<float> position;
	Vec3<float> velocity;

	// Item can bounce once after being thrown
	bool bounced = false;

	bool supported = false;

	int ownerInvulnerableTicks = 0;

	void handleCollision(GameState &state, Collision &c);
	void die(GameState &state, bool violently);
	void update(GameState &state, unsigned int ticks);

	BattleItem() = default;
	~BattleItem() = default;

	void setPosition(const Vec3<float> &pos);

	Collision checkItemCollision(Vec3<float> previousPosition, Vec3<float> nextPosition);

	bool findSupport(bool emitSound = true, bool forced = false);

	// Following members are not serialized, but rather are set up in the initBattle method

	sp<TileObjectBattleItem> tileObject;
	sp<TileObjectShadow> shadowObject;
	wp<Battle> battle;
};
} // namespace OpenApoc

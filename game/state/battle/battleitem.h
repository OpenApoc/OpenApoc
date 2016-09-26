#pragma once

#include "game/state/aequipment.h"
#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/vec.h"
#include <set>

namespace OpenApoc
{
class TileObjectShadow;
class TileObjectBattleItem;
class Collision;
class GameState;
class TileMap;
class Battle;

class BattleItem : public std::enable_shared_from_this<BattleItem>
{

  public:
	sp<AEquipment> item;

	Vec3<float> getPosition() const { return this->position; }

	Vec3<float> position;
	Vec3<float> velocity;

	void handleCollision(GameState &state, Collision &c);
	void die(GameState &state, bool violently);
	void update(GameState &state, unsigned int ticks);

	BattleItem() = default;
	~BattleItem() = default;

	void setPosition(const Vec3<float> &pos);

	bool findSupport(bool emitSound = true, bool forced = false);

	// Following members are not serialized, but rather are set up in the initBattle method

	bool supported = false;
	sp<TileObjectBattleItem> tileObject;
	sp<TileObjectShadow> shadowObject;
	wp<Battle> battle;
};
} // namespace OpenApoc

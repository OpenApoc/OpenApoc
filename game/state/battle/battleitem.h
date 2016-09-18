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
class BattleStrategyIconList;

class BattleItem : public std::enable_shared_from_this<BattleItem>
{

  public:
	sp<AEquipment> item;

	Vec3<float> getPosition() const { return this->position; }

	Vec3<float> position;
	Vec3<float> velocity;

	bool supported = false;

	void handleCollision(GameState &state, Collision &c);
	void die(GameState &state, bool violently);
	void update(GameState &state, unsigned int ticks);

	// Following members are not serialized, but rather are set in initBattle method

	sp<TileObjectBattleItem> tileObject;
	sp<TileObjectShadow> shadowObject;
	wp<Battle> battle;
	sp<BattleStrategyIconList> strategy_icon_list;

	BattleItem() = default;
	~BattleItem() = default;

	void setPosition(const Vec3<float> &pos);
};
} // namespace OpenApoc

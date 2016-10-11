#pragma once

#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/vec.h"

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
class AEquipment;
class Image;
class DamageType;

class BattleItem : public std::enable_shared_from_this<BattleItem>
{

  public:
	sp<AEquipment> item;

	Vec3<float> getPosition() const { return this->position; }

	Vec3<float> position;
	Vec3<float> velocity;

	// Item can bounce once after being thrown
	bool bounced = false;

	bool falling = false;

	unsigned int ownerInvulnerableTicks = 0;

	unsigned int ticksUntilCollapse = 0;

	// Check if we are still supported, and collapse if not
	void tryCollapse();
	void collapse();

	// Returns true if sound and doodad were handled by it
	bool applyDamage(GameState &state, int power, StateRef<DamageType> damageType);

	void die(GameState &state, bool violently = true);
	void update(GameState &state, unsigned int ticks);

	BattleItem() = default;
	~BattleItem() = default;

	void setPosition(const Vec3<float> &pos);

	Collision checkItemCollision(Vec3<float> previousPosition, Vec3<float> nextPosition);

	// Following members are not serialized, but rather are set up in the initBattle method

	sp<Image> strategySprite;

	sp<TileObjectBattleItem> tileObject;
	sp<TileObjectShadow> shadowObject;

  private:
	bool findSupport();
	void getSupport();
};
} // namespace OpenApoc

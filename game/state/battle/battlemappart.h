#pragma once

#include "game/state/battle/battlemappart_type.h"
#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/vec.h"
#include <list>
#include <set>

#define TICKS_PER_FRAME_MAP_PART 8

#define FALLING_ACCELERATION_MAP_PART 0.16666667f // 1/6th

namespace OpenApoc
{
class Battle;
class Collision;
class TileObjectBattleMapPart;
class BattleItem;
class BattleDoor;

class BattleMapPart : public std::enable_shared_from_this<BattleMapPart>
{
  public:
	StateRef<BattleMapPartType> type;
	StateRef<BattleMapPartType> alternative_type;

	const Vec3<float> &getPosition() const { return this->position; }
	Vec3<float> position;
	void setPosition(const Vec3<float> &pos);

	unsigned int ticksUntilCollapse = 0;
	bool damaged = false;
	bool falling = false;
	float fallingSpeed = 0.0f;
	bool destroyed = false;
	bool providesHardSupport = false;
	StateRef<BattleDoor> door;
	
	bool supportedItems = false;
	std::list<std::pair<Vec3<int>, BattleMapPartType::Type>> supportedParts;

	// Ticks for animation of non-doors
	int animation_frame_ticks = 0;
	int getAnimationFrame();
	int getMaxFrames();

	// Returns true if sound and doodad were handled by it
	bool handleCollision(GameState &state, Collision &c);
	// Handles mappart ceasing to exist (fatal damage or fell on something)
	void die(GameState &state, bool violently = true);
	// Collapses mappart immediately
	void collapse();

	// Makes mappart stop being valid for support and collapse in 1 vanilla tick
	void queueCollapse(unsigned additionalDelay = 0);
	// Cancels queued collapse
	void cancelCollapse();
	// Wether mappart is queued to collapse
	bool willCollapse() const { return ticksUntilCollapse > 0; }

	sp<std::set<BattleMapPart*>> getSupportedParts();
	static void attemptReLinkSupports(sp<std::set<BattleMapPart*>> set);

	void ceaseDoorFunction();

	void update(GameState &state, unsigned int ticks);
	
	bool isAlive() const;

	~BattleMapPart() = default;

	// Following members are not serialized, but rather are set in initBattle method

	sp<TileObjectBattleMapPart> tileObject;

  private:
	friend class Battle;

	// Find map parts that support this one and set "supported" flag
	bool findSupport();
	// Cease providing or requiring support
	void ceaseSupportProvision();
};
}

#pragma once

#include "game/state/rules/battle/battlemapparttype.h"
#include "game/state/rules/supportedmappart.h"
#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/vec.h"
#include <list>
#include <set>

#define TICKS_PER_FRAME_MAP_PART 8
#define FALLING_ACCELERATION_MAP_PART 0.16666667f // 1/6th
#define FALLING_MAP_PART_DAMAGE_TO_UNIT 50

namespace OpenApoc
{
class Battle;
class Collision;
class TileObjectBattleMapPart;
class BattleItem;
class BattleDoor;
class TileMap;
class Organisation;

class BattleMapPart : public SupportedMapPart, public std::enable_shared_from_this<BattleMapPart>
{
  public:
	StateRef<BattleMapPartType> type;
	StateRef<BattleMapPartType> alternative_type;

	const Vec3<float> &getPosition() const { return this->position; }
	Vec3<float> position;
	void setPosition(GameState &state, const Vec3<float> &pos);

	unsigned int ticksUntilCollapse = 0;
	int burnTicksAccumulated = 0;
	bool damaged = false;
	bool falling = false;
	float fallingSpeed = 0.0f;
	bool destroyed = false;
	bool providesHardSupport = false;
	StateRef<BattleDoor> door;

	bool supportedItems = false;
	std::list<std::pair<Vec3<int>, BattleMapPartType::Type>> supportedParts;
	StateRef<Organisation> owner;

	// Ticks for animation of non-doors
	int animation_frame_ticks = 0;
	int getAnimationFrame();
	int getMaxFrames();

	// Returns true if sound and doodad were handled by it
	bool applyDamage(GameState &state, int power, StateRef<DamageType> damageType);
	// Called when map part is burnt by fire. Returns true if burn was successful (provided fuel for
	// the fire)
	bool applyBurning(GameState &state, int age);
	// Whether this map part can burn (due to resist, timer and already burnt state)
	bool canBurn(int age);
	// Returns true if sound and doodad were handled by it
	bool handleCollision(GameState &state, Collision &c);
	// Handles mappart ceasing to exist (fatal damage or fell on something)
	void die(GameState &state, bool explosive = false, bool violently = true);
	// Collapses mappart immediately
	void collapse(GameState &state);
	// Whether mappart is queued to collapse
	bool willCollapse() const { return ticksUntilCollapse > 0; }

	void ceaseDoorFunction();

	void update(GameState &state, unsigned int ticks);
	void updateFalling(GameState &state, unsigned int ticks);

	bool isAlive() const;

	~BattleMapPart() = default;

	// Following members are not serialized, but rather are set in initBattle method

	sp<TileObjectBattleMapPart> tileObject;

  private:
	friend class Battle;

	// Try to attach to at least something, called for unlinked map parts when map starts
	bool attachToSomething(bool checkType, bool checkHard);

	// Cease providing or requiring support
	void ceaseSupportProvision();

	// Supported map part code
  public:
	// Makes mappart stop being valid for support and collapse in 1 vanilla tick
	void queueCollapse(unsigned additionalDelay = 0) override;
	// Cancels queued collapse
	void cancelCollapse() override;
	// Get ticks until this collapses (used to space out collapses)
	unsigned int getTicksUntilCollapse() const override { return ticksUntilCollapse; };

	// Compiles a list of parts supported by this part
	// Using sp because we switch to a new one constantly in re-linking
	// Using set because we need to easily weed out duplicates
	sp<std::set<SupportedMapPart *>> getSupportedParts() override;

	// Clears parts supported by this
	void clearSupportedParts() override;

	// Find map parts that support this one and set "hard supported" flag where appropriate
	bool findSupport(bool allowClinging = true) override;

	// Supported map part code
  protected:
	// Cease using support
	void ceaseBeingSupported() override;

	//
	// Debug output
	//

	Vec3<int> getTilePosition() const override;
	const TileMap &getMap() const override;
	UString getId() const override;
	int getType() const override;
	UString getSupportString() const override;
};
} // namespace OpenApoc

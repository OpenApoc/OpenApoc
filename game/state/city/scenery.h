#pragma once

#include "game/state/rules/supportedmappart.h"
#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/vec.h"
#include <set>

#define FALLING_ACCELERATION_MAP_PART 0.16666667f // 1/6th

namespace OpenApoc
{

static const int ROAD_ARMOR = 20;
static const int SC_COLLISION_DAMAGE = 50;

class TileObjectScenery;
class SceneryTileType;
class Building;
class Collision;
class GameState;
class TileMap;
class Doodad;
class City;
class Organisation;

class Scenery : public SupportedMapPart, public std::enable_shared_from_this<Scenery>
{

  public:
	StateRef<SceneryTileType> type;

	Vec3<float> getPosition() const
	{
		// The "position" is the center, so offset by {0.5,0.5,0.5}
		Vec3<float> offsetPos = currentPosition;
		offsetPos += Vec3<float>{0.5f, 0.5f, 0.5f};
		return offsetPos;
	}

	Vec3<int> initialPosition = {0, 0, 0};
	Vec3<float> currentPosition = {0, 0, 0};
	void setPosition(const Vec3<float> &pos);
	unsigned int ticksUntilCollapse = 0;
	std::set<Vec3<int>> supportedParts;
	std::list<Vec3<int>> supportedBy;

	bool damaged = false;
	bool falling = false;
	float fallingSpeed = 0.0f;
	bool destroyed = false;
	int supportHardness = 0;

	// Update relation with attacker which killed or hit us
	void updateRelationWithAttacker(GameState &state, StateRef<Organisation> attackerOrg,
	                                bool killed);

	bool handleCollision(GameState &state, Collision &c);
	// Returns true if sound and doodad were handled by it
	bool applyDamage(GameState &state, int power, StateRef<Organisation> attackerOrg = nullptr);
	// Handles scenery ceasing to exist (fatal damage or fell on something)
	// Forced to destroy regardless of damaged types
	void die(GameState &state, bool forced = false);
	// Collapses mappart immediately
	void collapse(GameState &state);
	// Whether mappart is queued to collapse
	bool willCollapse() const { return ticksUntilCollapse > 0; }

	void update(GameState &state, unsigned int ticks);
	void updateFalling(GameState &state, unsigned int ticks);

	bool canRepair() const;
	void repair(GameState &state);

	bool isAlive() const;

	// Attaches to at least something nearby
	bool attachToSomething();

	// Following members are not serialized, but rather are set in City::initCity method

	sp<TileObjectScenery> tileObject;
	sp<Doodad> overlayDoodad;
	StateRef<Building> building;
	StateRef<City> city;

	Scenery() = default;
	~Scenery() = default;

  private:
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

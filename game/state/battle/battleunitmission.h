#pragma once

#include "game/state/agent.h"
#include "game/state/tileview/tile.h"
#include "library/sp.h"
#include "library/strings.h"
#include "library/vec.h"
#include <list>

namespace OpenApoc
{
class BattleUnit;
class Tile;
class TileMap;

class BattleUnitTileHelper : public CanEnterTileHelper
{
  private:
	TileMap &map;
	BattleUnit &u;

  public:
	BattleUnitTileHelper(TileMap &map, BattleUnit &u) : map(map), u(u) {}

	static float getDistanceStatic(Vec3<float> from, Vec3<float> to);
	float getDistance(Vec3<float> from, Vec3<float> to) const override;

	bool canEnterTile(Tile *from, Tile *to, bool demandGiveWay = false) const override;
	bool canEnterTile(Tile *from, Tile *to, float &cost, bool &doorInTheWay,
	                  bool demandGiveWay = false) const override;

	bool canEnterTile(Tile *from, Tile *to, bool ignoreUnits, bool demandGiveWay) const;
	bool canEnterTile(Tile *from, Tile *to, float &cost, bool &doorInTheWay, bool ignoreUnits,
	                  bool demandGiveWay) const;

	float pathOverheadAlloawnce() const override { return 1.25f; }
};

class BattleUnitMission
{
  private:
	// INTERNAL: This checks if mission is actually finished. Called by isFinished.
	// If it is finished, update() is called by isFinished so that any remaining work could be done
	bool isFinishedInternal(GameState &state, BattleUnit &u);
	// INTERNAL: Never called directly
	static BattleUnitMission *turn(BattleUnit &u, Vec3<float> from, Vec3<float> to, bool free,
	                               bool requireGoal);

	// Methods that calculate next action
	bool advanceAlongPath(GameState &state, BattleUnit &u, Vec3<float> &dest);
	bool advanceFacing(GameState &state, BattleUnit &u, Vec2<int> &dest);
	bool advanceBodyState(GameState &state, BattleUnit &u, BodyState targetState, BodyState &dest);

  public:
	enum class Type
	{
		GotoLocation,
		Snooze,
		RestartNextMission,
		AcquireTU,
		ChangeBodyState,
		ThrowItem,
		DropItem,
		Turn,
		ReachGoal,
		Teleport
	};

	// Methods used in pathfinding etc.
	void update(GameState &state, BattleUnit &u, unsigned int ticks, bool finished = false);
	bool isFinished(GameState &state, BattleUnit &u, bool callUpdateIfFinished = true);
	void start(GameState &state, BattleUnit &u);
	void setPathTo(BattleUnit &u, Vec3<int> target, int maxIterations = 1000);

	// Methods to request next action
	// Request next destination
	bool getNextDestination(GameState &state, BattleUnit &u, Vec3<float> &dest);
	// Request next facing
	bool getNextFacing(GameState &state, BattleUnit &u, Vec2<int> &dest);
	// Request next body state
	bool getNextBodyState(GameState &state, BattleUnit &u, BodyState &dest);

	// Spend agent TUs or append AcquireTU mission
	bool spendAgentTUs(GameState &state, BattleUnit &u, int cost, bool cancel = false);

	static int getBodyStateChangeCost(BattleUnit &u, BodyState from, BodyState to);
	static int getThrowCost(BattleUnit &u);
	static Vec2<int> getFacingStep(BattleUnit &u, Vec2<int> targetFacing);
	// Used to determine target facings
	static Vec2<int> getFacing(BattleUnit &u, Vec3<float> from, Vec3<float> to);
	static Vec2<int> getFacing(BattleUnit &u, Vec3<int> to);

	// Other agent control methods
	void makeAgentMove(BattleUnit &u);

	// Methods to create new missions
	static BattleUnitMission *gotoLocation(BattleUnit &u, Vec3<int> target, int facingDelta = 0,
	                                       bool demandGiveWay = false, bool allowSkipNodes = true,
	                                       int giveWayAttempts = 20, bool allowRunningAway = false);
	static BattleUnitMission *snooze(BattleUnit &u, unsigned int ticks);
	static BattleUnitMission *acquireTU(BattleUnit &u, unsigned int tu);
	static BattleUnitMission *changeStance(BattleUnit &u, BodyState state);
	static BattleUnitMission *throwItem(BattleUnit &u, sp<AEquipment> item, Vec3<int> target);
	static BattleUnitMission *dropItem(BattleUnit &u, sp<AEquipment> item);
	static BattleUnitMission *turn(BattleUnit &u, Vec2<int> target, bool free = false,
	                               bool requireGoal = true);
	static BattleUnitMission *turn(BattleUnit &u, Vec3<int> target, bool free = false,
	                               bool requireGoal = true);
	static BattleUnitMission *turn(BattleUnit &u, Vec3<float> target, bool free = false,
	                               bool requireGoal = false);
	static BattleUnitMission *restartNextMission(BattleUnit &u);
	static BattleUnitMission *reachGoal(BattleUnit &u);
	static BattleUnitMission *teleport(BattleUnit &u, sp<AEquipment> item, Vec3<int> target);

	UString getName();

	Type type = Type::GotoLocation;

	// GotoLocation, ThrowItem, ReachGoal
	Vec3<int> targetLocation = {0, 0, 0};

	// GotoLocation
	int facingDelta = 0;
	int giveWayAttemptsRemaining = 0;
	bool allowRunningAway = false;
	std::list<Vec3<int>> currentPlannedPath;
	// Unit will follow its path exactly without trying to skip nodes
	bool allowSkipNodes = false;
	// Unit will ignore static non-large units when pathfinding
	bool demandGiveWay = false;
	// Unit paid for movement before turning and will be refunded when actually moving
	int costPaidUpFront = 0;

	// Turn
	Vec2<int> targetFacing = {0, 0};
	bool requireGoal = false;
	bool freeTurn = false;

	// ThrowItem, DropItem, Teleport
	sp<AEquipment> item;

	// ThrorwItem
	float velocityXY = 0.0f;
	float velocityZ = 0.0f;

	// Snooze
	unsigned int timeToSnooze = 0;

	// AcquireTU
	unsigned int timeUnits = 0;

	// ChangeBodyState
	BodyState targetBodyState = BodyState::Downed;

	// Mission cancelled (due to unsufficient TUs or something else failing)
	bool cancelled = false;
};
} // namespace OpenApoc

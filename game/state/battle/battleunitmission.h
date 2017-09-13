#pragma once

#include "game/state/agent.h"
#include "game/state/tileview/tile.h"
#include "game/state/tileview/tileobject_battleunit.h"
#include "library/sp.h"
#include "library/strings.h"
#include "library/vec.h"
#include <list>

namespace OpenApoc
{
class BattleUnit;
class Tile;
class TileMap;
enum class BattleUnitType;

class BattleUnitTileHelper : public CanEnterTileHelper
{
  private:
	TileMap &map;
	bool large;
	bool flying;
	int maxHeight;
	sp<TileObjectBattleUnit> tileObject;

  public:
	BattleUnitTileHelper(TileMap &map, BattleUnit &u);
	BattleUnitTileHelper(TileMap &map, bool large, bool flying, bool allowJumping, int maxHeight,
	                     sp<TileObjectBattleUnit> tileObject);
	BattleUnitTileHelper(TileMap &map, BattleUnitType type, bool allowJumping = false);

	static float getDistanceStatic(Vec3<float> from, Vec3<float> to);
	static float getDistanceStatic(Vec3<float> from, Vec3<float> toStart, Vec3<float> toEnd);
	float getDistance(Vec3<float> from, Vec3<float> to) const override;
	float getDistance(Vec3<float> from, Vec3<float> toStart, Vec3<float> toEnd) const override;

	bool canEnterTile(Tile *from, Tile *to, bool ignoreStaticUnits = false,
	                  bool ignoreAllUnits = false) const override;
	bool canEnterTile(Tile *from, Tile *to, bool allowJumping, bool &jumped, float &cost,
	                  bool &doorInTheWay, bool ignoreStaticUnits = false,
	                  bool ignoreAllUnits = false) const override;

	float pathOverheadAlloawnce() const override { return 1.25f; }

	BattleUnitType getType() const;
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
	bool advanceBrainsucker(GameState &state, BattleUnit &u, Vec3<float> &dest);
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
		Teleport,
		Brainsuck,
		Jump
	};

	// Methods (main)

	// Called each owner's update, plus guaranteed to be called before finished/cancelled mission is
	// removed
	void update(GameState &state, BattleUnit &u, unsigned int ticks, bool finished = false);
	bool isFinished(GameState &state, BattleUnit &u, bool callUpdateIfFinished = true);
	void start(GameState &state, BattleUnit &u);
	void setPathTo(GameState &state, BattleUnit &u, Vec3<int> target);

	// Methods to request next action

	// Request next destination
	bool getNextDestination(GameState &state, BattleUnit &u, Vec3<float> &dest);
	// Request next facing
	bool getNextFacing(GameState &state, BattleUnit &u, Vec2<int> &dest);
	// Request next body state
	bool getNextBodyState(GameState &state, BattleUnit &u, BodyState &dest);
	// Get next movement mode, returns None if default
	MovementState getNextMovementState(GameState &state, BattleUnit &u);

	// Spend agent TUs or append AcquireTU mission
	bool spendAgentTUs(GameState &state, BattleUnit &u, int cost, bool cancel = false,
	                   bool ignoreKneelReserve = false, bool allowInterrupt = false);

	static int getTurnCost(BattleUnit &u);
	static int getBodyStateChangeCost(const BattleUnit &u, BodyState from, BodyState to);
	static Vec2<int> getFacingStep(BattleUnit &u, Vec2<int> targetFacing, int facingDelta = 0);
	// Used to determine target facings
	static Vec2<int> getFacing(BattleUnit &u, Vec3<float> from, Vec3<float> to,
	                           int facingDelta = 0);
	static Vec2<int> getFacing(BattleUnit &u, Vec3<int> to, int facingDelta = 0);
	// Used to help with deltas
	static int getFacingDelta(Vec2<int> curFacing, Vec2<int> tarFacing);

	// Methods to create new missions
	static BattleUnitMission *gotoLocation(BattleUnit &u, Vec3<int> target, int facingDelta = 0,
	                                       bool demandGiveWay = false, bool allowSkipNodes = true,
	                                       int giveWayAttempts = 20, bool allowRunningAway = false);
	static BattleUnitMission *snooze(BattleUnit &u, unsigned int ticks);
	static BattleUnitMission *acquireTU(BattleUnit &u, bool allowContinue = false);
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
	static BattleUnitMission *reachGoal(BattleUnit &u, int facingDelta = 0);
	static BattleUnitMission *teleport(BattleUnit &u, sp<AEquipment> item, Vec3<int> target);
	static BattleUnitMission *brainsuck(BattleUnit &u, StateRef<BattleUnit> target,
	                                    int facingDelta);
	static BattleUnitMission *jump(BattleUnit &u, Vec3<float> target,
	                               BodyState state = BodyState::Standing,
	                               bool requireFacing = true);

	UString getName();

	Type type = Type::GotoLocation;

	// GotoLocation, ThrowItem, ReachGoal
	Vec3<int> targetLocation = {0, 0, 0};

	// GotoLocation
	int facingDelta = 0;
	int giveWayAttemptsRemaining = 0;
	bool allowRunningAway = false;
	std::list<Vec3<int>> currentPlannedPath;
	int maxIterations = 0;
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
	bool allowContinue = false;

	// ChangeBodyState
	BodyState targetBodyState = BodyState::Downed;

	// Jump
	Vec3<float> jumpTarget = {0.0f, 0.0f, 0.0f};
	bool jumped = false;

	// Brainsuck
	StateRef<BattleUnit> targetUnit;
	unsigned int brainsuckTicksAccumulated = 0;
	unsigned int brainsuckSoundsPlayed = 0;

	// Mission cancelled (due to unsufficient TUs or something else failing)
	bool cancelled = false;
};
} // namespace OpenApoc

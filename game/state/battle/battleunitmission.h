#pragma once

#include "game/state/agent.h"
#include "game/state/tileview/tile.h"
#include "library/strings.h"
#include "library/vec.h"
#include <list>
#include <map>

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

	float getDistance(Vec3<float> from, Vec3<float> to) const override;

	bool canEnterTile(Tile *from, Tile *to, bool demandGiveWay = false) const override;
	bool canEnterTile(Tile *from, Tile *to, float &cost, bool &doorInTheWay,
	                  bool demandGiveWay = false) const override;

	bool canEnterTile(Tile *from, Tile *to, bool ignoreUnits, bool demandGiveWay) const;
	bool canEnterTile(Tile *from, Tile *to, float &cost, bool &doorInTheWay, bool ignoreUnits,
	                  bool demandGiveWay) const;

	float applyPathOverheadAllowance(float cost) const override;
};

class BattleUnitMission
{
  private:
	// INTERNAL: This checks if mission is actually finished. Called by isFinished.
	// If it is finished, update() is called by isFinished so that any remaining work could be done
	bool isFinishedInternal(GameState &state, BattleUnit &u);

  public:
	enum class MissionType
	{
		GotoLocation,
		Snooze,
		RestartNextMission,
		AcquireTU,
		ChangeBodyState,
		ThrowItem,
		DropItem,
		Turn,
		Fall,
		ReachGoal,
	};

	// Methods used in pathfinding etc.
	bool getNextDestination(GameState &state, BattleUnit &u, Vec3<float> &dest);
	void update(GameState &state, BattleUnit &u, unsigned int ticks, bool finished = false);
	bool isFinished(GameState &state, BattleUnit &u, bool callUpdateIfFinished = true);
	void start(GameState &state, BattleUnit &u);
	bool advanceAlongPath(GameState &state, Vec3<float> &dest, BattleUnit &u);
	bool getNextFacing(GameState &state, BattleUnit &u, Vec2<int> &dest);
	void setPathTo(BattleUnit &u, Vec3<int> target, int maxIterations = 1000);

	// TU methods
	bool spendAgentTUs(GameState &state, BattleUnit &u, int cost);

	// Other agent control methods
	void makeAgentMove(BattleUnit &u);

	// Methods to create new missions
	static BattleUnitMission *gotoLocation(BattleUnit &u, Vec3<int> target, int facingDelta = 0,
	                                       bool allowSkipNodes = true, int giveWayAttempts = 10,
	                                       bool demandGiveWay = false,
	                                       bool allowRunningAway = false);
	static BattleUnitMission *snooze(BattleUnit &u, unsigned int ticks);
	static BattleUnitMission *restartNextMission(BattleUnit &u);
	static BattleUnitMission *acquireTU(BattleUnit &u, unsigned int tu);
	static BattleUnitMission *changeStance(BattleUnit &u, AgentType::BodyState state);
	static BattleUnitMission *throwItem(BattleUnit &u, sp<AEquipment> item, Vec3<int> target);
	static BattleUnitMission *dropItem(BattleUnit &u, sp<AEquipment> item);
	static BattleUnitMission *turn(BattleUnit &u, Vec2<int> target);
	static BattleUnitMission *turn(BattleUnit &u, Vec3<int> target);
	static BattleUnitMission *turn(BattleUnit &u, Vec3<float> target);
	static BattleUnitMission *fall(BattleUnit &u);
	static BattleUnitMission *reachGoal(BattleUnit &u);

	UString getName();

	MissionType type = MissionType::GotoLocation;

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

	// Turn
	Vec2<int> targetFacing = {0, 0};
	bool requireGoal = false;

	// ThrowItem, DropItem
	sp<AEquipment> item;

	// ThrowItem
	bool throwFailed = false;

	// Snooze
	unsigned int timeToSnooze = 0;

	// AcquireTU
	unsigned int timeUnits = 0;

	// ChangeBodyState
	AgentType::BodyState bodyState = AgentType::BodyState::Downed;

  private:
	static BattleUnitMission *turn(BattleUnit &u, Vec3<float> from, Vec3<float> to,
	                               bool requireGoal);
};
} // namespace OpenApoc

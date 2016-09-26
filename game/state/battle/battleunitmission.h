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

	bool canEnterTile(Tile *from, Tile *to, bool demandGiveWay = false) const override
	{
		float nothing;
		bool none;
		return canEnterTile(from, to, nothing, none, false, demandGiveWay);
	}

	bool canEnterTile(Tile *from, Tile *to, float &cost, bool &doorInTheWay, bool demandGiveWay = false) const override
	{
		return canEnterTile(from, to, cost, doorInTheWay, false, demandGiveWay);
	}

	bool canEnterTile(Tile *from, Tile *to, bool ignoreUnits, bool demandGiveWay) const
	{
		float nothing;
		bool none;
		return canEnterTile(from, to, nothing, none, ignoreUnits, demandGiveWay);
	}

	bool canEnterTile(Tile *from, Tile *to, float &cost, bool &doorInTheWay, bool ignoreUnits,
	                  bool demandGiveWay) const;
};

class BattleUnitMission
{
  public:
	enum class MissionType
	{
		GotoLocation,
		Snooze,
		RestartNextMission,
		AcquireTU,
		ChangeBodyState,
		ThrowItem,
		Turn,
		Fall,
		ReachGoal,
	};

	// Methods used in pathfinding etc.
	bool getNextDestination(GameState &state, BattleUnit &u, Vec3<float> &dest);
	void update(GameState &state, BattleUnit &u, unsigned int ticks);
	bool isFinished(GameState &state, BattleUnit &u);
	void start(GameState &state, BattleUnit &u);
	void setPathTo(BattleUnit &u, Vec3<int> target, int maxIterations = 500);
	bool advanceAlongPath(GameState &state, Vec3<float> &dest, BattleUnit &u);
	bool getNextFacing(GameState &state, BattleUnit &u, Vec2<int> &dest);

	// TU methods
	bool spendAgentTUs(GameState &state, BattleUnit &u, int cost);

	// Other agent control methods
	void makeAgentMove(BattleUnit &u);

	// Methods to create new missions
	static BattleUnitMission *gotoLocation(BattleUnit &u, Vec3<int> target, int facingDelta = 0,
	                                       bool allowSkipNodes = true, int giveWayAttempts = 10,
	                                       bool demandGiveWay = false, bool allowRunningAway = false);
	static BattleUnitMission *snooze(BattleUnit &u, unsigned int ticks);
	static BattleUnitMission *restartNextMission(BattleUnit &u);
	static BattleUnitMission *acquireTU(BattleUnit &u, unsigned int tu);
	static BattleUnitMission *changeStance(BattleUnit &u, AgentType::BodyState state);
	static BattleUnitMission *throwItem(BattleUnit &u, sp<AEquipment> item);
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
	// Unit will follow its path exactly without trying to skip nodes
	bool allowSkipNodes = false;
	// Unit will ignore static non-large units when pathfinding
	bool demandGiveWay = false;
	// Turn
	Vec2<int> targetFacing = {0, 0};
	bool requireGoal = false;
	// ThrowItem
	sp<AEquipment> item;
	// Snooze
	unsigned int timeToSnooze = 0;
	// AcquireTU
	unsigned int timeUnits = 0;
	// ChangeBodyState
	AgentType::BodyState bodyState = AgentType::BodyState::Downed;

	std::list<Vec3<int>> currentPlannedPath;

  private:
	static BattleUnitMission *turn(BattleUnit &u, Vec3<float> from, Vec3<float> to,
	                               bool requireGoal);
};
} // namespace OpenApoc

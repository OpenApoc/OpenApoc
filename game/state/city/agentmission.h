#pragma once

#include "game/state/stateobject.h"
#include "game/state/tileview/tile.h"
#include "library/strings.h"
#include "library/vec.h"
#include <list>
#include <map>

namespace OpenApoc
{

static const int TELEPORTER_SPREAD = 10;

class Agent;
class Tile;
class TileMap;
class Building;
class UString;

class AgentTileHelper : public CanEnterTileHelper
{
  private:
	TileMap &map;
	Agent &a;

  public:
	AgentTileHelper(TileMap &map, Agent &a);

	bool canEnterTile(Tile *from, Tile *to, bool ignoreStaticUnits = false,
	                  bool ignoreAllUnits = false) const override;

	float pathOverheadAlloawnce() const override;

	// Support 'from' being nullptr for if a vehicle is being spawned in the map
	bool canEnterTile(Tile *from, Tile *to, bool, bool &, float &cost, bool &, bool,
	                  bool) const override;

	float getDistance(Vec3<float> from, Vec3<float> to) const override;

	float getDistance(Vec3<float> from, Vec3<float> toStart, Vec3<float> toEnd) const override;

};

class AgentMission
{
  private:
	bool isFinishedInternal(GameState &state, Agent &a);

	bool teleportCheck(GameState &state, Agent &a);

  public:
	AgentMission() = default;

	// Methods used in pathfinding etc.
	bool getNextDestination(GameState &state, Agent &a, Vec3<float> &destPos);
	void update(GameState &state, Agent &a, unsigned int ticks, bool finished = false);
	bool isFinished(GameState &state, Agent &a, bool callUpdateIfFinished = true);
	void start(GameState &state, Agent &a);
	void setPathTo(GameState &state, Agent &a, Vec3<int> target, int maxIterations = 25,
	               bool checkValidity = true, bool giveUpIfInvalid = false);
	bool advanceAlongPath(GameState &state, Agent &a, Vec3<float> &destPos);
	
	// Methods to create new missions
	static AgentMission *gotoBuilding(GameState &state, Agent &a, StateRef<Building> target,
						bool allowTeleporter = false);
	static AgentMission *awaitPickup(GameState &state, Agent &a);
	static AgentMission *snooze(GameState &state, Agent &a, unsigned int ticks);
	static AgentMission *restartNextMission(GameState &state, Agent &a);
	static AgentMission *teleport(GameState &state, Agent &a, StateRef<Building> b);
	UString getName();

	enum class MissionType
	{
		GotoBuilding,
		AwaitPickup,
		RestartNextMission,
		Snooze,
		Teleport
	};

	MissionType type = MissionType::GotoLocation;

	//  GotoBuilding
	bool allowTeleporter = false;
	// GotoBuilding AttackBuilding Land Infiltrate
	StateRef<Building> targetBuilding;
	// AwaitPickup, Snooze
	unsigned int timeToSnooze = 0;
	
	std::list<Vec3<int>> currentPlannedPath;
};
} // namespace OpenApoc

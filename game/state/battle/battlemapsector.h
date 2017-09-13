#pragma once

#include "game/state/organisation.h"
#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/strings.h"
#include "library/vec.h"
#include <list>
#include <map>

namespace OpenApoc
{

class BattleMapSectorTiles;
class BattleMapPartType;

enum class SpawnType
{
	Player,
	Enemy,
	Civilian
};

class BattleMapSector : public StateObject
{
	STATE_OBJECT(BattleMapSector)
  public:
	BattleMapSector();
	~BattleMapSector() override = default;

	class LineOfSightBlock
	{
	  public:
		// Inclusive lower boundary
		Vec3<int> start;
		// Exclusive upper boundary
		Vec3<int> end;

		int ai_patrol_priority = 0;
		int ai_target_priority = 0;

		SpawnType spawn_type = SpawnType::Player;
		int spawn_priority = 0;
		bool low_priority = false;
		bool also_allow_civilians = false;
		bool spawn_large_units = false;
		bool spawn_walking_units = false;

		bool contains(Vec3<int> tile);
		bool contains(Vec3<float> position);

		sp<LineOfSightBlock> clone(Vec3<int> shift);
	};

	Vec3<int> size;
	int occurrence_min = 0;
	int occurrence_max = 0;

	UString sectorTilesName;
	up<BattleMapSectorTiles> tiles;

	std::map<StateRef<AgentType>, std::list<Vec3<int>>> spawnLocations;
};

class BattleMapSectorTiles
{
  public:
	std::list<sp<BattleMapSector::LineOfSightBlock>> losBlocks;

	std::map<Vec3<int>, Organisation::LootPriority> loot_locations;
	std::map<StateRef<AgentType>, std::list<Vec3<int>>> turretLocations;

	std::map<Vec3<int>, StateRef<BattleMapPartType>> initial_grounds;
	std::map<Vec3<int>, StateRef<BattleMapPartType>> initial_left_walls;
	std::map<Vec3<int>, StateRef<BattleMapPartType>> initial_right_walls;
	std::map<Vec3<int>, StateRef<BattleMapPartType>> initial_features;

	static UString getMapSectorPath();

	// high level api for loading map sectors
	bool loadSector(GameState &state, const UString &path);

	// high level api for saving map sectors
	bool saveSector(const UString &path, bool pack = true, bool pretty = false);
};
}

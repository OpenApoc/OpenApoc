#pragma once
#include "game/state/battlemappart_type.h"
#include "game/state/organisation.h"
#include "game/state/stateobject.h"
#include "library/sp.h"

namespace OpenApoc
{
class BattleMapSector
{
  public:
	BattleMapSector();
	~BattleMapSector() = default;

	class LineOfSightBlock
	{
	  public:
		enum class SpawnType
		{
			Player,
			Enemy,
			Civilian
		};

		Vec3<int> start;
		Vec3<int> end;

		int ai_patrol_priority = 0;
		int ai_target_priority = 0;

		SpawnType spawn_type = SpawnType::Player;
		int spawn_priority = 0;
		bool spawn_large_units = false;
		bool spawn_walking_units = false;

		bool contains(Vec3<int> tile)
		{
			return tile.x >= start.x && tile.y >= start.y && tile.z >= start.z && tile.x <= end.x &&
			       tile.y <= end.y && tile.z <= end.z;
		}
	};

	Vec3<int> size;
	int occurrence_min = 0;
	int occurrence_max = 0;

	std::list<sp<LineOfSightBlock>> los_blocks;

	std::map<Vec3<int>, Organisation::LootPriority> loot_locations;

	std::map<Vec3<int>, StateRef<BattleMapPartType>> initial_grounds;
	std::map<Vec3<int>, StateRef<BattleMapPartType>> initial_left_walls;
	std::map<Vec3<int>, StateRef<BattleMapPartType>> initial_right_walls;
	std::map<Vec3<int>, StateRef<BattleMapPartType>> initial_scenery;
};
}

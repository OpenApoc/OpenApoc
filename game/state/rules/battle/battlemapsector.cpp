#include "game/state/rules/battle/battlemapsector.h"
#include "framework/framework.h"
#include "game/state/gamestate.h"
#include "game/state/rules/battle/battlemap.h"

namespace OpenApoc
{

bool BattleMapSector::LineOfSightBlock::contains(Vec3<int> tile)
{
	return tile.x >= start.x && tile.y >= start.y && tile.z >= start.z && tile.x < end.x &&
	       tile.y < end.y && tile.z < end.z;
}

bool BattleMapSector::LineOfSightBlock::contains(Vec3<float> position)
{
	return position.x >= start.x && position.y >= start.y && position.z >= start.z &&
	       position.x < end.x && position.y < end.y && position.z < end.z;
}

sp<BattleMapSector::LineOfSightBlock> BattleMapSector::LineOfSightBlock::clone(Vec3<int> shift)
{
	auto b = mksp<BattleMapSector::LineOfSightBlock>();

	b->start = start + shift;
	b->end = end + shift;

	b->ai_patrol_priority = ai_patrol_priority;
	b->ai_target_priority = ai_target_priority;

	b->also_allow_civilians = also_allow_civilians;
	b->low_priority = low_priority;
	b->spawn_type = spawn_type;
	b->spawn_priority = spawn_priority;
	b->spawn_large_units = spawn_large_units;
	b->spawn_walking_units = spawn_walking_units;

	return b;
}

UString BattleMapSectorTiles::getMapSectorPath() { return fw().getDataDir() + "/maps"; }
} // namespace OpenApoc

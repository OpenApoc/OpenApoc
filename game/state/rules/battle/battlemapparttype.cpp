#include "game/state/rules/battle/battlemapparttype.h"
#include "game/state/gamestate.h"
#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/voxel.h"

namespace OpenApoc
{

sp<BattleMapPartType> BattleMapPartType::get(const GameState &state, const UString &id)
{
	auto it = state.battleMapTiles.find(id);
	if (it != state.battleMapTiles.end())
		return it->second;
	LogError("No battle map part type matching ID \"%s\"", id);

	return nullptr;
}

const UString &BattleMapPartType::getPrefix()
{
	static UString prefix = "BATTLEMAPPART_";
	return prefix;
}
const UString &BattleMapPartType::getTypeName()
{
	static UString name = "BattleMapPart";
	return name;
}
} // namespace OpenApoc

#pragma once
#include "game/state/battlemappart_type.h"
#include "framework/includes.h"
#include "game/state/gamestate.h"
#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/voxel.h"

namespace OpenApoc
{
const std::map<BattleMapPartType::Type, UString> BattleMapPartType::TypeMap = {
    {Type::Ground, "ground"},
    {Type::LeftWall, "leftwall"},
    {Type::RightWall, "rightwall"},
    {Type::Scenery, "scenery"},
};

template <>
sp<BattleMapPartType> StateObject<BattleMapPartType>::get(const GameState &state, const UString &id)
{

	auto it = state.battle.map_part_types.find(id);
	if (it != state.battle.map_part_types.end())
		return it->second;

	LogError("No battle map part type matching ID \"%s\"", id.cStr());
	return nullptr;
}

template <> const UString &StateObject<BattleMapPartType>::getPrefix()
{
	static UString prefix = "BATTLEMAPPART_";
	return prefix;
}
template <> const UString &StateObject<BattleMapPartType>::getTypeName()
{
	static UString name = "BattleMapPart";
	return name;
}
}
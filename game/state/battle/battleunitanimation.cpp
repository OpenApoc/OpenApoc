#include "game/state/battle/battleunitanimation.h"
#include "game/state/gamestate.h"

namespace OpenApoc
{
template <>
sp<BattleUnitAnimation> StateObject<BattleUnitAnimation>::get(const GameState &state,
                                                              const UString &id)
{
	auto it = state.battle_unit_animations.find(id);
	if (it == state.battle_unit_animations.end())
	{
		LogError("No BattleUnitAnimation matching ID \"%s\"", id.cStr());
		return nullptr;
	}
	return it->second;
}

template <> const UString &StateObject<BattleUnitAnimation>::getPrefix()
{
	static UString prefix = "BATTLEUNITIANIMATION_";
	return prefix;
}
template <> const UString &StateObject<BattleUnitAnimation>::getTypeName()
{
	static UString name = "BattleUnitAnimation";
	return name;
}
template <>
const UString &StateObject<BattleUnitAnimation>::getId(const GameState &state,
                                                       const sp<BattleUnitAnimation> ptr)
{
	static const UString emptyString = "";
	for (auto &a : state.battle_unit_animations)
	{
		if (a.second == ptr)
			return a.first;
	}
	LogError("No BattleUnitAnimation matching pointer %p", ptr.get());
	return emptyString;
}

int BattleUnitAnimation::getFrameCount(StateRef<AEquipmentType>, Vec2<int>, BattleUnit::Stance,
                                       BattleUnit::Stance, BattleUnit::HandState,
                                       BattleUnit::HandState, bool)
{
	LogWarning("Not implemented");
	return 0;
}

void BattleUnitAnimation::drawShadow(Renderer &, TileTransform &, Vec2<float>, TileViewMode,
                                     StateRef<BattleUnitImagePack>, StateRef<AEquipmentType>,
                                     Vec2<int>, BattleUnit::Stance, BattleUnit::Stance,
                                     BattleUnit::HandState, BattleUnit::HandState, bool, int)
{
	LogWarning("Not implemented");
}

void BattleUnitAnimation::drawUnit(Renderer &, TileTransform &, Vec2<float>, TileViewMode,
                                   StateRef<BattleUnitImagePack>, StateRef<BattleUnitImagePack>,
                                   StateRef<BattleUnitImagePack>, StateRef<BattleUnitImagePack>,
                                   StateRef<BattleUnitImagePack>, StateRef<AEquipmentType>,
                                   Vec2<int>, BattleUnit::Stance, BattleUnit::Stance,
                                   BattleUnit::HandState, BattleUnit::HandState, bool, int)
{
	LogWarning("Not implemented");
}
}

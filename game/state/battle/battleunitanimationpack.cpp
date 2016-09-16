#include "game/state/battle/BattleUnitAnimationPack.h"
#include "game/state/gamestate.h"

namespace OpenApoc
{
template <>
sp<BattleUnitAnimationPack> StateObject<BattleUnitAnimationPack>::get(const GameState &state,
                                                              const UString &id)
{
	auto it = state.battle_unit_animation_packs.find(id);
	if (it == state.battle_unit_animation_packs.end())
	{
		LogError("No BattleUnitAnimationPack matching ID \"%s\"", id.cStr());
		return nullptr;
	}
	return it->second;
}

template <> const UString &StateObject<BattleUnitAnimationPack>::getPrefix()
{
	static UString prefix = "BATTLEUNITIANIMATIONPACK_";
	return prefix;
}
template <> const UString &StateObject<BattleUnitAnimationPack>::getTypeName()
{
	static UString name = "BattleUnitAnimationPack";
	return name;
}
template <>
const UString &StateObject<BattleUnitAnimationPack>::getId(const GameState &state,
                                                       const sp<BattleUnitAnimationPack> ptr)
{
	static const UString emptyString = "";
	for (auto &a : state.battle_unit_animation_packs)
	{
		if (a.second == ptr)
			return a.first;
	}
	LogError("No BattleUnitAnimationPack matching pointer %p", ptr.get());
	return emptyString;
}

const UString BattleUnitAnimationPack::getNameFromID(UString id)
{
	static const UString emptyString = "";
	auto plen = getPrefix().length();
	if (id.length() > plen)
		return id.substr(plen, id.length() - plen);
	LogError("Invalid BattleUnitAnimationPack ID %s", id.cStr());
	return emptyString;
}

BattleUnitAnimationPack::AnimationEntry::Frame::InfoBlock::InfoBlock(int index, int offset_x, int offset_y) :
	index(index), offset_x(offset_x), offset_y(offset_y) {}


int BattleUnitAnimationPack::getFrameCountBody(StateRef<AEquipmentType> heldItem,
	AgentType::BodyState currentBody, AgentType::BodyState targetBody,
	AgentType::HandState currentHands,
	AgentType::MovementState movement,
	Vec2<int> direction)
{
	if (currentBody == targetBody)
		return standart_animations[heldItem ? (heldItem->two_handed ? ItemWieldMode::TwoHanded : ItemWieldMode::OneHanded) : ItemWieldMode::None][currentHands][movement][currentBody][direction]->frame_count;
	else
		return body_state_animations[heldItem ? (heldItem->two_handed ? ItemWieldMode::TwoHanded : ItemWieldMode::OneHanded) : ItemWieldMode::None][currentHands][movement][currentBody][targetBody][direction]->frame_count;
}

int BattleUnitAnimationPack::getFrameCountHands(StateRef<AEquipmentType> heldItem,
	AgentType::BodyState currentBody,
	AgentType::HandState currentHands, AgentType::HandState targetHands,
	AgentType::MovementState movement,
	Vec2<int> direction)
{
	if (currentHands == targetHands)
		return standart_animations[heldItem ? (heldItem->two_handed ? ItemWieldMode::TwoHanded : ItemWieldMode::OneHanded) : ItemWieldMode::None][currentHands][movement][currentBody][direction]->frame_count;
	else
		return hand_state_animations[heldItem ? (heldItem->two_handed ? ItemWieldMode::TwoHanded : ItemWieldMode::OneHanded) : ItemWieldMode::None][currentHands][targetHands][movement][currentBody][direction]->frame_count;
}


void BattleUnitAnimationPack::drawShadow(Renderer &r, TileTransform &transform, Vec2<float> screenPosition,
	TileViewMode mode, StateRef<BattleUnitImagePack> shadow,
	StateRef<AEquipmentType> heldItem, Vec2<int> facing,
	AgentType::BodyState currentBody, AgentType::BodyState targetBody,
	AgentType::HandState currentHands, AgentType::HandState targetHands,
	AgentType::MovementState movement,
	int body_animation_delay, int hands_animation_delay, int distance_passed)
{
	LogWarning("Not implemented");
}

void BattleUnitAnimationPack::drawUnit(Renderer &r, TileTransform &transform, Vec2<float> screenPosition,
	TileViewMode mode, StateRef<BattleUnitImagePack> body,
	StateRef<BattleUnitImagePack> legs, StateRef<BattleUnitImagePack> helmet,
	StateRef<BattleUnitImagePack> leftHand, StateRef<BattleUnitImagePack> rightHand,
	StateRef<AEquipmentType> heldItem, Vec2<int> facing,
	AgentType::BodyState currentBody, AgentType::BodyState targetBody,
	AgentType::HandState currentHands, AgentType::HandState targetHands,
	AgentType::MovementState movement,
	int body_animation_delay, int hands_animation_delay, int distance_passed)
{
	
	LogWarning("Not implemented");
}
}

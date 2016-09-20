#include "game/state/battle/battleunitanimationpack.h"
#include "game/state/gamestate.h"
#include "framework/renderer.h"

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
	// We're used to subtracting offests from positions, but vanilla uses an offset that should be added
	// therefore, we flip the sign here
	index(index), offset(Vec2<float>{-offset_x, -offset_y}) {} 

int BattleUnitAnimationPack::getFrameCountBody(StateRef<AEquipmentType> heldItem,
	AgentType::BodyState currentBody, AgentType::BodyState targetBody,
	AgentType::HandState currentHands,
	AgentType::MovementState movement,
	Vec2<int> facing)
{
	if (currentBody == targetBody)
		return standart_animations[heldItem ? (heldItem->two_handed ? ItemWieldMode::TwoHanded : ItemWieldMode::OneHanded) : ItemWieldMode::None][currentHands][movement][currentBody][facing]->frame_count;
	else
		return body_state_animations[heldItem ? (heldItem->two_handed ? ItemWieldMode::TwoHanded : ItemWieldMode::OneHanded) : ItemWieldMode::None][currentHands][movement][currentBody][targetBody][facing]->frame_count;
}

int BattleUnitAnimationPack::getFrameCountHands(StateRef<AEquipmentType> heldItem,
	AgentType::BodyState currentBody,
	AgentType::HandState currentHands, AgentType::HandState targetHands,
	AgentType::MovementState movement,
	Vec2<int> facing)
{
	if (currentHands == targetHands)
		return standart_animations[heldItem ? (heldItem->two_handed ? ItemWieldMode::TwoHanded : ItemWieldMode::OneHanded) : ItemWieldMode::None][currentHands][movement][currentBody][facing]->frame_count;
	else
		return hand_state_animations[heldItem ? (heldItem->two_handed ? ItemWieldMode::TwoHanded : ItemWieldMode::OneHanded) : ItemWieldMode::None][currentHands][targetHands][movement][currentBody][facing]->frame_count;
}


void BattleUnitAnimationPack::drawShadow(Renderer &r, Vec2<float> screenPosition,
	StateRef<BattleUnitImagePack> shadow,
	StateRef<AEquipmentType> heldItem, Vec2<int> facing,
	AgentType::BodyState currentBody, AgentType::BodyState targetBody,
	AgentType::HandState currentHands, AgentType::HandState targetHands,
	AgentType::MovementState movement,
	int body_animation_delay, int hands_animation_delay, int distance_travelled)
{
	// If we are calling this, then we have already ensured that object has shadows, 
	// and should not check for it again

	sp<AnimationEntry> e;
	int frame = -1;
	if (currentHands != targetHands)
	{
		e = hand_state_animations[heldItem ? (heldItem->two_handed ? ItemWieldMode::TwoHanded : ItemWieldMode::OneHanded) : ItemWieldMode::None][currentHands][targetHands][movement][currentBody][facing];
		frame = e->frame_count - hands_animation_delay;
	}
	else if (currentBody != targetBody)
	{
		e = body_state_animations[heldItem ? (heldItem->two_handed ? ItemWieldMode::TwoHanded : ItemWieldMode::OneHanded) : ItemWieldMode::None][currentHands][movement][currentBody][targetBody][facing];
		frame = e->frame_count - body_animation_delay;
	}
	else
	{
		e = standart_animations[heldItem ? (heldItem->two_handed ? ItemWieldMode::TwoHanded : ItemWieldMode::OneHanded) : ItemWieldMode::None][currentHands][movement][currentBody][facing];
		if (currentHands == AgentType::HandState::Firing)
			frame = e->frame_count - hands_animation_delay;
		else
			frame = distance_travelled % e->frame_count;
	}
	AnimationEntry::Frame::InfoBlock &b = e->frames[frame].unit_image_parts[AnimationEntry::Frame::UnitImagePart::Shadow];
	
	if (b.index == -1)
		return;
	
	r.draw(shadow->images[b.index], screenPosition - b.offset - shadow->image_offset);
}

void BattleUnitAnimationPack::drawUnit(Renderer &r, Vec2<float> screenPosition,
	StateRef<BattleUnitImagePack> body,
	StateRef<BattleUnitImagePack> legs, StateRef<BattleUnitImagePack> helmet,
	StateRef<BattleUnitImagePack> leftHand, StateRef<BattleUnitImagePack> rightHand,
	StateRef<AEquipmentType> heldItem, Vec2<int> facing,
	AgentType::BodyState currentBody, AgentType::BodyState targetBody,
	AgentType::HandState currentHands, AgentType::HandState targetHands,
	AgentType::MovementState movement,
	int body_animation_delay, int hands_animation_delay, int distance_travelled)
{
	sp<AnimationEntry> e;
	sp<AnimationEntry> e_legs;
	int frame = -1;
	int frame_legs = -1;
	if (currentHands != targetHands)
	{
		e = hand_state_animations[heldItem ? (heldItem->two_handed ? ItemWieldMode::TwoHanded : ItemWieldMode::OneHanded) : ItemWieldMode::None][currentHands][targetHands][movement][currentBody][facing];
		frame = e->frame_count - hands_animation_delay;
		if (e->is_overlay)
		{
			e_legs = standart_animations[heldItem ? (heldItem->two_handed ? ItemWieldMode::TwoHanded : ItemWieldMode::OneHanded) : ItemWieldMode::None][AgentType::HandState::AtEase][movement][currentBody][facing];
			frame_legs = distance_travelled % e_legs->frame_count;
		}
	}
	else if (currentBody != targetBody)
	{
		e = body_state_animations[heldItem ? (heldItem->two_handed ? ItemWieldMode::TwoHanded : ItemWieldMode::OneHanded) : ItemWieldMode::None][currentHands][movement][currentBody][targetBody][facing];
		frame = e->frame_count - body_animation_delay;

	}
	else
	{
		e = standart_animations[heldItem ? (heldItem->two_handed ? ItemWieldMode::TwoHanded : ItemWieldMode::OneHanded) : ItemWieldMode::None][currentHands][movement][currentBody][facing];
		if (currentHands == AgentType::HandState::Firing)
			frame = e->frame_count - hands_animation_delay;
		else
			frame = distance_travelled % e->frame_count;
		// Technically, if we're aiming, and this is overlay, we must always set frame to 0
		// But since frame_count is 1, the previous line attains the same result, so why bother
		if (e->is_overlay)
		{
			e_legs = standart_animations[heldItem ? (heldItem->two_handed ? ItemWieldMode::TwoHanded : ItemWieldMode::OneHanded) : ItemWieldMode::None][AgentType::HandState::AtEase][movement][currentBody][facing];
			frame_legs = distance_travelled % e_legs->frame_count;
		}
	}
	AnimationEntry::Frame &f = e->frames[frame];

	// Draw parts in order
	for (auto ie : f.unit_image_draw_order)
	{
		// Shadows are drawn elsewhere
		if (ie == AnimationEntry::Frame::UnitImagePart::Shadow)
			continue;

		// Pick proper animation info block in case of overlay
		AnimationEntry::Frame::InfoBlock *b = &f.unit_image_parts[ie];
		if (b->index == -1 && ie == AnimationEntry::Frame::UnitImagePart::Legs && frame_legs != -1)
		{
			b = &e_legs->frames[frame_legs].unit_image_parts[ie];
		}
		if (b->index == -1)
			continue;
		
		// Actually draw the part
		switch (ie)
		{
			case AnimationEntry::Frame::UnitImagePart::Body:
				if (!body)
					continue;
				r.draw(body->images[b->index], screenPosition - b->offset - body->image_offset);
				break;
			case AnimationEntry::Frame::UnitImagePart::Legs:
				if (!legs)
					continue;
				r.draw(legs->images[b->index], screenPosition - b->offset - legs->image_offset);
				break;
			case AnimationEntry::Frame::UnitImagePart::Helmet:
				if (!helmet)
					continue;
				r.draw(helmet->images[b->index], screenPosition - b->offset - helmet->image_offset);
				break;
			case AnimationEntry::Frame::UnitImagePart::LeftArm:
				if (!leftHand)
					continue;
				r.draw(leftHand->images[b->index], screenPosition - b->offset - leftHand->image_offset);
				break;
			case AnimationEntry::Frame::UnitImagePart::RightArm:
				if (!rightHand)
					continue;
				r.draw(rightHand->images[b->index], screenPosition - b->offset - rightHand->image_offset);
				break;
			case AnimationEntry::Frame::UnitImagePart::Weapon:
				if (!heldItem)
					continue;
				r.draw(heldItem->held_image_pack->images[b->index], screenPosition - b->offset - heldItem->held_image_pack->image_offset);
				break;
			// Travis complains I'm not handling "Shadow"
			default:
				break;
		}
	}
}
}

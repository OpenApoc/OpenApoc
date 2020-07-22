#include "game/state/rules/battle/battleunitanimationpack.h"
#include "framework/framework.h"
#include "framework/renderer.h"
#include "game/state/gamestate.h"
#include "game/state/rules/battle/battleunitimagepack.h"

namespace OpenApoc
{

UString BattleUnitAnimationPack::getAnimationPackPath() { return "/animationpacks"; }

template <>
sp<BattleUnitAnimationPack> StateObject<BattleUnitAnimationPack>::get(const GameState &state,
                                                                      const UString &id)
{
	auto it = state.battle_unit_animation_packs.find(id);
	if (it == state.battle_unit_animation_packs.end())
	{
		LogError("No BattleUnitAnimationPack matching ID \"%s\"", id);
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
	LogError("Invalid BattleUnitAnimationPack ID %s", id);
	return emptyString;
}

BattleUnitAnimationPack::AnimationEntry::Frame::InfoBlock::InfoBlock(int index, int offset_x,
                                                                     int offset_y)
    : // We're used to subtracting offests from positions, but vanilla uses an offset that should be
      // added
      // therefore, we flip the sign here
      index(index), offset(Vec2<float>{-offset_x, -offset_y})
{
}

int BattleUnitAnimationPack::getFrameCountBody(StateRef<AEquipmentType> heldItem,
                                               BodyState currentBody, BodyState targetBody,
                                               HandState currentHands, MovementState movement,
                                               Vec2<int> facing)
{
	sp<AnimationEntry> e;
	if (currentBody == targetBody)
	{
		AnimationKey key = {
		    heldItem ? (heldItem->two_handed ? ItemWieldMode::TwoHanded : ItemWieldMode::OneHanded)
		             : ItemWieldMode::None,
		    currentHands, movement, currentBody};

		e = standart_animations[key][facing];
	}
	else
	{
		ChangingBodyStateAnimationKey key = {
		    heldItem ? (heldItem->two_handed ? ItemWieldMode::TwoHanded : ItemWieldMode::OneHanded)
		             : ItemWieldMode::None,
		    currentHands, movement, currentBody, targetBody};
		e = body_state_animations[key][facing];
	}
	if (e)
		return e->frame_count;
	else
		return 0;
}

int BattleUnitAnimationPack::getFrameCountHands(StateRef<AEquipmentType> heldItem,
                                                BodyState currentBody, HandState currentHands,
                                                HandState targetHands, MovementState movement,
                                                Vec2<int> facing)
{
	sp<AnimationEntry> e;
	if (currentHands == targetHands)
	{
		AnimationKey key = {
		    heldItem ? (heldItem->two_handed ? ItemWieldMode::TwoHanded : ItemWieldMode::OneHanded)
		             : ItemWieldMode::None,
		    currentHands, movement, currentBody};

		e = standart_animations[key][facing];
	}
	else
	{
		ChangingHandAnimationKey key = {
		    heldItem ? (heldItem->two_handed ? ItemWieldMode::TwoHanded : ItemWieldMode::OneHanded)
		             : ItemWieldMode::None,
		    currentHands, targetHands, movement, currentBody};
		e = hand_state_animations[key][facing];
	}
	if (e)
		return e->frame_count;
	else
		return 0;
}

int BattleUnitAnimationPack::getFrameCountFiring(StateRef<AEquipmentType> heldItem,
                                                 BodyState currentBody, MovementState movement,
                                                 Vec2<int> facing)
{
	sp<AnimationEntry> e;
	{
		AnimationKey key = {
		    heldItem ? (heldItem->two_handed ? ItemWieldMode::TwoHanded : ItemWieldMode::OneHanded)
		             : ItemWieldMode::None,
		    HandState::Firing, movement, currentBody};
		e = standart_animations[key][facing];
	}
	if (e)
		return e->frame_count;
	else
		return 0;
}

void BattleUnitAnimationPack::drawShadow(
    Renderer &r, Vec2<float> screenPosition, StateRef<BattleUnitImagePack> shadow,
    StateRef<AEquipmentType> heldItem, Vec2<int> facing, BodyState currentBody,
    BodyState targetBody, HandState currentHands, HandState targetHands, MovementState movement,
    int body_animation_delay, int hands_animation_delay, int distance_travelled, bool visible)
{
	if (!visible)
	{
		return;
	}
	// If we are calling this, then we have already ensured that object has shadows,
	// and should not check for it again

	sp<AnimationEntry> e;
	int frame = -1;
	if (currentHands != targetHands)
	{
		ChangingHandAnimationKey key = {
		    heldItem ? (heldItem->two_handed ? ItemWieldMode::TwoHanded : ItemWieldMode::OneHanded)
		             : ItemWieldMode::None,
		    currentHands, targetHands, movement, currentBody};
		e = hand_state_animations[key][facing];
		if (!e)
		{
			LogWarning("Body %d %d Hands %d %d Movement %d Frame missing!", (int)currentBody,
			           (int)targetBody, (int)currentHands, (int)targetHands, (int)movement);
			return;
		}
		frame = e->frame_count - hands_animation_delay;
	}
	else if (currentBody != targetBody)
	{
		ChangingBodyStateAnimationKey key = {
		    heldItem ? (heldItem->two_handed ? ItemWieldMode::TwoHanded : ItemWieldMode::OneHanded)
		             : ItemWieldMode::None,
		    currentHands, movement, currentBody, targetBody};
		e = body_state_animations[key][facing];
		if (!e)
		{
			LogWarning("Body %d %d Hands %d %d Movement %d Frame missing!", (int)currentBody,
			           (int)targetBody, (int)currentHands, (int)targetHands, (int)movement);
			return;
		}
		frame = e->frame_count - body_animation_delay;
	}
	else
	{
		AnimationKey key = {
		    heldItem ? (heldItem->two_handed ? ItemWieldMode::TwoHanded : ItemWieldMode::OneHanded)
		             : ItemWieldMode::None,
		    currentHands, movement, currentBody};

		e = standart_animations[key][facing];
		if (!e)
		{
			LogWarning("Body %d %d Hands %d %d Movement %d Frame missing!", (int)currentBody,
			           (int)targetBody, (int)currentHands, (int)targetHands, (int)movement);
			return;
		}
		if (currentHands == HandState::Firing)
			frame = e->frame_count - hands_animation_delay;
		else
			frame = (distance_travelled * 100 / e->units_per_100_frames) % e->frame_count;
	}

	if ((int)e->frames.size() <= frame)
	{
		LogError("drawShadow: Frame missing?");
		return;
	}

	AnimationEntry::Frame::InfoBlock &b =
	    e->frames[frame].unit_image_parts[AnimationEntry::Frame::UnitImagePart::Shadow];

	if (b.index == -1)
		return;

	r.draw(shadow->images[b.index], screenPosition - b.offset - shadow->image_offset);
}

void draw(Renderer &r, sp<Image> sprite, Vec2<float> screenPosition, bool transparent)
{
	static const Colour COLOUR_TRANSPARENT = {255, 255, 255, 95};

	if (transparent)
	{
		r.drawTinted(sprite, screenPosition, COLOUR_TRANSPARENT);
	}
	else
	{
		r.draw(sprite, screenPosition);
	}
}

void BattleUnitAnimationPack::drawUnit(
    Renderer &r, Vec2<float> screenPosition, StateRef<BattleUnitImagePack> body,
    StateRef<BattleUnitImagePack> legs, StateRef<BattleUnitImagePack> helmet,
    StateRef<BattleUnitImagePack> leftHand, StateRef<BattleUnitImagePack> rightHand,
    StateRef<AEquipmentType> heldItem, Vec2<int> facing, BodyState currentBody,
    BodyState targetBody, HandState currentHands, HandState targetHands, MovementState movement,
    int body_animation_delay, int hands_animation_delay, int distance_travelled, int firingAngle,
    bool visible, bool stealth)
{
	if (!visible)
	{
		return;
	}

	sp<AnimationEntry> e;
	sp<AnimationEntry> e_legs;
	int frame = -1;
	int frame_legs = -1;
	if (currentHands != targetHands)
	{
		ChangingHandAnimationKey key = {
		    heldItem ? (heldItem->two_handed ? ItemWieldMode::TwoHanded : ItemWieldMode::OneHanded)
		             : ItemWieldMode::None,
		    currentHands, targetHands, movement, currentBody};
		e = hand_state_animations[key][facing];
		if (!e)
		{
			LogWarning("Body %d %d Hands %d %d Movement %d Frame missing!", (int)currentBody,
			           (int)targetBody, (int)currentHands, (int)targetHands, (int)movement);
			return;
		}
		frame = e->frame_count - hands_animation_delay;
		if (e->is_overlay)
		{
			AnimationKey standardKey = {heldItem ? (heldItem->two_handed ? ItemWieldMode::TwoHanded
			                                                             : ItemWieldMode::OneHanded)
			                                     : ItemWieldMode::None,
			                            HandState::AtEase, movement, currentBody};
			e_legs = standart_animations[standardKey][facing];
			frame_legs =
			    (distance_travelled * 100 / e_legs->units_per_100_frames) % e_legs->frame_count;
		}
	}
	else if (currentBody != targetBody)
	{
		ChangingBodyStateAnimationKey key = {
		    heldItem ? (heldItem->two_handed ? ItemWieldMode::TwoHanded : ItemWieldMode::OneHanded)
		             : ItemWieldMode::None,
		    currentHands, movement, currentBody, targetBody};
		e = body_state_animations[key][facing];
		if (!e)
		{
			LogWarning("Body %d %d Hands %d %d Movement %d Frame missing!", (int)currentBody,
			           (int)targetBody, (int)currentHands, (int)targetHands, (int)movement);
			return;
		}
		frame = e->frame_count - body_animation_delay;
	}
	else
	{
		if ((currentHands == HandState::Firing || currentHands == HandState::Aiming) &&
		    hasAlternativeFiringAnimations && firingAngle != 0)
		{
			e = alt_fire_animations[{heldItem ? (heldItem->two_handed ? ItemWieldMode::TwoHanded
			                                                          : ItemWieldMode::OneHanded)
			                                  : ItemWieldMode::None,
			                         currentHands, firingAngle, movement, currentBody}][facing];
			if (!e)
			{
				LogWarning("Body %d %d Hands %d %d Movement %d Frame missing!", (int)currentBody,
				           (int)targetBody, (int)currentHands, (int)targetHands, (int)movement);
				return;
			}
		}
		else
		{
			AnimationKey key = {heldItem ? (heldItem->two_handed ? ItemWieldMode::TwoHanded
			                                                     : ItemWieldMode::OneHanded)
			                             : ItemWieldMode::None,
			                    currentHands, movement, currentBody};
			e = standart_animations[key][facing];
			if (!e)
			{
				LogWarning("Body %d %d Hands %d %d Movement %d Frame missing!", (int)currentBody,
				           (int)targetBody, (int)currentHands, (int)targetHands, (int)movement);
				return;
			}
		}
		if (currentHands == HandState::Firing)
			frame = e->frame_count - hands_animation_delay;
		else if (movement != MovementState::None)
			frame = (distance_travelled * 100 / e->units_per_100_frames) % e->frame_count;
		else
			frame = body_animation_delay % e->frame_count;
		// Technically, if we're aiming, and this is overlay, we must always set frame to 0
		// But since frame_count is 1, the previous line attains the same result, so why bother
		if (e->is_overlay)
		{
			AnimationKey key = {heldItem ? (heldItem->two_handed ? ItemWieldMode::TwoHanded
			                                                     : ItemWieldMode::OneHanded)
			                             : ItemWieldMode::None,
			                    HandState::AtEase, movement, currentBody};
			e_legs = standart_animations[key][facing];
			frame_legs =
			    (distance_travelled * 100 / e_legs->units_per_100_frames) % e_legs->frame_count;
		}
	}

	if ((int)e->frames.size() <= frame)
	{
		LogError("drawUnit: body Frame missing?");
		return;
	}

	AnimationEntry::Frame &f = e->frames[frame];

	// Draw parts in order
	for (auto &ie : f.unit_image_draw_order)
	{
		// Shadows are drawn elsewhere
		if (ie == AnimationEntry::Frame::UnitImagePart::Shadow)
			continue;

		// Pick proper animation info block in case of overlay
		AnimationEntry::Frame::InfoBlock *b = &f.unit_image_parts[ie];
		if (b->index == -1 && ie == AnimationEntry::Frame::UnitImagePart::Legs && frame_legs != -1)
		{
			if ((int)e_legs->frames.size() <= frame_legs)
			{
				LogError("drawUnit: legs Frame missing?");
				return;
			}
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
				if ((int)body->images.size() <= b->index)
				{
					LogError("Missing image: body %s size %u index %d", body.id,
					         body->images.size(), b->index);
					break;
				}
				draw(r, body->images[b->index], screenPosition - b->offset - body->image_offset,
				     stealth);
				break;
			case AnimationEntry::Frame::UnitImagePart::Legs:
				if (!legs)
					continue;
				if ((int)legs->images.size() <= b->index)
				{
					LogError("Missing image: legs %s size %u index %d", legs.id,
					         legs->images.size(), b->index);
					break;
				}
				draw(r, legs->images[b->index], screenPosition - b->offset - legs->image_offset,
				     stealth);
				break;
			case AnimationEntry::Frame::UnitImagePart::Helmet:
				if (!helmet)
					continue;
				if ((int)helmet->images.size() <= b->index)
				{
					LogError("Missing image: helmet %s size %u index %d", helmet.id,
					         helmet->images.size(), b->index);
					break;
				}
				draw(r, helmet->images[b->index], screenPosition - b->offset - helmet->image_offset,
				     stealth);
				break;
			case AnimationEntry::Frame::UnitImagePart::LeftArm:
				if (!leftHand)
					continue;
				if ((int)leftHand->images.size() <= b->index)
				{
					LogError("Missing image: leftHand %s size %u index %d", leftHand.id,
					         leftHand->images.size(), b->index);
					break;
				}
				draw(r, leftHand->images[b->index],
				     screenPosition - b->offset - leftHand->image_offset, stealth);
				break;
			case AnimationEntry::Frame::UnitImagePart::RightArm:
				if (!rightHand)
					continue;
				if ((int)rightHand->images.size() <= b->index)
				{
					LogError("Missing image: rightHand %s size %u index %d", rightHand.id,
					         rightHand->images.size(), b->index);
					break;
				}
				draw(r, rightHand->images[b->index],
				     screenPosition - b->offset - rightHand->image_offset, stealth);
				break;
			case AnimationEntry::Frame::UnitImagePart::Weapon:
				if (!heldItem)
					continue;
				if ((int)heldItem->held_image_pack->images.size() <= b->index)
				{
					LogError("Missing image: heldItem %s pack %s size %u index %d", heldItem.id,
					         heldItem->held_image_pack.id, heldItem->held_image_pack->images.size(),
					         b->index);
					break;
				}
				draw(r, heldItem->held_image_pack->images[b->index],
				     screenPosition - b->offset - heldItem->held_image_pack->image_offset, stealth);
				break;
			// Travis complains I'm not handling "Shadow"
			case AnimationEntry::Frame::UnitImagePart::Shadow:
				break;
		}
	}
}
} // namespace OpenApoc

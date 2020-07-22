#include "framework/data.h"
#include "framework/framework.h"
#include "framework/palette.h"
#include "game/state/gamestate.h"
#include "game/state/rules/battle/battleunitanimationpack.h"
#include "game/state/shared/agent.h"
#include "tools/extractors/common/animation.h"
#include "tools/extractors/extractors.h"

namespace OpenApoc
{

sp<BattleUnitAnimationPack::AnimationEntry> makeUpQAnimationEntry(int from, int count, int fromB,
                                                                  int countB, Vec2<int> offset)
{
	auto e = mksp<BattleUnitAnimationPack::AnimationEntry>();
	bool noHead = count == 0;
	if (noHead)
	{
		from = fromB;
		count = countB;
	}

	for (int i = 0; i < count; i++)
	{

		e->frames.push_back(BattleUnitAnimationPack::AnimationEntry::Frame());
		for (int j = 1; j <= (noHead ? 1 : 2); j++)
		{
			int part_idx = j;
			int frame = from + i;
			auto part_type = BattleUnitAnimationPack::AnimationEntry::Frame::UnitImagePart::Shadow;
			switch (part_idx)
			{
				case 1:
					part_type = BattleUnitAnimationPack::AnimationEntry::Frame::UnitImagePart::Body;
					frame = fromB + i;
					while (frame >= fromB + countB)
					{
						frame -= countB;
					}
					break;
				case 2:
					part_type = BattleUnitAnimationPack::AnimationEntry::Frame::UnitImagePart::Legs;
					break;
				default:
					LogError("If you reached this then OpenApoc programmers made a mistake");
					break;
			}
			e->frames[i].unit_image_draw_order.push_back(part_type);
			e->frames[i].unit_image_parts[part_type] =
			    BattleUnitAnimationPack::AnimationEntry::Frame::InfoBlock(frame, offset.x,
			                                                              offset.y);
		}
	}

	e->is_overlay = false;
	e->frame_count = e->frames.size();
	e->units_per_100_frames = 100;

	return e;
}

void extractAnimationPackQInternal(sp<BattleUnitAnimationPack> p)
{
	Vec2<int> offset = {-60, -42};

	// Standing/Aiming/Firing state:
	p->standart_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
	                        BodyState::Standing}][{0, 1}] =
	    makeUpQAnimationEntry(0, 8, 0, 2, offset);
	p->standart_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
	                        BodyState::Standing}][{-1, 1}] =
	    makeUpQAnimationEntry(8, 8, 0, 2, offset);
	p->standart_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
	                        BodyState::Standing}][{-1, 0}] =
	    makeUpQAnimationEntry(16, 7, 0, 2, offset);
	p->standart_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
	                        BodyState::Standing}][{1, 0}] =
	    makeUpQAnimationEntry(23, 8, 0, 2, offset);
	p->standart_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
	                        BodyState::Standing}][{1, 1}] =
	    makeUpQAnimationEntry(31, 9, 0, 2, offset);
	// Aim
	p->standart_animations[{ItemWieldMode::None, HandState::Aiming, MovementState::None,
	                        BodyState::Standing}][{0, 1}] =
	    makeUpQAnimationEntry(0, 8, 0, 2, offset);
	p->standart_animations[{ItemWieldMode::None, HandState::Aiming, MovementState::None,
	                        BodyState::Standing}][{-1, 1}] =
	    makeUpQAnimationEntry(8, 8, 0, 2, offset);
	p->standart_animations[{ItemWieldMode::None, HandState::Aiming, MovementState::None,
	                        BodyState::Standing}][{-1, 0}] =
	    makeUpQAnimationEntry(16, 7, 0, 2, offset);
	p->standart_animations[{ItemWieldMode::None, HandState::Aiming, MovementState::None,
	                        BodyState::Standing}][{1, 0}] =
	    makeUpQAnimationEntry(23, 8, 0, 2, offset);
	p->standart_animations[{ItemWieldMode::None, HandState::Aiming, MovementState::None,
	                        BodyState::Standing}][{1, 1}] =
	    makeUpQAnimationEntry(31, 9, 0, 2, offset);
	// Fire
	p->standart_animations[{ItemWieldMode::None, HandState::Firing, MovementState::None,
	                        BodyState::Standing}][{0, 1}] =
	    makeUpQAnimationEntry(0, 8, 0, 2, offset);
	p->standart_animations[{ItemWieldMode::None, HandState::Firing, MovementState::None,
	                        BodyState::Standing}][{-1, 1}] =
	    makeUpQAnimationEntry(8, 8, 0, 2, offset);
	p->standart_animations[{ItemWieldMode::None, HandState::Firing, MovementState::None,
	                        BodyState::Standing}][{-1, 0}] =
	    makeUpQAnimationEntry(16, 7, 0, 2, offset);
	p->standart_animations[{ItemWieldMode::None, HandState::Firing, MovementState::None,
	                        BodyState::Standing}][{1, 0}] =
	    makeUpQAnimationEntry(23, 8, 0, 2, offset);
	p->standart_animations[{ItemWieldMode::None, HandState::Firing, MovementState::None,
	                        BodyState::Standing}][{1, 1}] =
	    makeUpQAnimationEntry(31, 9, 0, 2, offset);

	// Downed state:
	p->standart_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
	                        BodyState::Downed}][{0, 1}] = makeUpQAnimationEntry(0, 1, 0, 1, offset);
	p->standart_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
	                        BodyState::Downed}][{-1, 1}] =
	    makeUpQAnimationEntry(8, 1, 0, 1, offset);
	p->standart_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
	                        BodyState::Downed}][{-1, 0}] =
	    makeUpQAnimationEntry(16, 1, 0, 1, offset);
	p->standart_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
	                        BodyState::Downed}][{1, 0}] =
	    makeUpQAnimationEntry(23, 1, 0, 1, offset);
	p->standart_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
	                        BodyState::Downed}][{1, 1}] =
	    makeUpQAnimationEntry(31, 1, 0, 1, offset);

	// Dying state:
	p->body_state_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
	                          BodyState::Standing, BodyState::Dead}][{0, 1}] =
	    makeUpQAnimationEntry(40, 10, 0, 2, offset);
	p->body_state_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
	                          BodyState::Standing, BodyState::Dead}][{-1, 1}] =
	    makeUpQAnimationEntry(40, 10, 0, 2, offset);
	p->body_state_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
	                          BodyState::Standing, BodyState::Dead}][{-1, 0}] =
	    makeUpQAnimationEntry(40, 10, 0, 2, offset);
	p->body_state_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
	                          BodyState::Standing, BodyState::Dead}][{1, 0}] =
	    makeUpQAnimationEntry(40, 10, 0, 2, offset);
	p->body_state_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
	                          BodyState::Standing, BodyState::Dead}][{1, 1}] =
	    makeUpQAnimationEntry(40, 10, 0, 2, offset);

	// Dead state
	p->standart_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
	                        BodyState::Dead}][{0, 1}] = makeUpQAnimationEntry(49, 1, 0, 1, offset);
	p->standart_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
	                        BodyState::Dead}][{-1, 1}] = makeUpQAnimationEntry(49, 1, 0, 1, offset);
	p->standart_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
	                        BodyState::Dead}][{-1, 0}] = makeUpQAnimationEntry(49, 1, 0, 1, offset);
	p->standart_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
	                        BodyState::Dead}][{1, 0}] = makeUpQAnimationEntry(49, 1, 0, 1, offset);
}

void InitialGameStateExtractor::extractAnimationPackQ(sp<BattleUnitAnimationPack> p) const
{
	extractAnimationPackQInternal(p);
}
} // namespace OpenApoc

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

sp<BattleUnitAnimationPack::AnimationEntry>
makeUpChrysalisAnimationEntry(int from, int count, int fromB, int countB, bool bidirectional,
                              Vec2<int> offset, bool first = false)
{
	auto e = mksp<BattleUnitAnimationPack::AnimationEntry>();
	bool noHead = count == 0;
	if (noHead)
	{
		from = fromB;
		count = countB;
	}

	for (int i = 0; i < (bidirectional ? count * 2 : count); i++)
	{
		int f = (bidirectional && i >= count) ? count * 2 - 1 - i : i;

		e->frames.push_back(BattleUnitAnimationPack::AnimationEntry::Frame());
		for (int j = 1; j <= (noHead ? 1 : 2); j++)
		{
			int part_idx = j;
			int frame = from + f;
			auto part_type = BattleUnitAnimationPack::AnimationEntry::Frame::UnitImagePart::Shadow;
			switch (part_idx)
			{
				case 1:
					part_type = BattleUnitAnimationPack::AnimationEntry::Frame::UnitImagePart::Body;
					frame = fromB + f;
					while (frame >= fromB + countB)
					{
						frame -= countB;
					}
					break;
				case 2:
					part_type =
					    BattleUnitAnimationPack::AnimationEntry::Frame::UnitImagePart::Helmet;
					break;
				default:
					LogError("If you reached this then OpenApoc programmers made a mistake");
					break;
			}
			e->frames[i].unit_image_draw_order.push_back(part_type);
			e->frames[i].unit_image_parts[part_type] =
			    BattleUnitAnimationPack::AnimationEntry::Frame::InfoBlock(
			        frame, offset.x, (j == 2 ? (first ? -4 : 9) : 0) + offset.y);
		}
	}

	e->is_overlay = false;
	e->frame_count = e->frames.size();
	e->units_per_100_frames = 100;

	return e;
}

void extractAnimationPackChrysalisInternal(sp<BattleUnitAnimationPack> p, bool first)
{
	int x = first ? 0 : -1;
	int y = first ? 1 : 0;

	// Prone state:
	p->standart_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
	                        BodyState::Prone}][{x, y}] =
	    makeUpChrysalisAnimationEntry(first ? 0 : 5, 5, first ? 0 : 1, 1, true, {-17, -24}, first);

	// Dying state:
	p->body_state_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
	                          BodyState::Prone, BodyState::Dead}][{x, y}] =
	    makeUpChrysalisAnimationEntry(0, 0, first ? 2 : 8, 6, false, {-17, first ? -16 : -8});
	p->body_state_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
	                          BodyState::Downed, BodyState::Dead}][{x, y}] =
	    makeUpChrysalisAnimationEntry(0, 0, first ? 2 : 8, 6, false, {-17, first ? -16 : -8});

	// Dead state:
	p->standart_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
	                        BodyState::Dead}][{x, y}] =
	    makeUpChrysalisAnimationEntry(0, 0, first ? 7 : 13, 1, false, {-5, first ? -22 : -2});

	// Downed state:
	p->standart_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
	                        BodyState::Downed}][{x, y}] =
	    makeUpChrysalisAnimationEntry(first ? 0 : 5, 1, first ? 0 : 1, 1, true,
	                                  {-5, first ? -30 : -18}, first);
}

void InitialGameStateExtractor::extractAnimationPackChrysalis(sp<BattleUnitAnimationPack> p,
                                                              bool first) const
{
	extractAnimationPackChrysalisInternal(p, first);
}
} // namespace OpenApoc

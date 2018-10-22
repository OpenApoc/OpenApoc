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

sp<BattleUnitAnimationPack::AnimationEntry> makeUpEggAnimationEntry(int from, int count, int fromB,
                                                                    int countB, Vec2<int> offsetEgg,
                                                                    Vec2<int> offsetTube,
                                                                    bool first = false)
{
	auto e = mksp<BattleUnitAnimationPack::AnimationEntry>();
	bool noHead = count == 0;
	Vec2<int> offset;
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
					offset = offsetEgg;
					part_type = BattleUnitAnimationPack::AnimationEntry::Frame::UnitImagePart::Body;
					frame = fromB + i;
					while (frame >= fromB + countB)
					{
						frame -= countB;
					}
					break;
				case 2:
					offset = offsetTube;
					part_type =
					    BattleUnitAnimationPack::AnimationEntry::Frame::UnitImagePart::Helmet;
					break;
				default:
					LogError("If you reached this then OpenApoc programmers made a mistake");
					return e;
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

void extractAnimationPackEggInternal(sp<BattleUnitAnimationPack> p, bool first)
{
	static const std::map<Vec2<int>, int> offset_dir_map = {
	    {{0, -1}, 4}, {{1, -1}, 5}, {{1, 0}, 6},  {{1, 1}, 7},
	    {{0, 1}, 0},  {{-1, 1}, 1}, {{-1, 0}, 2}, {{-1, -1}, 3},
	};

	for (int x = -1; x <= 1; x++)
	{
		for (int y = -1; y <= 1; y++)
		{
			// 0, 0 facing does not exist
			if (x == 0 && y == 0)
				continue;

			// Standing state:
			p->standart_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
			                        BodyState::Standing}][{x, y}] =
			    makeUpEggAnimationEntry(offset_dir_map.at({x, y}), 1, first ? 0 : 1, 1, {0, 0},
			                            first ? Vec2<int>{0, 4} : Vec2<int>{-12, -8}, first);

			// Aiming state:
			p->standart_animations[{ItemWieldMode::None, HandState::Aiming, MovementState::None,
			                        BodyState::Standing}][{x, y}] =
			    makeUpEggAnimationEntry(offset_dir_map.at({x, y}), 1, first ? 0 : 1, 1, {0, 0},
			                            first ? Vec2<int>{0, 4} : Vec2<int>{-12, -8}, first);

			// Firing state:
			p->standart_animations[{ItemWieldMode::None, HandState::Firing, MovementState::None,
			                        BodyState::Standing}][{x, y}] =
			    makeUpEggAnimationEntry(offset_dir_map.at({x, y}), 1, first ? 0 : 1, 1, {0, 0},
			                            first ? Vec2<int>{0, 4} : Vec2<int>{-12, -8}, first);

			// Dying state:
			p->body_state_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
			                          BodyState::Standing, BodyState::Dead}][{x, y}] =
			    makeUpEggAnimationEntry(0, 0, first ? 2 : 9, 7, {0, 0}, {0, 0});
			p->body_state_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
			                          BodyState::Downed, BodyState::Dead}][{x, y}] =
			    makeUpEggAnimationEntry(0, 0, first ? 2 : 9, 7, {0, 0}, {0, 0});

			// Dead state:
			p->standart_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
			                        BodyState::Dead}][{x, y}] =
			    makeUpEggAnimationEntry(0, 0, first ? 8 : 15, 1, {0, 0}, {0, 0});

			// Downed state: (same as standing)
			p->standart_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
			                        BodyState::Downed}][{x, y}] =
			    makeUpEggAnimationEntry(offset_dir_map.at({x, y}), 1, first ? 0 : 1, 1, {0, 0},
			                            first ? Vec2<int>{0, 4} : Vec2<int>{-12, -8}, first);
		}
	}
}

void InitialGameStateExtractor::extractAnimationPackEgg(sp<BattleUnitAnimationPack> p,
                                                        bool first) const
{
	extractAnimationPackEggInternal(p, first);
}
} // namespace OpenApoc

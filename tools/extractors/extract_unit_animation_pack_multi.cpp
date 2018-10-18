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
Vec2<float> getLastFrameOffset(int x, int y)
{
	static const std::map<Vec2<int>, Vec2<float>> offset_dir_map = {
	    {{0, -1}, {12, -6}}, {{1, -1}, {24, 0}},  {{1, 0}, {12, 6}},    {{1, 1}, {0, 12}},
	    {{0, 1}, {-12, 6}},  {{-1, 1}, {-24, 0}}, {{-1, 0}, {-12, -6}}, {{-1, -1}, {0, -12}},
	};

	return offset_dir_map.at({x, y});
}

Vec2<float> getProneOffset(int x, int y)
{
	static const std::map<Vec2<int>, Vec2<float>> offset_dir_map = {
	    {{0, -1}, {-14, -12}}, {{1, -1}, {-14, -12}}, {{1, 0}, {-10, -4}},  {{1, 1}, {-24, -5}},
	    {{0, 1}, {-12, -6}},   {{-1, 1}, {-32, -12}}, {{-1, 0}, {-14, -5}}, {{-1, -1}, {-24, -18}},
	};

	return offset_dir_map.at({x, y});
}
Vec2<float> getDownOffset(int x, int y)
{
	static const std::map<Vec2<int>, Vec2<float>> offset_dir_map = {
	    {{0, -1}, {-11, 6}}, {{1, -1}, {-23, 0}}, {{1, 0}, {-14, -6}}, {{1, 1}, {0, -11}},
	    {{0, 1}, {12, -5}},  {{-1, 1}, {24, 0}},  {{-1, 0}, {12, 7}},  {{-1, -1}, {0, 13}},
	};

	return offset_dir_map.at({x, y}) + getProneOffset(x, y);
}

void extractAnimationPackMultiInternal(sp<BattleUnitAnimationPack> p,
                                       const std::vector<AnimationDataAD> &dataAD,
                                       const std::vector<AnimationDataUA> &dataUA,
                                       std::vector<AnimationDataUF> &dataUF, int x, int y,
                                       const InitialGameStateExtractor &e)
{
	// Units per 100 frames
	static const int pFrames = 300; // Prone

	// Standart animations
	{
		// Downed state: 0 first frame
		p->standart_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
		                        BodyState::Downed}][{x, y}] =
		    e.getAnimationEntry(dataAD, dataUA, dataUF, 0, {x, y}, 100, 1, true, false, false,
		                        getDownOffset(x, y), getDownOffset(x, y));

		// Dead state: 4
		p->standart_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
		                        BodyState::Dead}][{x, y}] =
		    e.getAnimationEntry(dataAD, dataUA, dataUF, 4, {x, y}, getDownOffset(x, y),
		                        getDownOffset(x, y));

		// Prone static/aiming: 0 first frame
		p->standart_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
		                        BodyState::Prone}][{x, y}] =
		    e.getAnimationEntry(dataAD, dataUA, dataUF, 0, {x, y}, 100, 1, true, false, false,
		                        getProneOffset(x, y), getProneOffset(x, y));
		p->standart_animations[{ItemWieldMode::None, HandState::Aiming, MovementState::None,
		                        BodyState::Prone}][{x, y}] =
		    e.getAnimationEntry(dataAD, dataUA, dataUF, 0, {x, y}, 100, 1, true, false, false,
		                        getProneOffset(x, y), getProneOffset(x, y));

		// Prone firing: 6
		p->standart_animations[{ItemWieldMode::None, HandState::Firing, MovementState::None,
		                        BodyState::Prone}][{x, y}] =
		    e.getAnimationEntry(dataAD, dataUA, dataUF, 6, {x, y}, getProneOffset(x, y),
		                        getProneOffset(x, y));

		// Kneeling static/aiming/attacking: 7
		p->standart_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
		                        BodyState::Kneeling}][{x, y}] =
		    e.getAnimationEntry(dataAD, dataUA, dataUF, 7, {x, y}, {0, 0}, {0, 0});
		p->standart_animations[{ItemWieldMode::None, HandState::Aiming, MovementState::None,
		                        BodyState::Kneeling}][{x, y}] =
		    e.getAnimationEntry(dataAD, dataUA, dataUF, 7, {x, y}, {0, 0}, {0, 0});
		p->standart_animations[{ItemWieldMode::None, HandState::Firing, MovementState::None,
		                        BodyState::Kneeling}][{x, y}] =
		    e.getAnimationEntry(dataAD, dataUA, dataUF, 7, {x, y}, {0, 0}, {0, 0});

		// Prone moving state: 0
		p->standart_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::Normal,
		                        BodyState::Prone}][{x, y}] =
		    e.getAnimationEntry(dataAD, dataUA, dataUF, 0, {x, y}, pFrames, false,
		                        getProneOffset(x, y), getProneOffset(x, y));
	}

	// Body state change animations
	{
		// Body Kneeling -> Prone animation: 2
		p->body_state_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
		                          BodyState::Kneeling, BodyState::Prone}][{x, y}] =
		    e.getAnimationEntry(dataAD, dataUA, dataUF, 2, {x, y}, getProneOffset(x, y),
		                        getProneOffset(x, y));

		// Body Prone -> Kneeling 1  (offset the last unkneeling frame)
		auto kneelingFrames = e.getAnimationEntry(dataAD, dataUA, dataUF, 1, {x, y},
		                                          getProneOffset(x, y), getProneOffset(x, y));
		kneelingFrames->frames[kneelingFrames->frame_count - 1]
		    .unit_image_parts[BattleUnitAnimationPack::AnimationEntry::Frame::UnitImagePart::Body]
		    .offset = -getLastFrameOffset(x, y);
		p->body_state_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
		                          BodyState::Prone, BodyState::Kneeling}][{x, y}] = kneelingFrames;

		// Body Prone -> Dead animation: 3
		p->body_state_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
		                          BodyState::Prone, BodyState::Dead}][{x, y}] =
		    e.getAnimationEntry(dataAD, dataUA, dataUF, 3, {x, y}, getProneOffset(x, y),
		                        getProneOffset(x, y));

		// Body Kneel -> Dead animation: 2+3
		p->body_state_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
		                          BodyState::Kneeling, BodyState::Dead}][{x, y}] =
		    e.combineAnimationEntries(
		        e.getAnimationEntry(dataAD, dataUA, dataUF, 2, {x, y}, getProneOffset(x, y),
		                            getProneOffset(x, y)),
		        e.getAnimationEntry(dataAD, dataUA, dataUF, 3, {x, y}, getProneOffset(x, y),
		                            getProneOffset(x, y)));
	}
}

void InitialGameStateExtractor::extractAnimationPackMulti(
    sp<BattleUnitAnimationPack> p, const std::vector<AnimationDataAD> &dataAD,
    const std::vector<AnimationDataUA> &dataUA, std::vector<AnimationDataUF> &dataUF) const
{
	for (int x = -1; x <= 1; x++)
	{
		for (int y = -1; y <= 1; y++)
		{
			// 0, 0 facing does not exist
			if (x == 0 && y == 0)
				continue;

			extractAnimationPackMultiInternal(p, dataAD, dataUA, dataUF, x, y, *this);
		}
	}
}
} // namespace OpenApoc

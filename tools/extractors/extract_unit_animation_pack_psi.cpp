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

void extractAnimationPackPsiInternal(sp<BattleUnitAnimationPack> p,
                                     const std::vector<AnimationDataAD> &dataAD,
                                     const std::vector<AnimationDataUA> &dataUA,
                                     std::vector<AnimationDataUF> &dataUF, int x, int y,
                                     const InitialGameStateExtractor &e)
{
	Vec2<int> offset = {-28, -46};
	// Units per 100 frames
	static const int wFrames = 300; // Walk

	// Standart animations
	{
		// Flying state: 0
		p->standart_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
		                        BodyState::Flying}][{x, y}] =
		    e.getAnimationEntry(dataAD, dataUA, dataUF, 0, {x, y}, offset, offset);

		// Downed/Dead state: 4
		p->standart_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
		                        BodyState::Downed}][{x, y}] =
		    e.getAnimationEntry(dataAD, dataUA, dataUF, 4, {x, y}, offset, offset);
		p->standart_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
		                        BodyState::Dead}][{x, y}] =
		    e.getAnimationEntry(dataAD, dataUA, dataUF, 4, {x, y}, offset, offset);

		// Flying walking/running states: 1
		p->standart_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::Normal,
		                        BodyState::Flying}][{x, y}] =
		    e.getAnimationEntry(dataAD, dataUA, dataUF, 1, {x, y}, wFrames, false, offset, offset);
		p->standart_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::Running,
		                        BodyState::Flying}][{x, y}] =
		    e.getAnimationEntry(dataAD, dataUA, dataUF, 1, {x, y}, wFrames, false, offset, offset);

		// Firing state: 3
		p->standart_animations[{ItemWieldMode::None, HandState::Firing, MovementState::None,
		                        BodyState::Flying}][{x, y}] =
		    e.getAnimationEntryDbl(dataAD, dataUA, dataUF, 3, {x, y}, 100, false, offset, offset);
		p->standart_animations[{ItemWieldMode::None, HandState::Firing, MovementState::Normal,
		                        BodyState::Flying}][{x, y}] =
		    e.getAnimationEntryDbl(dataAD, dataUA, dataUF, 3, {x, y}, 100, false, offset, offset);
		p->standart_animations[{ItemWieldMode::None, HandState::Firing, MovementState::Running,
		                        BodyState::Flying}][{x, y}] =
		    e.getAnimationEntryDbl(dataAD, dataUA, dataUF, 3, {x, y}, 100, false, offset, offset);
	}

	// Body state change animations
	{
		// Body Flying -> Downed/Dead animation: 2
		p->body_state_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
		                          BodyState::Flying, BodyState::Downed}][{x, y}] =
		    e.getAnimationEntry(dataAD, dataUA, dataUF, 2, {x, y}, offset, offset);
		p->body_state_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
		                          BodyState::Flying, BodyState::Dead}][{x, y}] =
		    e.getAnimationEntry(dataAD, dataUA, dataUF, 2, {x, y}, offset, offset);

		// Body Downed -> Flying animation: 2
		p->body_state_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
		                          BodyState::Downed, BodyState::Flying}][{x, y}] =
		    e.getAnimationEntryInv(dataAD, dataUA, dataUF, 2, {x, y}, 100, false, offset, offset);
	}
}
void InitialGameStateExtractor::extractAnimationPackPsi(sp<BattleUnitAnimationPack> p,
                                                        const std::vector<AnimationDataAD> &dataAD,
                                                        const std::vector<AnimationDataUA> &dataUA,
                                                        std::vector<AnimationDataUF> &dataUF) const
{
	for (int x = -1; x <= 1; x++)
	{
		for (int y = -1; y <= 1; y++)
		{
			// 0, 0 facing does not exist
			if (x == 0 && y == 0)
				continue;

			extractAnimationPackPsiInternal(p, dataAD, dataUA, dataUF, x, y, *this);
		}
	}
	p->useFiringAnimationForPsi = true;
}
} // namespace OpenApoc

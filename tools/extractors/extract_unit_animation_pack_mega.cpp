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

void extractAnimationPackMegaInternal(sp<BattleUnitAnimationPack> p,
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
		// Standing state: 0
		p->standart_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
		                        BodyState::Standing}][{x, y}] =
		    e.getAnimationEntry(dataAD, dataUA, dataUF, 0, {x, y}, offset, offset);

		// Downed/Dead state: 1 (frame #9)
		p->standart_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
		                        BodyState::Downed}][{x, y}] =
		    e.getAnimationEntry(dataAD, dataUA, dataUF, 1, {x, y}, 100, 9, false, false, false,
		                        offset, offset);
		p->standart_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
		                        BodyState::Dead}][{x, y}] =
		    e.getAnimationEntry(dataAD, dataUA, dataUF, 1, {x, y}, 100, 9, false, false, false,
		                        offset, offset);

		// Standing walking/running states: 3
		p->standart_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::Normal,
		                        BodyState::Standing}][{x, y}] =
		    e.getAnimationEntry(dataAD, dataUA, dataUF, 3, {x, y}, wFrames, false, offset, offset);
		p->standart_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::Running,
		                        BodyState::Standing}][{x, y}] =
		    e.getAnimationEntry(dataAD, dataUA, dataUF, 3, {x, y}, wFrames, false, offset, offset);

		// Aiming state: 6 (frame #2)
		p->standart_animations[{ItemWieldMode::None, HandState::Aiming, MovementState::None,
		                        BodyState::Standing}][{x, y}] =
		    e.getAnimationEntry(dataAD, dataUA, dataUF, 6, {x, y}, 100, 2, false, false, false,
		                        offset, offset);

		// Firing state: 6
		p->standart_animations[{ItemWieldMode::None, HandState::Firing, MovementState::None,
		                        BodyState::Standing}][{x, y}] =
		    e.getAnimationEntry(dataAD, dataUA, dataUF, 6, {x, y}, 100, false, offset, offset);
	}

	// Body state change animations
	{
		// Body Standing -> Downed/Dead animation: 1
		p->body_state_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
		                          BodyState::Standing, BodyState::Downed}][{x, y}] =
		    e.getAnimationEntry(dataAD, dataUA, dataUF, 1, {x, y}, offset, offset);
		p->body_state_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
		                          BodyState::Standing, BodyState::Dead}][{x, y}] =
		    e.getAnimationEntry(dataAD, dataUA, dataUF, 1, {x, y}, offset, offset);
		// Body Downed -> Standing animation: 1
		p->body_state_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
		                          BodyState::Downed, BodyState::Standing}][{x, y}] =
		    e.getAnimationEntryInv(dataAD, dataUA, dataUF, 1, {x, y}, 100, false, offset, offset);
	}

	// Alt-fire
	{
		// Aiming state UpUp: 4 (frame #2)
		p->alt_fire_animations[{ItemWieldMode::None, HandState::Aiming, 2, MovementState::None,
		                        BodyState::Standing}][{x, y}] =
		    e.getAnimationEntry(dataAD, dataUA, dataUF, 4, {x, y}, 100, 2, false, false, false,
		                        offset, offset);
		// Firing state UpUp: 4
		p->alt_fire_animations[{ItemWieldMode::None, HandState::Firing, 2, MovementState::None,
		                        BodyState::Standing}][{x, y}] =
		    e.getAnimationEntry(dataAD, dataUA, dataUF, 4, {x, y}, 100, false, offset, offset);

		// Aiming state Up: 5 (frame #2)
		p->alt_fire_animations[{ItemWieldMode::None, HandState::Aiming, 1, MovementState::None,
		                        BodyState::Standing}][{x, y}] =
		    e.getAnimationEntry(dataAD, dataUA, dataUF, 5, {x, y}, 100, 2, false, false, false,
		                        offset, offset);
		// Firing state Up: 5
		p->alt_fire_animations[{ItemWieldMode::None, HandState::Firing, 1, MovementState::None,
		                        BodyState::Standing}][{x, y}] =
		    e.getAnimationEntry(dataAD, dataUA, dataUF, 5, {x, y}, 100, false, offset, offset);

		// Aiming state Dn: 7 (frame #2)
		p->alt_fire_animations[{ItemWieldMode::None, HandState::Aiming, -1, MovementState::None,
		                        BodyState::Standing}][{x, y}] =
		    e.getAnimationEntry(dataAD, dataUA, dataUF, 7, {x, y}, 100, 2, false, false, false,
		                        offset, offset);
		// Firing state Dn: 7
		p->alt_fire_animations[{ItemWieldMode::None, HandState::Firing, -1, MovementState::None,
		                        BodyState::Standing}][{x, y}] =
		    e.getAnimationEntry(dataAD, dataUA, dataUF, 7, {x, y}, 100, false, offset, offset);

		// Aiming state DnDn: 8 (frame #2)
		p->alt_fire_animations[{ItemWieldMode::None, HandState::Aiming, -2, MovementState::None,
		                        BodyState::Standing}][{x, y}] =
		    e.getAnimationEntry(dataAD, dataUA, dataUF, 8, {x, y}, 100, 2, false, false, false,
		                        offset, offset);
		// Firing state DnDn: 8
		p->alt_fire_animations[{ItemWieldMode::None, HandState::Firing, -2, MovementState::None,
		                        BodyState::Standing}][{x, y}] =
		    e.getAnimationEntry(dataAD, dataUA, dataUF, 8, {x, y}, 100, false, offset, offset);
	}
}
void InitialGameStateExtractor::extractAnimationPackMega(sp<BattleUnitAnimationPack> p,
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

			extractAnimationPackMegaInternal(p, dataAD, dataUA, dataUF, x, y, *this);
		}
	}
	p->hasAlternativeFiringAnimations = true;
}
} // namespace OpenApoc

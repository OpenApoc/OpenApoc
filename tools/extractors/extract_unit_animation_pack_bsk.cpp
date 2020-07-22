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

void InitialGameStateExtractor::extractAnimationPackBsk(sp<BattleUnitAnimationPack> p,
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

			// Frames per 100 units
			static const int wFrames = 300; // Walk
			static const int rFrames = 300; // Run

			// Standart animations
			{
				// Dead state: 5's last frame (#2)
				p->standart_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
				                        BodyState::Dead}][{x, y}] =
				    getAnimationEntry(dataAD, dataUA, dataUF, 5, {x, y}, 100, 2, false);

				// Downed state: frozen frame 143, regardless of where it looks
				p->standart_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
				                        BodyState::Downed}][{x, y}] =
				    makeUpAnimationEntry(143, 1, 0, 0, 1, {0, 0}, 100);

				// Standing state: 2's first frame
				p->standart_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
				                        BodyState::Standing}][{x, y}] =
				    getAnimationEntry(dataAD, dataUA, dataUF, 2, {x, y}, 100, 1, true);

				// Walking state: 2
				p->standart_animations[{ItemWieldMode::None, HandState::AtEase,
				                        MovementState::Normal, BodyState::Standing}][{x, y}] =
				    getAnimationEntry(dataAD, dataUA, dataUF, 2, {x, y}, wFrames);

				// Running state: 2
				p->standart_animations[{ItemWieldMode::None, HandState::AtEase,
				                        MovementState::Running, BodyState::Standing}][{x, y}] =
				    getAnimationEntry(dataAD, dataUA, dataUF, 2, {x, y}, rFrames);

				// Jumping state: 3's last frame (#9)
				p->standart_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
				                        BodyState::Jumping}][{x, y}] =
				    getAnimationEntry(dataAD, dataUA, dataUF, 3, {x, y}, 100, 9, false);
				p->standart_animations[{ItemWieldMode::None, HandState::AtEase,
				                        MovementState::Normal, BodyState::Jumping}][{x, y}] =
				    getAnimationEntry(dataAD, dataUA, dataUF, 3, {x, y}, 100, 9, false);

				// Sucking state: 4
				p->standart_animations[{ItemWieldMode::None, HandState::AtEase,
				                        MovementState::Brainsuck, BodyState::Jumping}][{x, y}] =
				    getAnimationEntry(dataAD, dataUA, dataUF, 4, {x, y}, 100);
			}

			// Body state change animations
			{
				// Body Standing -> Dead animation: 5
				p->body_state_animations[{ItemWieldMode::None, HandState::AtEase,
				                          MovementState::None, BodyState::Standing,
				                          BodyState::Dead}][{x, y}] =
				    getAnimationEntry(dataAD, dataUA, dataUF, 5, {x, y});

				// Body Standing -> Jumping animation: 3
				p->body_state_animations[{ItemWieldMode::None, HandState::AtEase,
				                          MovementState::None, BodyState::Standing,
				                          BodyState::Jumping}][{x, y}] =
				    getAnimationEntry(dataAD, dataUA, dataUF, 3, {x, y});
				p->body_state_animations[{ItemWieldMode::None, HandState::AtEase,
				                          MovementState::Normal, BodyState::Standing,
				                          BodyState::Jumping}][{x, y}] =
				    getAnimationEntry(dataAD, dataUA, dataUF, 3, {x, y});
			}
		}
	}

	// Brainsucker ready to pop image
	p->standart_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
	                        BodyState::Throwing}][{0, 1}] =
	    getAnimationEntry(dataAD, dataUA, dataUF, 1, {0, 1}, 100, 1, true);

	// Brainsucker popping up animation (Throwing - > Standing)
	p->body_state_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
	                          BodyState::Throwing, BodyState::Standing}][{0, 1}] =
	    getAnimationEntry(dataAD, dataUA, dataUF, 1, {0, 1});
}
} // namespace OpenApoc

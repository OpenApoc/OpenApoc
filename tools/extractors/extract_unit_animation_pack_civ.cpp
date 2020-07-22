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

void extractAnimationPackCivInternal(sp<BattleUnitAnimationPack> p,
                                     const std::vector<AnimationDataAD> &dataAD,
                                     const std::vector<AnimationDataUA> &dataUA,
                                     std::vector<AnimationDataUF> &dataUF, int x, int y,
                                     const InitialGameStateExtractor &e)
{
	// Units per 100 frames
	static const int wFrames = 300; // Walk
	static const int rFrames = 600; // Run

	// Standart animations
	{
		// Downed/Dead state: 3
		p->standart_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
		                        BodyState::Downed}][{x, y}] =
		    e.getAnimationEntry(dataAD, dataUA, dataUF, 3, {x, y});
		p->standart_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
		                        BodyState::Dead}][{x, y}] =
		    e.getAnimationEntry(dataAD, dataUA, dataUF, 3, {x, y});

		// Standing static state: 5
		p->standart_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
		                        BodyState::Standing}][{x, y}] =
		    e.getAnimationEntry(dataAD, dataUA, dataUF, 5, {x, y});

		// Standing walking state: 0
		p->standart_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::Normal,
		                        BodyState::Standing}][{x, y}] =
		    e.getAnimationEntry(dataAD, dataUA, dataUF, 0, {x, y}, wFrames);

		// Standing running state: 0
		p->standart_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::Running,
		                        BodyState::Standing}][{x, y}] =
		    e.getAnimationEntry(dataAD, dataUA, dataUF, 1, {x, y}, rFrames);
	}

	// Body state change animations
	{
		// Body Standing -> Downed/Dead animation: 2
		p->body_state_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
		                          BodyState::Standing, BodyState::Downed}][{x, y}] =
		    e.getAnimationEntry(dataAD, dataUA, dataUF, 2, {x, y});
		p->body_state_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
		                          BodyState::Standing, BodyState::Dead}][{x, y}] =
		    e.getAnimationEntry(dataAD, dataUA, dataUF, 2, {x, y});
	}
}

void InitialGameStateExtractor::extractAnimationPackCiv(sp<BattleUnitAnimationPack> p,
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

			extractAnimationPackCivInternal(p, dataAD, dataUA, dataUF, x, y, *this);
		}
	}
}
} // namespace OpenApoc

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

void extractAnimationPackMicroInternal(sp<BattleUnitAnimationPack> p, int x, int y,
                                       const InitialGameStateExtractor &e)
{
	// Units per 100 frames
	static const int wFrames = 300; // Walk

	// Standing state:
	p->standart_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
	                        BodyState::Standing}][{x, y}] =
	    e.makeUpAnimationEntry(0, 15, 0, 0, 1, {0, 0});

	// Downed/Dead state:
	p->standart_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
	                        BodyState::Downed}][{x, y}] =
	    e.makeUpAnimationEntry(0, 1, 0, 0, 1, {0, 0});
	p->standart_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
	                        BodyState::Dead}][{x, y}] =
	    e.makeUpAnimationEntry(0, 1, 0, 0, 1, {0, 0});

	// Moving states:
	p->standart_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::Normal,
	                        BodyState::Standing}][{x, y}] =
	    e.makeUpAnimationEntry(0, 15, 0, 0, 1, {0, 0}, wFrames);
	p->standart_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::Running,
	                        BodyState::Standing}][{x, y}] =
	    e.makeUpAnimationEntry(0, 15, 0, 0, 1, {0, 0}, wFrames);
}

void InitialGameStateExtractor::extractAnimationPackMicro(sp<BattleUnitAnimationPack> p) const
{
	for (int x = -1; x <= 1; x++)
	{
		for (int y = -1; y <= 1; y++)
		{
			// 0, 0 facing does not exist
			if (x == 0 && y == 0)
				continue;

			extractAnimationPackMicroInternal(p, x, y, *this);
		}
	}
}
} // namespace OpenApoc

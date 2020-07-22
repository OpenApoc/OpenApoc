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

void extractAnimationPackGunInternal(sp<BattleUnitAnimationPack> p, int x, int y,
                                     const InitialGameStateExtractor &e)
{

	p->standart_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
	                        BodyState::Standing}][{x, y}] =
	    e.makeUpAnimationEntry(0, 1, 0, 0, 1, {x, y}, {0, 0});
	p->standart_animations[{ItemWieldMode::None, HandState::Aiming, MovementState::None,
	                        BodyState::Standing}][{x, y}] =
	    e.makeUpAnimationEntry(0, 1, 0, 0, 1, {x, y}, {0, 0});
	p->standart_animations[{ItemWieldMode::None, HandState::Firing, MovementState::None,
	                        BodyState::Standing}][{x, y}] =
	    e.makeUpAnimationEntry(0, 1, 0, 0, 1, {x, y}, {0, 0});
	p->standart_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
	                        BodyState::Downed}][{x, y}] =
	    e.makeUpAnimationEntry(0, 1, 0, 0, 1, {x, y}, {0, 0});
	p->standart_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
	                        BodyState::Dead}][{x, y}] =
	    e.makeUpAnimationEntry(0, 1, 0, 0, 1, {x, y}, {0, 0});
}

void InitialGameStateExtractor::extractAnimationPackGun(sp<BattleUnitAnimationPack> p) const
{
	for (int x = -1; x <= 1; x++)
	{
		for (int y = -1; y <= 1; y++)
		{
			// 0, 0 facing does not exist
			if (x == 0 && y == 0)
				continue;

			extractAnimationPackGunInternal(p, x, y, *this);
		}
	}
}
} // namespace OpenApoc

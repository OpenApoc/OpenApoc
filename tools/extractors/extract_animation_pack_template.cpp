#include "framework/data.h"
#include "framework/framework.h"
#include "framework/palette.h"
#include "game/state/agent.h"
#include "game/state/battle/battleunitanimationpack.h"
#include "game/state/gamestate.h"
#include "tools/extractors/common/animation.h"
#include "tools/extractors/extractors.h"

namespace OpenApoc
{

void InitialGameStateExtractor::extractAnimationPackTemplate(
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

			// Frames per 100 units
			static const int pFrames = 300; // Prone
			static const int wFrames = 300; // Walk
			static const int rFrames = 600; // Run
			static const int sFrames = 300; // Strafe
			static const int jFrames = 300; // Jump

			/*
			// Standart animations
			{
			    // Downed state: 27
			    p->standart_animations[{ItemWieldMode::None, HandState::AtEase, MovementState::None,
			        BodyState::Downed}][{x, y}] =
			        getAnimationEntry(dataAD, dataUA, dataUF, 27, { x, y });
			}
			*/
		}
	}
}
}

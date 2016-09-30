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

// Parsing animations for "unit/anim" files
void InitialGameStateExtractor::extractAnimationPackUnit(sp<BattleUnitAnimationPack> p,
	const std::vector<AnimationDataAD> &dataAD,
	const std::vector<AnimationDataUA> &dataUA,
	std::vector<AnimationDataUF> &dataUF, int x, int y)
{
	// Frames per 100 units
	static const int pFrames = 300; // Prone
	static const int wFrames = 300; // Walk
	static const int rFrames = 600; // Run
	static const int sFrames = 300; // Strafe
	static const int jFrames = 300; // Jump

	// Standart animations
	{
		// Downed state: 27
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::None]
			[AgentType::HandState::AtEase][AgentType::MovementState::None]
			[AgentType::BodyState::Downed][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 27, { x, y });
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
			[AgentType::HandState::AtEase][AgentType::MovementState::None]
			[AgentType::BodyState::Downed][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 27, { x, y });
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
			[AgentType::HandState::AtEase][AgentType::MovementState::None]
			[AgentType::BodyState::Downed][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 27, { x, y });

		// Standing static states: 0, 1, 2
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::None]
			[AgentType::HandState::AtEase][AgentType::MovementState::None]
			[AgentType::BodyState::Standing][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 0, { x, y });
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
			[AgentType::HandState::AtEase][AgentType::MovementState::None]
			[AgentType::BodyState::Standing][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 1, { x, y });
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
			[AgentType::HandState::AtEase][AgentType::MovementState::None]
			[AgentType::BodyState::Standing][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 2, { x, y });

		// Kneeling static states: 3, 4, 5
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::None]
			[AgentType::HandState::AtEase][AgentType::MovementState::None]
			[AgentType::BodyState::Kneeling][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 3, { x, y });
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
			[AgentType::HandState::AtEase][AgentType::MovementState::None]
			[AgentType::BodyState::Kneeling][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 4, { x, y });
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
			[AgentType::HandState::AtEase][AgentType::MovementState::None]
			[AgentType::BodyState::Kneeling][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 5, { x, y });

		// Prone static states: 6, 7, 8
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::None]
			[AgentType::HandState::AtEase][AgentType::MovementState::None]
			[AgentType::BodyState::Prone][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 6, { x, y });
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
			[AgentType::HandState::AtEase][AgentType::MovementState::None]
			[AgentType::BodyState::Prone][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 7, { x, y });
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
			[AgentType::HandState::AtEase][AgentType::MovementState::None]
			[AgentType::BodyState::Prone][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 8, { x, y });

		// Standing walking states: 9, 10, 11
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::None]
			[AgentType::HandState::AtEase][AgentType::MovementState::Normal]
			[AgentType::BodyState::Standing][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 9, { x, y }, wFrames);
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
			[AgentType::HandState::AtEase][AgentType::MovementState::Normal]
			[AgentType::BodyState::Standing][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 10, { x, y }, wFrames);
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
			[AgentType::HandState::AtEase][AgentType::MovementState::Normal]
			[AgentType::BodyState::Standing][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 11, { x, y }, wFrames);

		// Standing strafing states: 12, 13, 14
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::None]
			[AgentType::HandState::AtEase][AgentType::MovementState::Strafing]
			[AgentType::BodyState::Standing][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 12, { x, y }, sFrames);
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
			[AgentType::HandState::AtEase][AgentType::MovementState::Strafing]
			[AgentType::BodyState::Standing][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 13, { x, y }, sFrames);
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
			[AgentType::HandState::AtEase][AgentType::MovementState::Strafing]
			[AgentType::BodyState::Standing][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 14, { x, y }),
			sFrames;

		// Standing running state: 15
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::None]
			[AgentType::HandState::AtEase][AgentType::MovementState::Running]
			[AgentType::BodyState::Standing][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 15, { x, y }, rFrames);
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
			[AgentType::HandState::AtEase][AgentType::MovementState::Running]
			[AgentType::BodyState::Standing][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 15, { x, y }, rFrames);
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
			[AgentType::HandState::AtEase][AgentType::MovementState::Running]
			[AgentType::BodyState::Standing][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 15, { x, y }, rFrames);

		// Prone moving state: 18
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::None]
			[AgentType::HandState::AtEase][AgentType::MovementState::Normal]
			[AgentType::BodyState::Prone][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 18, { x, y }, pFrames);
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
			[AgentType::HandState::AtEase][AgentType::MovementState::Normal]
			[AgentType::BodyState::Prone][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 18, { x, y }, pFrames);
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
			[AgentType::HandState::AtEase][AgentType::MovementState::Normal]
			[AgentType::BodyState::Prone][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 18, { x, y }, pFrames);

		// Jumping moving state: 22
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::None]
			[AgentType::HandState::AtEase][AgentType::MovementState::Normal]
			[AgentType::BodyState::Jumping][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 22, { x, y }, jFrames);
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
			[AgentType::HandState::AtEase][AgentType::MovementState::Normal]
			[AgentType::BodyState::Jumping][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 22, { x, y }, jFrames);
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
			[AgentType::HandState::AtEase][AgentType::MovementState::Normal]
			[AgentType::BodyState::Jumping][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 22, { x, y }, jFrames);

		// Flying standing/moving/running/strafing state: 24, 25, 26
		// Standing
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::None]
			[AgentType::HandState::AtEase][AgentType::MovementState::None]
			[AgentType::BodyState::Flying][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 24, { x, y });
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
			[AgentType::HandState::AtEase][AgentType::MovementState::None]
			[AgentType::BodyState::Flying][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 25, { x, y });
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
			[AgentType::HandState::AtEase][AgentType::MovementState::None]
			[AgentType::BodyState::Flying][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 26, { x, y });
		// "Walking" in the air
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::None]
			[AgentType::HandState::AtEase][AgentType::MovementState::Normal]
			[AgentType::BodyState::Flying][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 24, { x, y }, wFrames);
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
			[AgentType::HandState::AtEase][AgentType::MovementState::Normal]
			[AgentType::BodyState::Flying][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 25, { x, y }, wFrames);
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
			[AgentType::HandState::AtEase][AgentType::MovementState::Normal]
			[AgentType::BodyState::Flying][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 26, { x, y }, wFrames);
		// "Running" in the air
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::None]
			[AgentType::HandState::AtEase][AgentType::MovementState::Running]
			[AgentType::BodyState::Flying][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 24, { x, y }, rFrames);
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
			[AgentType::HandState::AtEase][AgentType::MovementState::Running]
			[AgentType::BodyState::Flying][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 25, { x, y }, rFrames);
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
			[AgentType::HandState::AtEase][AgentType::MovementState::Running]
			[AgentType::BodyState::Flying][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 26, { x, y }, rFrames);
		// "Strafing" in the air
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::None]
			[AgentType::HandState::AtEase][AgentType::MovementState::Strafing]
			[AgentType::BodyState::Flying][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 24, { x, y }, sFrames);
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
			[AgentType::HandState::AtEase][AgentType::MovementState::Strafing]
			[AgentType::BodyState::Flying][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 25, { x, y }, sFrames);
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
			[AgentType::HandState::AtEase][AgentType::MovementState::Strafing]
			[AgentType::BodyState::Flying][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 26, { x, y }, sFrames);

		// Standing aiming static animation: 54, 58
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
			[AgentType::HandState::Aiming][AgentType::MovementState::None]
			[AgentType::BodyState::Standing][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 54, { x, y });
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
			[AgentType::HandState::Aiming][AgentType::MovementState::None]
			[AgentType::BodyState::Standing][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 58, { x, y });

		// Kneeling aiming static animation: 62, 66
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
			[AgentType::HandState::Aiming][AgentType::MovementState::None]
			[AgentType::BodyState::Kneeling][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 62, { x, y });
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
			[AgentType::HandState::Aiming][AgentType::MovementState::None]
			[AgentType::BodyState::Kneeling][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 66, { x, y });

		// Prone aiming static animation: 70, 74
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
			[AgentType::HandState::Aiming][AgentType::MovementState::None]
			[AgentType::BodyState::Prone][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 70, { x, y });
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
			[AgentType::HandState::Aiming][AgentType::MovementState::None]
			[AgentType::BodyState::Prone][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 74, { x, y });

		// Flying aiming static animation: 86, 90
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
			[AgentType::HandState::Aiming][AgentType::MovementState::None]
			[AgentType::BodyState::Flying][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 86, { x, y });
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
			[AgentType::HandState::Aiming][AgentType::MovementState::None]
			[AgentType::BodyState::Flying][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 90, { x, y });

		// Flying aiming moving animation: 86, 90
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
			[AgentType::HandState::Aiming][AgentType::MovementState::Normal]
			[AgentType::BodyState::Flying][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 86, { x, y }, wFrames);
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
			[AgentType::HandState::Aiming][AgentType::MovementState::Normal]
			[AgentType::BodyState::Flying][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 90, { x, y }, wFrames);

		// Standing aiming moving overlay animation: 78, 82
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
			[AgentType::HandState::Aiming][AgentType::MovementState::Normal]
			[AgentType::BodyState::Standing][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 78, { x, y }, true, wFrames);
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
			[AgentType::HandState::Aiming][AgentType::MovementState::Normal]
			[AgentType::BodyState::Standing][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 82, { x, y }, true, wFrames);
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
			[AgentType::HandState::Aiming][AgentType::MovementState::Strafing]
			[AgentType::BodyState::Standing][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 78, { x, y }, true, wFrames);
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
			[AgentType::HandState::Aiming][AgentType::MovementState::Strafing]
			[AgentType::BodyState::Standing][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 82, { x, y }, true, wFrames);

		// Standing firing static animation: 53, 57
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
			[AgentType::HandState::Firing][AgentType::MovementState::None]
			[AgentType::BodyState::Standing][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 53, { x, y });
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
			[AgentType::HandState::Firing][AgentType::MovementState::None]
			[AgentType::BodyState::Standing][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 57, { x, y });

		// Kneeling firing static animation: 61, 65
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
			[AgentType::HandState::Firing][AgentType::MovementState::None]
			[AgentType::BodyState::Kneeling][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 61, { x, y });
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
			[AgentType::HandState::Firing][AgentType::MovementState::None]
			[AgentType::BodyState::Kneeling][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 65, { x, y });

		// Prone firing static animation: 69, 73
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
			[AgentType::HandState::Firing][AgentType::MovementState::None]
			[AgentType::BodyState::Prone][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 69, { x, y });
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
			[AgentType::HandState::Firing][AgentType::MovementState::None]
			[AgentType::BodyState::Prone][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 73, { x, y });

		// Flying firing static animation: 85, 89
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
			[AgentType::HandState::Firing][AgentType::MovementState::None]
			[AgentType::BodyState::Flying][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 85, { x, y });
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
			[AgentType::HandState::Firing][AgentType::MovementState::None]
			[AgentType::BodyState::Flying][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 89, { x, y });

		// Flying firing "walking" animation: 85, 89
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
			[AgentType::HandState::Firing][AgentType::MovementState::Normal]
			[AgentType::BodyState::Flying][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 85, { x, y }, wFrames);
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
			[AgentType::HandState::Firing][AgentType::MovementState::Normal]
			[AgentType::BodyState::Flying][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 89, { x, y }, wFrames);

		// Standing firing moving overlay animation: 77, 81
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
			[AgentType::HandState::Firing][AgentType::MovementState::Normal]
			[AgentType::BodyState::Standing][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 77, { x, y }, true, wFrames);
		p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
			[AgentType::HandState::Firing][AgentType::MovementState::Normal]
			[AgentType::BodyState::Standing][{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 81, { x, y }, true, wFrames);
	}

	// Body state change animations
	{
		// Body Standing -> Downned animation: 47
		p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::None]
			[AgentType::HandState::AtEase][AgentType::MovementState::None]
			[AgentType::BodyState::Standing][AgentType::BodyState::Downed]
			[{x, y}] = getAnimationEntry(dataAD, dataUA, dataUF, 47, { x, y });
		p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
			[AgentType::HandState::AtEase][AgentType::MovementState::None]
			[AgentType::BodyState::Standing][AgentType::BodyState::Downed]
			[{x, y}] = getAnimationEntry(dataAD, dataUA, dataUF, 47, { x, y });
		p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
			[AgentType::HandState::AtEase][AgentType::MovementState::None]
			[AgentType::BodyState::Standing][AgentType::BodyState::Downed]
			[{x, y}] = getAnimationEntry(dataAD, dataUA, dataUF, 47, { x, y });
		p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::None]
			[AgentType::HandState::AtEase][AgentType::MovementState::None]
			[AgentType::BodyState::Downed][AgentType::BodyState::Standing]
			[{x, y}] = getAnimationEntry(dataAD, dataUA, dataUF, 47, { x, y }, true);
		p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
			[AgentType::HandState::AtEase][AgentType::MovementState::None]
			[AgentType::BodyState::Downed][AgentType::BodyState::Standing]
			[{x, y}] = getAnimationEntry(dataAD, dataUA, dataUF, 47, { x, y }, true);
		p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
			[AgentType::HandState::AtEase][AgentType::MovementState::None]
			[AgentType::BodyState::Downed][AgentType::BodyState::Standing]
			[{x, y}] = getAnimationEntry(dataAD, dataUA, dataUF, 47, { x, y }, true);

		// Body Kneeling -> Downned animation: 49
		p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::None]
			[AgentType::HandState::AtEase][AgentType::MovementState::None]
			[AgentType::BodyState::Kneeling][AgentType::BodyState::Downed]
			[{x, y}] = getAnimationEntry(dataAD, dataUA, dataUF, 49, { x, y });
		p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
			[AgentType::HandState::AtEase][AgentType::MovementState::None]
			[AgentType::BodyState::Kneeling][AgentType::BodyState::Downed]
			[{x, y}] = getAnimationEntry(dataAD, dataUA, dataUF, 49, { x, y });
		p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
			[AgentType::HandState::AtEase][AgentType::MovementState::None]
			[AgentType::BodyState::Kneeling][AgentType::BodyState::Downed]
			[{x, y}] = getAnimationEntry(dataAD, dataUA, dataUF, 49, { x, y });

		// Body Prone -> Downned animation: 50
		p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::None]
			[AgentType::HandState::AtEase][AgentType::MovementState::None]
			[AgentType::BodyState::Prone][AgentType::BodyState::Downed]
			[{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 50, { x, y }, gPrOff({ x, y }), { 0, 0 });
		p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
			[AgentType::HandState::AtEase][AgentType::MovementState::None]
			[AgentType::BodyState::Prone][AgentType::BodyState::Downed]
			[{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 50, { x, y }, gPrOff({ x, y }), { 0, 0 });
		p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
			[AgentType::HandState::AtEase][AgentType::MovementState::None]
			[AgentType::BodyState::Prone][AgentType::BodyState::Downed]
			[{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 50, { x, y }, gPrOff({ x, y }), { 0, 0 });

		// Body Flying -> Downned animation: 51
		p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::None]
			[AgentType::HandState::AtEase][AgentType::MovementState::None]
			[AgentType::BodyState::Flying][AgentType::BodyState::Downed]
			[{x, y}] = getAnimationEntry(dataAD, dataUA, dataUF, 51, { x, y });
		p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
			[AgentType::HandState::AtEase][AgentType::MovementState::None]
			[AgentType::BodyState::Flying][AgentType::BodyState::Downed]
			[{x, y}] = getAnimationEntry(dataAD, dataUA, dataUF, 51, { x, y });
		p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
			[AgentType::HandState::AtEase][AgentType::MovementState::None]
			[AgentType::BodyState::Flying][AgentType::BodyState::Downed]
			[{x, y}] = getAnimationEntry(dataAD, dataUA, dataUF, 51, { x, y });

		// Body Standing -> Jumping animation: 21
		p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::None]
			[AgentType::HandState::AtEase][AgentType::MovementState::Normal]
			[AgentType::BodyState::Standing][AgentType::BodyState::Jumping]
			[{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 21, { x, y }, jFrames);
		p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
			[AgentType::HandState::AtEase][AgentType::MovementState::Normal]
			[AgentType::BodyState::Standing][AgentType::BodyState::Jumping]
			[{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 21, { x, y }, jFrames);
		p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
			[AgentType::HandState::AtEase][AgentType::MovementState::Normal]
			[AgentType::BodyState::Standing][AgentType::BodyState::Jumping]
			[{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 21, { x, y }, jFrames);

		// Body Jumping -> Standing animation: 23, 28, 29
		p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::None]
			[AgentType::HandState::AtEase][AgentType::MovementState::Normal]
			[AgentType::BodyState::Jumping][AgentType::BodyState::Standing]
			[{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 23, { x, y }, jFrames);
		p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
			[AgentType::HandState::AtEase][AgentType::MovementState::Normal]
			[AgentType::BodyState::Jumping][AgentType::BodyState::Standing]
			[{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 28, { x, y }, jFrames);
		p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
			[AgentType::HandState::AtEase][AgentType::MovementState::Normal]
			[AgentType::BodyState::Jumping][AgentType::BodyState::Standing]
			[{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 29, { x, y }, jFrames);

		// Body Standing -> Kneeling animation: 35, 36, 37
		p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::None]
			[AgentType::HandState::AtEase][AgentType::MovementState::None]
			[AgentType::BodyState::Standing][AgentType::BodyState::Kneeling]
			[{x, y}] = getAnimationEntry(dataAD, dataUA, dataUF, 35, { x, y });
		p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
			[AgentType::HandState::AtEase][AgentType::MovementState::None]
			[AgentType::BodyState::Standing][AgentType::BodyState::Kneeling]
			[{x, y}] = getAnimationEntry(dataAD, dataUA, dataUF, 36, { x, y });
		p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
			[AgentType::HandState::AtEase][AgentType::MovementState::None]
			[AgentType::BodyState::Standing][AgentType::BodyState::Kneeling]
			[{x, y}] = getAnimationEntry(dataAD, dataUA, dataUF, 37, { x, y });

		// Body Kneeling -> Prone animation: 38, 39, 40
		p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::None]
			[AgentType::HandState::AtEase][AgentType::MovementState::None]
			[AgentType::BodyState::Kneeling][AgentType::BodyState::Prone]
			[{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 38, { x, y }, { 0, 0 }, gPrOff({ x, y }));
		p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
			[AgentType::HandState::AtEase][AgentType::MovementState::None]
			[AgentType::BodyState::Kneeling][AgentType::BodyState::Prone]
			[{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 38, { x, y }, { 0, 0 }, gPrOff({ x, y }));
		p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
			[AgentType::HandState::AtEase][AgentType::MovementState::None]
			[AgentType::BodyState::Kneeling][AgentType::BodyState::Prone]
			[{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 40, { x, y }, { 0, 0 }, gPrOff({ x, y }));

		// Body Prone -> Kneeling animation: 92, 93, 94
		p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::None]
			[AgentType::HandState::AtEase][AgentType::MovementState::None]
			[AgentType::BodyState::Prone][AgentType::BodyState::Kneeling]
			[{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 92, { x, y }, gPrOff({ x, y }), { 0, 0 });
		p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
			[AgentType::HandState::AtEase][AgentType::MovementState::None]
			[AgentType::BodyState::Prone][AgentType::BodyState::Kneeling]
			[{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 93, { x, y }, gPrOff({ x, y }), { 0, 0 });
		p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
			[AgentType::HandState::AtEase][AgentType::MovementState::None]
			[AgentType::BodyState::Prone][AgentType::BodyState::Kneeling]
			[{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 94, { x, y }, gPrOff({ x, y }), { 0, 0 });

		// Body Kneeling -> Standing animation: 19, 16, 17
		p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::None]
			[AgentType::HandState::AtEase][AgentType::MovementState::None]
			[AgentType::BodyState::Kneeling][AgentType::BodyState::Standing]
			[{x, y}] = getAnimationEntry(dataAD, dataUA, dataUF, 19, { x, y });
		p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
			[AgentType::HandState::AtEase][AgentType::MovementState::None]
			[AgentType::BodyState::Kneeling][AgentType::BodyState::Standing]
			[{x, y}] = getAnimationEntry(dataAD, dataUA, dataUF, 16, { x, y });
		p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
			[AgentType::HandState::AtEase][AgentType::MovementState::None]
			[AgentType::BodyState::Kneeling][AgentType::BodyState::Standing]
			[{x, y}] = getAnimationEntry(dataAD, dataUA, dataUF, 17, { x, y });

		// Body Standing -> Throwing and back animation: 41
		p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
			[AgentType::HandState::AtEase][AgentType::MovementState::None]
			[AgentType::BodyState::Standing][AgentType::BodyState::Throwing]
			[{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 41, { x, y }, 100, 5, true);
		p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
			[AgentType::HandState::AtEase][AgentType::MovementState::None]
			[AgentType::BodyState::Standing][AgentType::BodyState::Throwing]
			[{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 41, { x, y }, 100, 5, true);
		p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::None]
			[AgentType::HandState::AtEase][AgentType::MovementState::None]
			[AgentType::BodyState::Throwing][AgentType::BodyState::Standing]
			[{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 41, { x, y }, 100, 5, false, false, true, { 0,0 }, { 0,0 }, false, 5);
		p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
			[AgentType::HandState::AtEase][AgentType::MovementState::None]
			[AgentType::BodyState::Throwing][AgentType::BodyState::Standing]
			[{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 41, { x, y }, 100, 5, false, false, true, { 0,0 }, { 0,0 }, false, 5);
		p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
			[AgentType::HandState::AtEase][AgentType::MovementState::None]
			[AgentType::BodyState::Throwing][AgentType::BodyState::Standing]
			[{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 41, { x, y }, 100, 5, false, false, true, { 0,0 }, { 0,0 }, false, 5);
	}

	// Hand state change animation
	{
		// Hand Aiming -> Ease standing static animation: 55, 59
		p->hand_state_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
			[AgentType::HandState::Aiming][AgentType::HandState::AtEase]
			[AgentType::MovementState::None][AgentType::BodyState::Standing]
			[{x, y}] = getAnimationEntry(dataAD, dataUA, dataUF, 55, { x, y });
		p->hand_state_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
			[AgentType::HandState::Aiming][AgentType::HandState::AtEase]
			[AgentType::MovementState::None][AgentType::BodyState::Standing]
			[{x, y}] = getAnimationEntry(dataAD, dataUA, dataUF, 59, { x, y });

		// Hand Ease -> Aiming standing static animation: 52, 56
		p->hand_state_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
			[AgentType::HandState::AtEase][AgentType::HandState::Aiming]
			[AgentType::MovementState::None][AgentType::BodyState::Standing]
			[{x, y}] = getAnimationEntry(dataAD, dataUA, dataUF, 52, { x, y });
		p->hand_state_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
			[AgentType::HandState::AtEase][AgentType::HandState::Aiming]
			[AgentType::MovementState::None][AgentType::BodyState::Standing]
			[{x, y}] = getAnimationEntry(dataAD, dataUA, dataUF, 56, { x, y });

		// Hand Aiming -> Ease kneeling static animation: 63, 67
		p->hand_state_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
			[AgentType::HandState::Aiming][AgentType::HandState::AtEase]
			[AgentType::MovementState::None][AgentType::BodyState::Kneeling]
			[{x, y}] = getAnimationEntry(dataAD, dataUA, dataUF, 63, { x, y });
		p->hand_state_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
			[AgentType::HandState::Aiming][AgentType::HandState::AtEase]
			[AgentType::MovementState::None][AgentType::BodyState::Kneeling]
			[{x, y}] = getAnimationEntry(dataAD, dataUA, dataUF, 67, { x, y });

		// Hand Ease -> Aiming kneeling static animation: 60, 64
		p->hand_state_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
			[AgentType::HandState::AtEase][AgentType::HandState::Aiming]
			[AgentType::MovementState::None][AgentType::BodyState::Kneeling]
			[{x, y}] = getAnimationEntry(dataAD, dataUA, dataUF, 60, { x, y });
		p->hand_state_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
			[AgentType::HandState::AtEase][AgentType::HandState::Aiming]
			[AgentType::MovementState::None][AgentType::BodyState::Kneeling]
			[{x, y}] = getAnimationEntry(dataAD, dataUA, dataUF, 64, { x, y });

		// Hand Aiming -> Ease prone static animation: 71, 75
		p->hand_state_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
			[AgentType::HandState::Aiming][AgentType::HandState::AtEase]
			[AgentType::MovementState::None][AgentType::BodyState::Prone]
			[{x, y}] = getAnimationEntry(dataAD, dataUA, dataUF, 71, { x, y });
		p->hand_state_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
			[AgentType::HandState::Aiming][AgentType::HandState::AtEase]
			[AgentType::MovementState::None][AgentType::BodyState::Prone]
			[{x, y}] = getAnimationEntry(dataAD, dataUA, dataUF, 75, { x, y });

		// Hand Ease -> Aiming prone static animation: 68, 72
		p->hand_state_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
			[AgentType::HandState::AtEase][AgentType::HandState::Aiming]
			[AgentType::MovementState::None][AgentType::BodyState::Prone]
			[{x, y}] = getAnimationEntry(dataAD, dataUA, dataUF, 68, { x, y });
		p->hand_state_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
			[AgentType::HandState::AtEase][AgentType::HandState::Aiming]
			[AgentType::MovementState::None][AgentType::BodyState::Prone]
			[{x, y}] = getAnimationEntry(dataAD, dataUA, dataUF, 72, { x, y });

		// Hand Aiming -> Ease flying static animation: 87, 91
		p->hand_state_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
			[AgentType::HandState::Aiming][AgentType::HandState::AtEase]
			[AgentType::MovementState::None][AgentType::BodyState::Flying]
			[{x, y}] = getAnimationEntry(dataAD, dataUA, dataUF, 87, { x, y });
		p->hand_state_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
			[AgentType::HandState::Aiming][AgentType::HandState::AtEase]
			[AgentType::MovementState::None][AgentType::BodyState::Flying]
			[{x, y}] = getAnimationEntry(dataAD, dataUA, dataUF, 91, { x, y });

		// Hand Ease -> Aiming flying static animation: 84, 88
		p->hand_state_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
			[AgentType::HandState::AtEase][AgentType::HandState::Aiming]
			[AgentType::MovementState::None][AgentType::BodyState::Flying]
			[{x, y}] = getAnimationEntry(dataAD, dataUA, dataUF, 84, { x, y });
		p->hand_state_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
			[AgentType::HandState::AtEase][AgentType::HandState::Aiming]
			[AgentType::MovementState::None][AgentType::BodyState::Flying]
			[{x, y}] = getAnimationEntry(dataAD, dataUA, dataUF, 88, { x, y });

		// Hand Aiming -> Ease flying moving animation: 87, 91
		p->hand_state_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
			[AgentType::HandState::Aiming][AgentType::HandState::AtEase]
			[AgentType::MovementState::Normal][AgentType::BodyState::Flying]
			[{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 87, { x, y }, wFrames);
		p->hand_state_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
			[AgentType::HandState::Aiming][AgentType::HandState::AtEase]
			[AgentType::MovementState::Normal][AgentType::BodyState::Flying]
			[{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 91, { x, y }, wFrames);

		// Hand Ease -> Aiming flying moving animation: 84, 88
		p->hand_state_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
			[AgentType::HandState::AtEase][AgentType::HandState::Aiming]
			[AgentType::MovementState::Normal][AgentType::BodyState::Flying]
			[{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 84, { x, y }, wFrames);
		p->hand_state_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
			[AgentType::HandState::AtEase][AgentType::HandState::Aiming]
			[AgentType::MovementState::Normal][AgentType::BodyState::Flying]
			[{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 88, { x, y }, wFrames);

		// Hand Aiming -> Ease standing overlay moving animation: 79, 83
		p->hand_state_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
			[AgentType::HandState::Aiming][AgentType::HandState::AtEase]
			[AgentType::MovementState::Normal][AgentType::BodyState::Standing]
			[{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 79, { x, y }, true, wFrames);
		p->hand_state_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
			[AgentType::HandState::Aiming][AgentType::HandState::AtEase]
			[AgentType::MovementState::Normal][AgentType::BodyState::Standing]
			[{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 83, { x, y }, true, wFrames);

		// Hand Ease -> Aiming standing overlay moving animation: 76, 80
		p->hand_state_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
			[AgentType::HandState::AtEase][AgentType::HandState::Aiming]
			[AgentType::MovementState::Normal][AgentType::BodyState::Standing]
			[{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 76, { x, y }, true, wFrames);
		p->hand_state_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
			[AgentType::HandState::AtEase][AgentType::HandState::Aiming]
			[AgentType::MovementState::Normal][AgentType::BodyState::Standing]
			[{x, y}] =
			getAnimationEntry(dataAD, dataUA, dataUF, 80, { x, y }, true, wFrames);
	}
}


}
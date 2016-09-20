#include "game/state/agent.h"
#include "game/state/battle/battleunitanimationpack.h"
#include "game/state/gamestate.h"
#include "framework/data.h"
#include "framework/framework.h"
#include "framework/palette.h"
#include "tools/extractors/extractors.h"
#include "tools/extractors/common/animation.h"

namespace OpenApoc
{
	/*
	// May be required in order to join together animations to create a longer one, i.e. Standing->Prone from Standing->Kneeling and Kneeling->Prone
	// But for now, not used
	sp<BattleUnitAnimationPack::AnimationEntry> combineAnimationEntries(sp<BattleUnitAnimationPack::AnimationEntry> e1, sp<BattleUnitAnimationPack::AnimationEntry> e2)
	{
		auto e = mksp<BattleUnitAnimationPack::AnimationEntry>();

		if (e1->is_overlay != e2->is_overlay)
			LogError("Incompatible entries: one is overlay, other isn't!");

		e->is_overlay = e1->is_overlay;
		e->frame_count = e1->frame_count + e2->frame_count;

		for (auto &f : e1->frames)
			e->frames.push_back(f);
		for (auto &f : e2->frames)
			e->frames.push_back(f);

		return e;
	}
	*/

	sp<BattleUnitAnimationPack::AnimationEntry> getAnimationEntry(const std::vector<AnimationDataAD> &dataAD,
		const std::vector<AnimationDataUA> &dataUA, std::vector<AnimationDataUF> &dataUF,
		int index, Vec2<int> direction, int split_point, bool left_side, bool isOverlay = false)
	{
		static const std::map<Vec2<int>, int> offset_dir_map = {
			{ {0,-1},	0 },
			{ {1,-1},	1 },
			{ {1,0},	2 },
			{ {1,1},	3 },
			{ {0,1},	4 },
			{ {-1,1},	5 },
			{ {-1,0},	6 },
			{ {-1,-1},	7 },
		};

		auto e = mksp<BattleUnitAnimationPack::AnimationEntry>();

		int offset_dir = offset_dir_map.at(direction);
		int offset_ua = dataAD[index * 8 + offset_dir].offset;
		int offset_uf = dataUA[offset_ua].offset;
		int from = left_side ? 0 : split_point;
		int to = left_side ? split_point : dataUA[offset_ua].frame_count;

		for (int i = from; i < to; i++)
		{
			auto data = dataUF[offset_uf + i];

			e->frames.push_back(BattleUnitAnimationPack::AnimationEntry::Frame());
			
			for (int j = 0; j < 7; j++)
			{
				int part_idx = data.draw_order[j];
				auto part_type = BattleUnitAnimationPack::AnimationEntry::Frame::UnitImagePart::Shadow;
				switch (part_idx)
				{
					case 0:
						part_type = BattleUnitAnimationPack::AnimationEntry::Frame::UnitImagePart::Shadow;
						break;
					case 1:
						part_type = BattleUnitAnimationPack::AnimationEntry::Frame::UnitImagePart::Body;
						break;
					case 2:
						part_type = BattleUnitAnimationPack::AnimationEntry::Frame::UnitImagePart::Legs;
						break;
					case 3:
						part_type = BattleUnitAnimationPack::AnimationEntry::Frame::UnitImagePart::Helmet;
						break;
					case 4:
						part_type = BattleUnitAnimationPack::AnimationEntry::Frame::UnitImagePart::LeftArm;
						break;
					case 5:
						part_type = BattleUnitAnimationPack::AnimationEntry::Frame::UnitImagePart::RightArm;
						break;
					case 6:
						part_type = BattleUnitAnimationPack::AnimationEntry::Frame::UnitImagePart::Weapon;
						break;
					default:
						LogError("Impossible part index %d found in UF located at entry %d offset %d", part_idx, offset_uf, j);
						break;
				}
				e->frames[i - from].unit_image_draw_order.push_back(part_type);
				e->frames[i - from].unit_image_parts[part_type] = BattleUnitAnimationPack::AnimationEntry::Frame::InfoBlock(
						data.parts[part_idx].frame_idx, data.parts[part_idx].x_offset, data.parts[part_idx].y_offset);
			}
		}
		
		e->is_overlay = isOverlay;
		e->frame_count = e->frames.size();

		return e;
	}

	sp<BattleUnitAnimationPack::AnimationEntry> getAnimationEntry(const std::vector<AnimationDataAD> &dataAD,
		const std::vector<AnimationDataUA> &dataUA, std::vector<AnimationDataUF> &dataUF,
		int index, Vec2<int> direction, bool isOverlay = false)
	{
		return getAnimationEntry(dataAD, dataUA, dataUF, index, direction, 0, false, isOverlay);
	}

	// Parsing animations for "unit/anim" files
	void extractAnimationPackUnit(sp<BattleUnitAnimationPack> p, const std::vector<AnimationDataAD> &dataAD,
		const std::vector<AnimationDataUA> &dataUA, std::vector<AnimationDataUF> &dataUF, int x, int y)
	{
		// 0, 0 facing does not exist
		if (x == 0 && y == 0)
			return;

		// Standart animations
		{
			// Downed state: 27
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::None]
				[AgentType::HandState::AtEase][AgentType::MovementState::None]
				[AgentType::BodyState::Downed][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 27, { x,y });
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
				[AgentType::HandState::AtEase][AgentType::MovementState::None]
				[AgentType::BodyState::Downed][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 27, { x,y });
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
				[AgentType::HandState::AtEase][AgentType::MovementState::None]
				[AgentType::BodyState::Downed][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 27, { x,y });

			// Standing static states: 0, 1, 2
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::None]
				[AgentType::HandState::AtEase][AgentType::MovementState::None]
				[AgentType::BodyState::Standing][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 0, { x,y });
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
				[AgentType::HandState::AtEase][AgentType::MovementState::None]
				[AgentType::BodyState::Standing][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 1, { x,y });
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
				[AgentType::HandState::AtEase][AgentType::MovementState::None]
				[AgentType::BodyState::Standing][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 2, { x,y });

			// Kneeling static states: 3, 4, 5
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::None]
				[AgentType::HandState::AtEase][AgentType::MovementState::None]
				[AgentType::BodyState::Kneeling][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 3, { x,y });
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
				[AgentType::HandState::AtEase][AgentType::MovementState::None]
				[AgentType::BodyState::Kneeling][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 4, { x,y });
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
				[AgentType::HandState::AtEase][AgentType::MovementState::None]
				[AgentType::BodyState::Kneeling][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 5, { x,y });

			// Prone static states: 6, 7, 8
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::None]
				[AgentType::HandState::AtEase][AgentType::MovementState::None]
				[AgentType::BodyState::Prone][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 6, { x,y });
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
				[AgentType::HandState::AtEase][AgentType::MovementState::None]
				[AgentType::BodyState::Prone][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 7, { x,y });
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
				[AgentType::HandState::AtEase][AgentType::MovementState::None]
				[AgentType::BodyState::Prone][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 8, { x,y });

			// Standing walking states: 9, 10, 11
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::None]
				[AgentType::HandState::AtEase][AgentType::MovementState::Normal]
				[AgentType::BodyState::Standing][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 9, { x,y });
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
				[AgentType::HandState::AtEase][AgentType::MovementState::Normal]
				[AgentType::BodyState::Standing][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 10, { x,y });
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
				[AgentType::HandState::AtEase][AgentType::MovementState::Normal]
				[AgentType::BodyState::Standing][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 11, { x,y });

			// Standing strafing states: 12, 13, 14
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::None]
				[AgentType::HandState::AtEase][AgentType::MovementState::Strafing]
				[AgentType::BodyState::Standing][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 12, { x,y });
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
				[AgentType::HandState::AtEase][AgentType::MovementState::Strafing]
				[AgentType::BodyState::Standing][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 13, { x,y });
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
				[AgentType::HandState::AtEase][AgentType::MovementState::Strafing]
				[AgentType::BodyState::Standing][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 14, { x,y });

			// Standing running state: 15
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::None]
				[AgentType::HandState::AtEase][AgentType::MovementState::Running]
				[AgentType::BodyState::Standing][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 15, { x,y });
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
				[AgentType::HandState::AtEase][AgentType::MovementState::Running]
				[AgentType::BodyState::Standing][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 15, { x,y });
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
				[AgentType::HandState::AtEase][AgentType::MovementState::Running]
				[AgentType::BodyState::Standing][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 15, { x,y });

			// Prone moving state: 18
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::None]
				[AgentType::HandState::AtEase][AgentType::MovementState::Normal]
				[AgentType::BodyState::Prone][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 18, { x,y });
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
				[AgentType::HandState::AtEase][AgentType::MovementState::Normal]
				[AgentType::BodyState::Prone][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 18, { x,y });
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
				[AgentType::HandState::AtEase][AgentType::MovementState::Normal]
				[AgentType::BodyState::Prone][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 18, { x,y });

			// Jumping moving state: 22
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::None]
				[AgentType::HandState::AtEase][AgentType::MovementState::Normal]
				[AgentType::BodyState::Jumping][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 22, { x,y });
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
				[AgentType::HandState::AtEase][AgentType::MovementState::Normal]
				[AgentType::BodyState::Jumping][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 22, { x,y });
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
				[AgentType::HandState::AtEase][AgentType::MovementState::Normal]
				[AgentType::BodyState::Jumping][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 22, { x,y });

			// Flying standing/moving/running/strafing state: 24, 25, 26
			// Standing
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::None]
				[AgentType::HandState::AtEase][AgentType::MovementState::None]
				[AgentType::BodyState::Flying][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 24, { x,y });
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
				[AgentType::HandState::AtEase][AgentType::MovementState::None]
				[AgentType::BodyState::Flying][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 25, { x,y });
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
				[AgentType::HandState::AtEase][AgentType::MovementState::None]
				[AgentType::BodyState::Flying][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 26, { x,y });
			// "Walking" in the air
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::None]
				[AgentType::HandState::AtEase][AgentType::MovementState::Normal]
				[AgentType::BodyState::Flying][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 24, { x,y });
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
				[AgentType::HandState::AtEase][AgentType::MovementState::Normal]
				[AgentType::BodyState::Flying][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 25, { x,y });
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
				[AgentType::HandState::AtEase][AgentType::MovementState::Normal]
				[AgentType::BodyState::Flying][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 26, { x,y });
			// "Running" in the air
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::None]
				[AgentType::HandState::AtEase][AgentType::MovementState::Running]
				[AgentType::BodyState::Flying][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 24, { x,y });
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
				[AgentType::HandState::AtEase][AgentType::MovementState::Running]
				[AgentType::BodyState::Flying][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 25, { x,y });
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
				[AgentType::HandState::AtEase][AgentType::MovementState::Running]
				[AgentType::BodyState::Flying][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 26, { x,y });
			// "Strafing" in the air
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::None]
				[AgentType::HandState::AtEase][AgentType::MovementState::Strafing]
				[AgentType::BodyState::Flying][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 24, { x,y });
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
				[AgentType::HandState::AtEase][AgentType::MovementState::Strafing]
				[AgentType::BodyState::Flying][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 25, { x,y });
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
				[AgentType::HandState::AtEase][AgentType::MovementState::Strafing]
				[AgentType::BodyState::Flying][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 26, { x,y });

			// Standing aiming static animation: 54, 58
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
				[AgentType::HandState::Aiming][AgentType::MovementState::None]
				[AgentType::BodyState::Standing][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 54, { x,y });
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
				[AgentType::HandState::Aiming][AgentType::MovementState::None]
				[AgentType::BodyState::Standing][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 58, { x,y });

			// Kneeling aiming static animation: 62, 66
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
				[AgentType::HandState::Aiming][AgentType::MovementState::None]
				[AgentType::BodyState::Kneeling][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 62, { x,y });
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
				[AgentType::HandState::Aiming][AgentType::MovementState::None]
				[AgentType::BodyState::Kneeling][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 66, { x,y });

			// Prone aiming static animation: 70, 74
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
				[AgentType::HandState::Aiming][AgentType::MovementState::None]
				[AgentType::BodyState::Prone][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 70, { x,y });
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
				[AgentType::HandState::Aiming][AgentType::MovementState::None]
				[AgentType::BodyState::Prone][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 74, { x,y });

			// Flying aiming static animation: 86, 90
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
				[AgentType::HandState::Aiming][AgentType::MovementState::None]
				[AgentType::BodyState::Flying][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 86, { x,y });
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
				[AgentType::HandState::Aiming][AgentType::MovementState::None]
				[AgentType::BodyState::Flying][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 90, { x,y });
			
			// Flying aiming moving animation: 86, 90
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
				[AgentType::HandState::Aiming][AgentType::MovementState::Normal]
				[AgentType::BodyState::Flying][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 86, { x,y });
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
				[AgentType::HandState::Aiming][AgentType::MovementState::Normal]
				[AgentType::BodyState::Flying][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 90, { x,y });

			// Standing aiming moving overlay animation: 78, 82
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
				[AgentType::HandState::Aiming][AgentType::MovementState::Normal]
				[AgentType::BodyState::Standing][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 78, { x,y }, true);
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
				[AgentType::HandState::Aiming][AgentType::MovementState::Normal]
				[AgentType::BodyState::Standing][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 82, { x,y }, true);
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
				[AgentType::HandState::Aiming][AgentType::MovementState::Strafing]
				[AgentType::BodyState::Standing][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 78, { x,y }, true);
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
				[AgentType::HandState::Aiming][AgentType::MovementState::Strafing]
				[AgentType::BodyState::Standing][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 82, { x,y }, true);

			// Standing firing static animation: 53, 57
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
				[AgentType::HandState::Firing][AgentType::MovementState::None]
				[AgentType::BodyState::Standing][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 53, { x,y });
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
				[AgentType::HandState::Firing][AgentType::MovementState::None]
				[AgentType::BodyState::Standing][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 57, { x,y });

			// Kneeling firing static animation: 61, 65
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
				[AgentType::HandState::Firing][AgentType::MovementState::None]
				[AgentType::BodyState::Kneeling][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 61, { x,y });
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
				[AgentType::HandState::Firing][AgentType::MovementState::None]
				[AgentType::BodyState::Kneeling][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 65, { x,y });

			// Prone firing static animation: 69, 73
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
				[AgentType::HandState::Firing][AgentType::MovementState::None]
				[AgentType::BodyState::Prone][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 69, { x,y });
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
				[AgentType::HandState::Firing][AgentType::MovementState::None]
				[AgentType::BodyState::Prone][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 73, { x,y });

			// Flying firing static animation: 85, 89
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
				[AgentType::HandState::Firing][AgentType::MovementState::None]
				[AgentType::BodyState::Flying][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 85, { x,y });
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
				[AgentType::HandState::Firing][AgentType::MovementState::None]
				[AgentType::BodyState::Flying][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 89, { x,y });
			
			// Flying firing "walking" animation: 85, 89
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
				[AgentType::HandState::Firing][AgentType::MovementState::Normal]
				[AgentType::BodyState::Flying][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 85, { x,y });
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
				[AgentType::HandState::Firing][AgentType::MovementState::Normal]
				[AgentType::BodyState::Flying][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 89, { x,y });

			// Standing firing moving overlay animation: 77, 81
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
				[AgentType::HandState::Firing][AgentType::MovementState::Normal]
				[AgentType::BodyState::Standing][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 77, { x,y });
			p->standart_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
				[AgentType::HandState::Firing][AgentType::MovementState::Normal]
				[AgentType::BodyState::Standing][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 81, { x,y });
		}
		
		// Body state change animations
		{
			// Body Standing -> Downned animation: 47
			p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::None]
				[AgentType::HandState::AtEase][AgentType::MovementState::None]
				[AgentType::BodyState::Standing][AgentType::BodyState::Downed][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 47, { x,y });
			p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
				[AgentType::HandState::AtEase][AgentType::MovementState::None]
				[AgentType::BodyState::Standing][AgentType::BodyState::Downed][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 47, { x,y });
			p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
				[AgentType::HandState::AtEase][AgentType::MovementState::None]
				[AgentType::BodyState::Standing][AgentType::BodyState::Downed][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 47, { x,y });

			// Body Kneeling -> Downned animation: 49
			p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::None]
				[AgentType::HandState::AtEase][AgentType::MovementState::None]
				[AgentType::BodyState::Kneeling][AgentType::BodyState::Downed][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 49, { x,y });
			p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
				[AgentType::HandState::AtEase][AgentType::MovementState::None]
				[AgentType::BodyState::Kneeling][AgentType::BodyState::Downed][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 49, { x,y });
			p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
				[AgentType::HandState::AtEase][AgentType::MovementState::None]
				[AgentType::BodyState::Kneeling][AgentType::BodyState::Downed][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 49, { x,y });

			// Body Prone -> Downned animation: 50
			p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::None]
				[AgentType::HandState::AtEase][AgentType::MovementState::None]
				[AgentType::BodyState::Prone][AgentType::BodyState::Downed][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 50, { x,y });
			p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
				[AgentType::HandState::AtEase][AgentType::MovementState::None]
				[AgentType::BodyState::Prone][AgentType::BodyState::Downed][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 50, { x,y });
			p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
				[AgentType::HandState::AtEase][AgentType::MovementState::None]
				[AgentType::BodyState::Prone][AgentType::BodyState::Downed][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 50, { x,y });

			// Body Flying -> Downned animation: 51
			p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::None]
				[AgentType::HandState::AtEase][AgentType::MovementState::None]
				[AgentType::BodyState::Flying][AgentType::BodyState::Downed][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 51, { x,y });
			p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
				[AgentType::HandState::AtEase][AgentType::MovementState::None]
				[AgentType::BodyState::Flying][AgentType::BodyState::Downed][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 51, { x,y });
			p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
				[AgentType::HandState::AtEase][AgentType::MovementState::None]
				[AgentType::BodyState::Flying][AgentType::BodyState::Downed][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 51, { x,y });

			// Body Standing -> Jumping animation: 21
			p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::None]
				[AgentType::HandState::AtEase][AgentType::MovementState::Normal]
				[AgentType::BodyState::Standing][AgentType::BodyState::Jumping][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 21, { x,y });
			p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
				[AgentType::HandState::AtEase][AgentType::MovementState::Normal]
				[AgentType::BodyState::Standing][AgentType::BodyState::Jumping][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 21, { x,y });
			p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
				[AgentType::HandState::AtEase][AgentType::MovementState::Normal]
				[AgentType::BodyState::Standing][AgentType::BodyState::Jumping][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 21, { x,y });

			// Body Jumping -> Standing animation: 23, 28, 29
			p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::None]
				[AgentType::HandState::AtEase][AgentType::MovementState::Normal]
				[AgentType::BodyState::Jumping][AgentType::BodyState::Standing][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 23, { x,y });
			p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
				[AgentType::HandState::AtEase][AgentType::MovementState::Normal]
				[AgentType::BodyState::Jumping][AgentType::BodyState::Standing][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 28, { x,y });
			p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
				[AgentType::HandState::AtEase][AgentType::MovementState::Normal]
				[AgentType::BodyState::Jumping][AgentType::BodyState::Standing][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 29, { x,y });

			// Body Standing -> Kneeling animation: 35, 36, 37
			p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::None]
				[AgentType::HandState::AtEase][AgentType::MovementState::None]
				[AgentType::BodyState::Standing][AgentType::BodyState::Kneeling][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 35, { x,y });
			p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
				[AgentType::HandState::AtEase][AgentType::MovementState::None]
				[AgentType::BodyState::Standing][AgentType::BodyState::Kneeling][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 36, { x,y });
			p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
				[AgentType::HandState::AtEase][AgentType::MovementState::None]
				[AgentType::BodyState::Standing][AgentType::BodyState::Kneeling][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 37, { x,y });

			// Body Kneeling -> Prone animation: 38, 39, 40
			p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::None]
				[AgentType::HandState::AtEase][AgentType::MovementState::None]
				[AgentType::BodyState::Kneeling][AgentType::BodyState::Prone][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 38, { x,y });
			p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
				[AgentType::HandState::AtEase][AgentType::MovementState::None]
				[AgentType::BodyState::Kneeling][AgentType::BodyState::Prone][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 38, { x,y });
			p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
				[AgentType::HandState::AtEase][AgentType::MovementState::None]
				[AgentType::BodyState::Kneeling][AgentType::BodyState::Prone][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 40, { x,y });

			// Body Prone -> Kneeling animation: 92, 93, 94
			p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::None]
				[AgentType::HandState::AtEase][AgentType::MovementState::None]
				[AgentType::BodyState::Prone][AgentType::BodyState::Kneeling][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 92, { x,y });
			p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
				[AgentType::HandState::AtEase][AgentType::MovementState::None]
				[AgentType::BodyState::Prone][AgentType::BodyState::Kneeling][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 93, { x,y });
			p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
				[AgentType::HandState::AtEase][AgentType::MovementState::None]
				[AgentType::BodyState::Prone][AgentType::BodyState::Kneeling][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 94, { x,y });

			// Body Kneeling -> Standing animation: 19, 16, 17
			p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::None]
				[AgentType::HandState::AtEase][AgentType::MovementState::None]
				[AgentType::BodyState::Kneeling][AgentType::BodyState::Standing][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 19, { x,y });
			p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
				[AgentType::HandState::AtEase][AgentType::MovementState::None]
				[AgentType::BodyState::Kneeling][AgentType::BodyState::Standing][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 16, { x,y });
			p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
				[AgentType::HandState::AtEase][AgentType::MovementState::None]
				[AgentType::BodyState::Kneeling][AgentType::BodyState::Standing][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 17, { x,y });

			// Body Standing -> Throwing and back animation: 41
			p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
				[AgentType::HandState::AtEase][AgentType::MovementState::None]
				[AgentType::BodyState::Standing][AgentType::BodyState::Throwing][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 41, { x,y }, 6, false);
			p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
				[AgentType::HandState::AtEase][AgentType::MovementState::None]
				[AgentType::BodyState::Standing][AgentType::BodyState::Throwing][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 41, { x,y }, 6, false);
			p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
				[AgentType::HandState::AtEase][AgentType::MovementState::None]
				[AgentType::BodyState::Throwing][AgentType::BodyState::Standing][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 41, { x,y }, 6, true);
			p->body_state_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
				[AgentType::HandState::AtEase][AgentType::MovementState::None]
				[AgentType::BodyState::Throwing][AgentType::BodyState::Standing][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 41, { x,y }, 6, true);
		}

		// Hand state change animation
		{
			// Hand Aiming -> Ease standing static animation: 55, 59
			p->hand_state_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
				[AgentType::HandState::Aiming][AgentType::HandState::AtEase]
				[AgentType::MovementState::None][AgentType::BodyState::Standing][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 55, { x,y });
			p->hand_state_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
				[AgentType::HandState::Aiming][AgentType::HandState::AtEase]
				[AgentType::MovementState::None][AgentType::BodyState::Standing][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 59, { x,y });

			// Hand Ease -> Aiming standing static animation: 52, 56
			p->hand_state_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
				[AgentType::HandState::AtEase][AgentType::HandState::Aiming]
				[AgentType::MovementState::None][AgentType::BodyState::Standing][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 52, { x,y });
			p->hand_state_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
				[AgentType::HandState::AtEase][AgentType::HandState::Aiming]
				[AgentType::MovementState::None][AgentType::BodyState::Standing][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 56, { x,y });

			// Hand Aiming -> Ease kneeling static animation: 63, 67
			p->hand_state_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
				[AgentType::HandState::Aiming][AgentType::HandState::AtEase]
				[AgentType::MovementState::None][AgentType::BodyState::Kneeling][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 63, { x,y });
			p->hand_state_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
				[AgentType::HandState::Aiming][AgentType::HandState::AtEase]
				[AgentType::MovementState::None][AgentType::BodyState::Kneeling][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 67, { x,y });

			// Hand Ease -> Aiming kneeling static animation: 60, 64
			p->hand_state_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
				[AgentType::HandState::AtEase][AgentType::HandState::Aiming]
				[AgentType::MovementState::None][AgentType::BodyState::Kneeling][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 60, { x,y });
			p->hand_state_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
				[AgentType::HandState::AtEase][AgentType::HandState::Aiming]
				[AgentType::MovementState::None][AgentType::BodyState::Kneeling][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 64, { x,y });

			// Hand Aiming -> Ease prone static animation: 71, 75
			p->hand_state_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
				[AgentType::HandState::Aiming][AgentType::HandState::AtEase]
				[AgentType::MovementState::None][AgentType::BodyState::Prone][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 71, { x,y });
			p->hand_state_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
				[AgentType::HandState::Aiming][AgentType::HandState::AtEase]
				[AgentType::MovementState::None][AgentType::BodyState::Prone][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 75, { x,y });

			// Hand Ease -> Aiming prone static animation: 68, 72
			p->hand_state_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
				[AgentType::HandState::AtEase][AgentType::HandState::Aiming]
				[AgentType::MovementState::None][AgentType::BodyState::Prone][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 68, { x,y });
			p->hand_state_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
				[AgentType::HandState::AtEase][AgentType::HandState::Aiming]
				[AgentType::MovementState::None][AgentType::BodyState::Prone][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 72, { x,y });

			// Hand Aiming -> Ease flying static animation: 87, 91
			p->hand_state_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
				[AgentType::HandState::Aiming][AgentType::HandState::AtEase]
				[AgentType::MovementState::None][AgentType::BodyState::Flying][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 87, { x,y });
			p->hand_state_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
				[AgentType::HandState::Aiming][AgentType::HandState::AtEase]
				[AgentType::MovementState::None][AgentType::BodyState::Flying][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 91, { x,y });

			// Hand Ease -> Aiming flying static animation: 84, 88
			p->hand_state_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
				[AgentType::HandState::AtEase][AgentType::HandState::Aiming]
				[AgentType::MovementState::None][AgentType::BodyState::Flying][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 84, { x,y });
			p->hand_state_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
				[AgentType::HandState::AtEase][AgentType::HandState::Aiming]
				[AgentType::MovementState::None][AgentType::BodyState::Flying][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 88, { x,y });

			// Hand Aiming -> Ease flying moving animation: 87, 91
			p->hand_state_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
				[AgentType::HandState::Aiming][AgentType::HandState::AtEase]
				[AgentType::MovementState::Normal][AgentType::BodyState::Flying][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 87, { x,y });
			p->hand_state_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
				[AgentType::HandState::Aiming][AgentType::HandState::AtEase]
				[AgentType::MovementState::Normal][AgentType::BodyState::Flying][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 91, { x,y });

			// Hand Ease -> Aiming flying moving animation: 84, 88
			p->hand_state_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
				[AgentType::HandState::AtEase][AgentType::HandState::Aiming]
				[AgentType::MovementState::Normal][AgentType::BodyState::Flying][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 84, { x,y });
			p->hand_state_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
				[AgentType::HandState::AtEase][AgentType::HandState::Aiming]
				[AgentType::MovementState::Normal][AgentType::BodyState::Flying][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 88, { x,y });

			// Hand Aiming -> Ease standing overlay moving animation: 79, 83
			p->hand_state_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
				[AgentType::HandState::Aiming][AgentType::HandState::AtEase]
				[AgentType::MovementState::Normal][AgentType::BodyState::Standing][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 79, { x,y }, true);
			p->hand_state_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
				[AgentType::HandState::Aiming][AgentType::HandState::AtEase]
				[AgentType::MovementState::Normal][AgentType::BodyState::Standing][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 83, { x,y }, true);

			// Hand Ease -> Aiming standing overlay moving animation: 76, 80
			p->hand_state_animations[BattleUnitAnimationPack::ItemWieldMode::OneHanded]
				[AgentType::HandState::AtEase][AgentType::HandState::Aiming]
				[AgentType::MovementState::Normal][AgentType::BodyState::Standing][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 76, { x, y }, true);
			p->hand_state_animations[BattleUnitAnimationPack::ItemWieldMode::TwoHanded]
				[AgentType::HandState::AtEase][AgentType::HandState::Aiming]
				[AgentType::MovementState::Normal][AgentType::BodyState::Standing][{ x, y }] =
				getAnimationEntry(dataAD, dataUA, dataUF, 80, { x,y }, true);
		}
	}

	sp<BattleUnitAnimationPack> InitialGameStateExtractor::extractAnimationPack(GameState &state, const UString &path, const UString &name)
	{
		std::ignore = state;
		UString dirName = "xcom3/tacdata/";

		auto p = mksp<BattleUnitAnimationPack>();

		std::vector<AnimationDataAD> dataAD;
		{
			auto fileName = UString::format("%s%s%s", dirName, path, ".ad");

			auto inFile = fw().data->fs.open(fileName);
			if (inFile)
			{
				auto fileSize = inFile.size();
				auto objectCount = fileSize / sizeof(struct AnimationDataAD);

				for (unsigned i = 0; i < objectCount; i++)
				{
					AnimationDataAD data;
					inFile.read((char *)&data, sizeof(data));
					if (!inFile)
					{
						LogError("Failed to read entry in \"%s\"", fileName.cStr());
						return nullptr;
					}
					dataAD.push_back(data);
				}
			}
		}
				
		std::vector<AnimationDataUA> dataUA;
		{
			auto fileName = UString::format("%s%s%s", dirName, path, ".ua");

			auto inFile = fw().data->fs.open(fileName);
			if (inFile)
			{
				auto fileSize = inFile.size();
				auto objectCount = fileSize / sizeof(struct AnimationDataUA);

				for (unsigned i = 0; i < objectCount; i++)
				{
					AnimationDataUA data;
					inFile.read((char *)&data, sizeof(data));
					if (!inFile)
					{
						LogError("Failed to read entry in \"%s\"", fileName.cStr());
						return nullptr;
					}
					dataUA.push_back(data);
				}
			}
		}

		std::vector<AnimationDataUF> dataUF;
		{
			auto fileName = UString::format("%s%s%s", dirName, path, ".uf");

			auto inFile = fw().data->fs.open(fileName);
			if (inFile)
			{
				auto fileSize = inFile.size();
				auto objectCount = fileSize / sizeof(struct AnimationDataUF);

				for (unsigned i = 0; i < objectCount; i++)
				{
					AnimationDataUF data;
					inFile.read((char *)&data, sizeof(data));
					if (!inFile)
					{
						LogError("Failed to read entry in \"%s\"", fileName.cStr());
						return nullptr;
					}
					dataUF.push_back(data);
				}
			}
		}

		// It is not understood in what order animations come in .AD files.
		// Plus, some simplier aliens are missing those files alltogether.
		// Therefore, we have to parse them all manually here. I see no other option

		if (name == "unit") 
		{
			for (int x = -1; x <= 1; x++)
			{
				for (int y = -1; y <= 1; y++)
				{
					extractAnimationPackUnit(p, dataAD, dataUA, dataUF, x, y);
				}
			}
		}
		if (name == "bsk")
		{
		}	
		if (name == "chrys1")
		{
		}	
		if (name == "chrys2")
		{
		}
		if (name == "gun")
		{
		}	
		if (name == "hypr")
		{
		}	
		if (name == "mega")
		{
		}	
		if (name == "micro")
		{
		}	
		if (name == "multi")
		{
		}	
		if (name == "mwegg1")
		{
		}	
		if (name == "mwegg2")
		{
		}
		if (name == "popper")
		{
		}	
		if (name == "psi")
		{
		}	
		if (name == "queen")
		{
		}	
		if (name == "spitr")
		{
		}	
		if (name == "civ")
		{
		}	

		return p;
	}

}
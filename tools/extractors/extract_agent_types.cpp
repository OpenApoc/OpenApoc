#include "framework/framework.h"
#include "game/state/agent.h"
#include "game/state/rules/aequipment_type.h"
#include "tools/extractors/common/tacp.h"
#include "tools/extractors/common/ufo2p.h"
#include "tools/extractors/extractors.h"
#include <climits>
#include <algorithm>

namespace OpenApoc
{
void fillAgentImagePacksByDefault(GameState &state, sp<AgentType> a, UString imagePackName)
{
	a->image_packs[a->image_packs.size() - 1][AgentType::BodyPart::Body] = { &state, UString::format("%s%s%s",
		BattleUnitImagePack::getPrefix(), imagePackName, "a") };
	a->image_packs[a->image_packs.size() - 1][AgentType::BodyPart::Legs] = { &state, UString::format("%s%s%s",
		BattleUnitImagePack::getPrefix(), imagePackName, "b") };
	a->image_packs[a->image_packs.size() - 1][AgentType::BodyPart::Helmet] = { &state, UString::format("%s%s%s",
		BattleUnitImagePack::getPrefix(), imagePackName, "c") };
	a->image_packs[a->image_packs.size() - 1][AgentType::BodyPart::LeftArm] = { &state, UString::format("%s%s%s",
		BattleUnitImagePack::getPrefix(), imagePackName, "d") };
	a->image_packs[a->image_packs.size() - 1][AgentType::BodyPart::RightArm] = { &state, UString::format("%s%s%s",
		BattleUnitImagePack::getPrefix(), imagePackName, "e") };
}

void pushEquipmentSlot(sp<AgentType> a, int x, int y, int w = 1, int h = 1, AgentType::EquipmentSlotType type = AgentType::EquipmentSlotType::General, AgentType::AlignmentX align_x = AgentType::AlignmentX::Left, AgentType::AlignmentY align_y = AgentType::AlignmentY::Top)
{
	a->equipment_layout_slots.emplace_back();
	auto &outSlot = a->equipment_layout_slots.back();
	outSlot.type = type;
	outSlot.align_x = align_x;
	outSlot.align_y = align_y;
	outSlot.bounds = { x, y, x + w, y + h };
}

void InitialGameStateExtractor::extractAgentTypes(GameState &state, Difficulty)
{
	const int HUMAN_FEMALE_PORTRAIT_START = 0;
	const int HUMAN_FEMALE_PORTRAIT_END = 30;
	// const int HYBRID_FEMALE_PORTRAIT_START = 30;
	// const int HYBRID_FEMALE_PORTRAIT_END = 35;
	const int HUMAN_MALE_PORTRAIT_START = 35;
	const int HUMAN_MALE_PORTRAIT_END = 65;
	const int HYBRID_MALE_PORTRAIT_START = 65;
	const int HYBRID_MALE_PORTRAIT_END = 70;
	const int ANDROID_MALE_PORTRAIT_START = 70;
	const int ANDROID_MALE_PORTRAIT_END = 75;
	const int ALIEN_PORTRAIT_OFFSET = 74;

	const int UNIT_TYPE_BIOCHEMIST = 1;
	const int UNIT_TYPE_ENGINEER = 2;
	const int UNIT_TYPE_QUANTUM_PHYSIST = 3;
	const int UNIT_TYPE_GANG_LEADER = 4;
	const int UNIT_TYPE_CORPORATE_BOSS = 5;
	const int UNIT_TYPE_CULT_LEADER = 6;
	const int UNIT_TYPE_POLITICIAN = 7;
	const int UNIT_TYPE_CHIEF_OF_POLICE = 8;
	const int UNIT_TYPE_CORPORATE_HOOD = 9;
	const int UNIT_TYPE_POLICE = 10;
	const int UNIT_TYPE_GANGSTER = 11;
	const int UNIT_TYPE_CULTIST = 12;
	const int UNIT_TYPE_SECURITY = 13;
	const int UNIT_TYPE_ANDROID = 14;
	const int UNIT_TYPE_GREY = 15;
	const int UNIT_TYPE_UPPER_CLASS_FEMALE_1 = 16;
	const int UNIT_TYPE_UPPER_CLASS_FEMALE_2 = 17;
	const int UNIT_TYPE_UPPER_CLASS_FEMALE_3 = 18;
	const int UNIT_TYPE_UPPER_CLASS_MALE_1 = 19;
	const int UNIT_TYPE_UPPER_CLASS_MALE_2 = 20;
	const int UNIT_TYPE_UPPER_CLASS_MALE_3 = 21;
	const int UNIT_TYPE_CIVILIAN_FEMALE_1 = 22;
	const int UNIT_TYPE_CIVILIAN_FEMALE_2 = 23;
	const int UNIT_TYPE_CIVILIAN_FEMALE_3 = 24;
	const int UNIT_TYPE_CIVILIAN_MALE_1 = 25;
	const int UNIT_TYPE_CIVILIAN_MALE_2 = 26;
	const int UNIT_TYPE_CIVILIAN_MALE_3 = 27;
	const int UNIT_TYPE_LOWER_CLASS_MALE_1 = 28;
	const int UNIT_TYPE_LOWER_CLASS_MALE_2 = 29;
	const int UNIT_TYPE_LOWER_CLASS_MALE_3 = 30;
	const int UNIT_TYPE_LOWER_CLASS_FEMALE_1 = 31;
	const int UNIT_TYPE_LOWER_CLASS_FEMALE_2 = 32;
	const int UNIT_TYPE_LOWER_CLASS_FEMALE_3 = 33;
	const int UNIT_TYPE_MULTIWORM_EGG = 34;
	const int UNIT_TYPE_BRAINSUCKER = 35;
	const int UNIT_TYPE_MULTIWORM = 36;
	const int UNIT_TYPE_HYPERWORM = 37;
	const int UNIT_TYPE_CRYSALIS = 38;
	const int UNIT_TYPE_ANTHROPOD = 39;
	const int UNIT_TYPE_SKELETOID = 40;
	const int UNIT_TYPE_SPITTER = 41;
	const int UNIT_TYPE_POPPER = 42;
	const int UNIT_TYPE_MEGASPAWN = 43;
	const int UNIT_TYPE_PSIMORPH = 44;
	const int UNIT_TYPE_QUEENSPAWN = 45;
	const int UNIT_TYPE_MICRONOID = 46;

	const UString loftempsFile = "xcom3/tacdata/loftemps.dat";
	const UString loftempsTab = "xcom3/tacdata/loftemps.tab";

	auto &data_t = this->tacp;
	auto &data_u = this->ufo2p;

	// Portraits

	auto portraitSmallTabFileName = UString("xcom3/ufodata/agntico.tab");
	auto portraitSmallTabFile = fw().data->fs.open(portraitSmallTabFileName);
	if (!portraitSmallTabFile)
	{
		LogError("Failed to open small portrait TAB file \"%s\"", portraitSmallTabFileName.cStr());
		return;
	}
	size_t portraitSmallCount = portraitSmallTabFile.size() / 4;

	auto portraitLargeTabFileName = UString("xcom3/ufodata/photo.tab");
	auto portraitLargeTabFile = fw().data->fs.open(portraitLargeTabFileName);
	if (!portraitLargeTabFile)
	{
		LogError("Failed to open Large portrait TAB file \"%s\"", portraitLargeTabFileName.cStr());
		return;
	}
	size_t portraitLargeCount = portraitLargeTabFile.size() / 4;

	std::vector<AgentPortrait> portraits;

	for (unsigned i = 0; i < portraitSmallCount; i++)
	{
		auto p = AgentPortrait();
		p.icon = fw().data->loadImage(UString::format(
		    "PCK:xcom3/ufodata/agntico.pck:xcom3/ufodata/agntico.tab:%d:xcom3/ufodata/pal_01.dat",
		    i));
		if (i < portraitLargeCount)
			p.photo = fw().data->loadImage(UString::format(
			    "PCK:xcom3/ufodata/photo.pck:xcom3/ufodata/photo.tab:%d:xcom3/ufodata/pal_01.dat",
			    i));
		portraits.push_back(p);
	}

	// Unit Types

	// X-Com Agents are hand-filled in the patch xml, because they're not stored in the files
	// Therefore, we skip #0
	// Also not present here are two agent types for X-Com Base Turrets, laser and disruptor
	for (unsigned i = 1; i < data_u.agent_types->count(); i++)
	{
		auto a = mksp<AgentType>();
		auto data = data_u.agent_types->get(i);

		a->name = data_u.agent_type_names->get(i);
		UString id = UString::format("%s%s", AgentType::getPrefix(), canon_string(a->name));

		a->id = id;

		// Playability
		switch (i)
		{
			case UNIT_TYPE_BIOCHEMIST:
				a->role = AgentType::Role::BioChemist;
				a->playable = true;
				break;
			case UNIT_TYPE_ENGINEER:
				a->role = AgentType::Role::Engineer;
				a->playable = true;
				break;
			case UNIT_TYPE_QUANTUM_PHYSIST:
				a->role = AgentType::Role::Physicist;
				a->playable = true;
				break;
			default:
				a->role = AgentType::Role::Soldier;
				a->playable = false;
				break;
		}

		// Portraits
		switch (i)
		{
			case UNIT_TYPE_BIOCHEMIST:
			case UNIT_TYPE_ENGINEER:
			case UNIT_TYPE_QUANTUM_PHYSIST:
				a->possible_genders.insert(a->possible_genders.end(), AgentType::Gender::Male);
				a->possible_genders.insert(a->possible_genders.end(), AgentType::Gender::Female);
				a->gender_chance[AgentType::Gender::Male] = 1;
				a->gender_chance[AgentType::Gender::Female] = 1;
				for (unsigned p = HUMAN_MALE_PORTRAIT_START; p < HUMAN_MALE_PORTRAIT_END; p++)
					a->portraits[AgentType::Gender::Male][p - HUMAN_MALE_PORTRAIT_START] =
					    portraits[p];
				for (unsigned p = HUMAN_FEMALE_PORTRAIT_START; p < HUMAN_FEMALE_PORTRAIT_END; p++)
					a->portraits[AgentType::Gender::Female][p - HUMAN_FEMALE_PORTRAIT_START] =
					    portraits[p];
				break;
			case UNIT_TYPE_ANDROID:
				a->possible_genders.insert(a->possible_genders.end(), AgentType::Gender::Male);
				a->gender_chance[AgentType::Gender::Male] = 1;
				for (unsigned p = ANDROID_MALE_PORTRAIT_START; p < ANDROID_MALE_PORTRAIT_END; p++)
					a->portraits[AgentType::Gender::Male][p - ANDROID_MALE_PORTRAIT_START] =
					portraits[p];
				break;
			case UNIT_TYPE_GREY:
				a->possible_genders.insert(a->possible_genders.end(), AgentType::Gender::Male);
				a->gender_chance[AgentType::Gender::Male] = 1;
				for (unsigned p = HYBRID_MALE_PORTRAIT_START; p < HYBRID_MALE_PORTRAIT_END; p++)
					a->portraits[AgentType::Gender::Male][p - HYBRID_MALE_PORTRAIT_START] =
					portraits[p];
				break;
			case UNIT_TYPE_UPPER_CLASS_FEMALE_1:
			case UNIT_TYPE_UPPER_CLASS_FEMALE_2:
			case UNIT_TYPE_UPPER_CLASS_FEMALE_3:
			case UNIT_TYPE_CIVILIAN_FEMALE_1:
			case UNIT_TYPE_CIVILIAN_FEMALE_2:
			case UNIT_TYPE_CIVILIAN_FEMALE_3:
			case UNIT_TYPE_LOWER_CLASS_FEMALE_1:
			case UNIT_TYPE_LOWER_CLASS_FEMALE_2:
			case UNIT_TYPE_LOWER_CLASS_FEMALE_3:
				a->possible_genders.insert(a->possible_genders.end(), AgentType::Gender::Female);
				a->gender_chance[AgentType::Gender::Female] = 1;
				for (unsigned p = HUMAN_FEMALE_PORTRAIT_START; p < HUMAN_FEMALE_PORTRAIT_END; p++)
					a->portraits[AgentType::Gender::Female][p - HUMAN_FEMALE_PORTRAIT_START] =
					portraits[p];
				break;
			case UNIT_TYPE_CORPORATE_BOSS:
			case UNIT_TYPE_UPPER_CLASS_MALE_1:
			case UNIT_TYPE_UPPER_CLASS_MALE_2:
			case UNIT_TYPE_UPPER_CLASS_MALE_3:
			case UNIT_TYPE_CIVILIAN_MALE_1:
			case UNIT_TYPE_CIVILIAN_MALE_2:
			case UNIT_TYPE_CIVILIAN_MALE_3:
			case UNIT_TYPE_LOWER_CLASS_MALE_1:
			case UNIT_TYPE_LOWER_CLASS_MALE_2:
			case UNIT_TYPE_LOWER_CLASS_MALE_3:
				a->possible_genders.insert(a->possible_genders.end(), AgentType::Gender::Male);
				a->gender_chance[AgentType::Gender::Male] = 1;
				for (unsigned p = HUMAN_MALE_PORTRAIT_START; p < HUMAN_MALE_PORTRAIT_END; p++)
					a->portraits[AgentType::Gender::Male][p - HUMAN_MALE_PORTRAIT_START] =
					portraits[p];
				break;
			case UNIT_TYPE_MULTIWORM_EGG:
			case UNIT_TYPE_BRAINSUCKER:
			case UNIT_TYPE_MULTIWORM:
			case UNIT_TYPE_HYPERWORM:
			case UNIT_TYPE_CRYSALIS:
			case UNIT_TYPE_ANTHROPOD:
			case UNIT_TYPE_SKELETOID:
			case UNIT_TYPE_SPITTER:
			case UNIT_TYPE_POPPER:
			case UNIT_TYPE_MEGASPAWN:
			case UNIT_TYPE_PSIMORPH:
			case UNIT_TYPE_QUEENSPAWN:
			case UNIT_TYPE_MICRONOID:
				// Aliens start at id 34, their portraits start at 75,
				// first alien has image index of 1 rather than 0, so we shift by 74
				a->possible_genders.insert(a->possible_genders.end(), AgentType::Gender::Male);
				a->gender_chance[AgentType::Gender::Male] = 1;
				a->portraits[AgentType::Gender::Male][0] =
					portraits[data.image + ALIEN_PORTRAIT_OFFSET];
				break;
			// Other humans
			default:
				a->possible_genders.insert(a->possible_genders.end(), AgentType::Gender::Male);
				a->possible_genders.insert(a->possible_genders.end(), AgentType::Gender::Female);
				a->gender_chance[AgentType::Gender::Male] = 1;
				a->gender_chance[AgentType::Gender::Female] = 1;
				for (unsigned p = HUMAN_MALE_PORTRAIT_START; p < HUMAN_MALE_PORTRAIT_END; p++)
					a->portraits[AgentType::Gender::Male][p - HUMAN_MALE_PORTRAIT_START] =
					portraits[p];
				for (unsigned p = HUMAN_FEMALE_PORTRAIT_START; p < HUMAN_FEMALE_PORTRAIT_END; p++)
					a->portraits[AgentType::Gender::Female][p - HUMAN_FEMALE_PORTRAIT_START] =
					portraits[p];
				break;
		}

		// Allowed facings
		switch (i)
		{
			case UNIT_TYPE_CRYSALIS:
				a->allowed_facing.insert({ 0,  1 });
				break;
			case UNIT_TYPE_QUEENSPAWN:
				a->allowed_facing.insert({ 1,  0 });
				a->allowed_facing.insert({ 1,  1 });
				a->allowed_facing.insert({ 0,  1 });
				a->allowed_facing.insert({ -1,  1 });
				a->allowed_facing.insert({ -1,  0 });
				break;
			default:
				a->allowed_facing.insert({ 0, -1 });
				a->allowed_facing.insert({ 1, -1 });
				a->allowed_facing.insert({ 1,  0 });
				a->allowed_facing.insert({ 1,  1 });
				a->allowed_facing.insert({ 0,  1 });
				a->allowed_facing.insert({ -1,  1 });
				a->allowed_facing.insert({ -1,  0 });
				a->allowed_facing.insert({ -1, -1 });
				break;
		}
		
		// Allowed body states, voxel maps and animations
		std::map<AgentType::BodyState, Vec2<int>> voxelInfo;
		a->appearance_count = 1;
		switch (i)
		{
			// Civilians
			case UNIT_TYPE_ANDROID:
			case UNIT_TYPE_BIOCHEMIST:
			case UNIT_TYPE_ENGINEER:
			case UNIT_TYPE_QUANTUM_PHYSIST:
			case UNIT_TYPE_CORPORATE_BOSS:
			case UNIT_TYPE_GREY:
			case UNIT_TYPE_UPPER_CLASS_FEMALE_1:
			case UNIT_TYPE_UPPER_CLASS_FEMALE_2:
			case UNIT_TYPE_UPPER_CLASS_FEMALE_3:
			case UNIT_TYPE_CIVILIAN_FEMALE_1:
			case UNIT_TYPE_CIVILIAN_FEMALE_2:
			case UNIT_TYPE_CIVILIAN_FEMALE_3:
			case UNIT_TYPE_LOWER_CLASS_FEMALE_1:
			case UNIT_TYPE_LOWER_CLASS_FEMALE_2:
			case UNIT_TYPE_LOWER_CLASS_FEMALE_3:
			case UNIT_TYPE_UPPER_CLASS_MALE_1:
			case UNIT_TYPE_UPPER_CLASS_MALE_2:
			case UNIT_TYPE_UPPER_CLASS_MALE_3:
			case UNIT_TYPE_CIVILIAN_MALE_1:
			case UNIT_TYPE_CIVILIAN_MALE_2:
			case UNIT_TYPE_CIVILIAN_MALE_3:
			case UNIT_TYPE_LOWER_CLASS_MALE_1:
			case UNIT_TYPE_LOWER_CLASS_MALE_2:
			case UNIT_TYPE_LOWER_CLASS_MALE_3:
				if (i == UNIT_TYPE_ANDROID)
				{
					// Android differs from other civilians only in that it has 4 possible appearances
					// They all use same animation and differ in head image sets
					a->appearance_count = 4;
					a->animation_packs.emplace_back(&state, UString::format("%s%s", BattleUnitAnimationPack::getPrefix(),
						"civ"));
					a->animation_packs.emplace_back(&state, UString::format("%s%s", BattleUnitAnimationPack::getPrefix(),
						"civ"));
					a->animation_packs.emplace_back(&state, UString::format("%s%s", BattleUnitAnimationPack::getPrefix(),
						"civ"));
				}
				a->animation_packs.emplace_back(&state, UString::format("%s%s", BattleUnitAnimationPack::getPrefix(), 
					"civ"));
				a->allowed_movement_states.insert(AgentType::MovementState::None);
				a->allowed_movement_states.insert(AgentType::MovementState::Normal);
				a->allowed_movement_states.insert(AgentType::MovementState::Running);
				a->allowed_body_states.insert(AgentType::BodyState::Standing);
				a->allowed_body_states.insert(AgentType::BodyState::Downed);
				voxelInfo[AgentType::BodyState::Standing] = {data.loftemps_height, data.loftemps_idx};
				voxelInfo[AgentType::BodyState::Downed] = { 8, data.loftemps_idx };
				break;

			// Stationary aliens
			case UNIT_TYPE_MULTIWORM_EGG:
			case UNIT_TYPE_CRYSALIS:
			case UNIT_TYPE_QUEENSPAWN:
			{
				UString animPack = "";
				switch (i)
				{
					// There are two types of multiworm egg, one that has body facing sw, other nw
					case UNIT_TYPE_MULTIWORM_EGG:
						a->appearance_count = 2;
						a->animation_packs.emplace_back(&state, UString::format("%s%s", BattleUnitAnimationPack::getPrefix(),
							"mwegg1"));
						a->animation_packs.emplace_back(&state, UString::format("%s%s", BattleUnitAnimationPack::getPrefix(),
							"mwegg2"));
						break;
					// There are two types of chrysalises, one that has body facing sw, other nw
					case UNIT_TYPE_CRYSALIS:
						a->appearance_count = 2;
						a->animation_packs.emplace_back(&state, UString::format("%s%s", BattleUnitAnimationPack::getPrefix(),
							"chrys1"));
						a->animation_packs.emplace_back(&state, UString::format("%s%s", BattleUnitAnimationPack::getPrefix(),
							"chrys2"));
						break;
					case UNIT_TYPE_QUEENSPAWN:
						a->animation_packs.emplace_back(&state, UString::format("%s%s", BattleUnitAnimationPack::getPrefix(),
							"queen"));
						break;
				}
				a->allowed_movement_states.insert(AgentType::MovementState::None);
				a->allowed_body_states.insert(AgentType::BodyState::Standing);
				a->allowed_body_states.insert(AgentType::BodyState::Downed);
				voxelInfo[AgentType::BodyState::Standing] = { data.loftemps_height, data.loftemps_idx };
				voxelInfo[AgentType::BodyState::Downed] = { (i == UNIT_TYPE_QUEENSPAWN) ? 16 : 8, data.loftemps_idx };
				break;
			}

			// Non-humanoid aliens
			case UNIT_TYPE_BRAINSUCKER:
			case UNIT_TYPE_HYPERWORM:
			case UNIT_TYPE_SPITTER:
			case UNIT_TYPE_POPPER:
			case UNIT_TYPE_MICRONOID:
			{
				UString animPack = "";
				switch (i)
				{
				case UNIT_TYPE_BRAINSUCKER:
					animPack = "bsk";
					break;
				case UNIT_TYPE_HYPERWORM:
					animPack = "hypr";
					break;
				case UNIT_TYPE_SPITTER:
					animPack = "spitr";
					break;
				case UNIT_TYPE_POPPER:
					animPack = "popper";
					break;
				case UNIT_TYPE_MICRONOID:
					animPack = "micro";
					break;
				}
				a->animation_packs.emplace_back(&state, UString::format("%s%s", BattleUnitAnimationPack::getPrefix(),
					animPack));
				a->allowed_movement_states.insert(AgentType::MovementState::None);
				a->allowed_movement_states.insert(AgentType::MovementState::Normal);
				a->allowed_movement_states.insert(AgentType::MovementState::Running);
				a->allowed_body_states.insert(AgentType::BodyState::Standing);
				a->allowed_body_states.insert(AgentType::BodyState::Downed);
				voxelInfo[AgentType::BodyState::Standing] = { data.loftemps_height, data.loftemps_idx };
				voxelInfo[AgentType::BodyState::Downed] = { 8, data.loftemps_idx };
				break;
			}

			// Special case: Multiworm, can only crawl
			case UNIT_TYPE_MULTIWORM:
				a->animation_packs.emplace_back(&state, UString::format("%s%s", BattleUnitAnimationPack::getPrefix(),
					"multi"));
				a->allowed_movement_states.insert(AgentType::MovementState::None);
				a->allowed_movement_states.insert(AgentType::MovementState::Normal);
				a->allowed_body_states.insert(AgentType::BodyState::Kneeling);
				a->allowed_body_states.insert(AgentType::BodyState::Prone);
				a->allowed_body_states.insert(AgentType::BodyState::Downed);
				voxelInfo[AgentType::BodyState::Kneeling] = { data.loftemps_height, data.loftemps_idx };
				voxelInfo[AgentType::BodyState::Prone] = { data.loftemps_height, data.loftemps_idx };
				voxelInfo[AgentType::BodyState::Downed] = { 8, data.loftemps_idx };
				break;

			// Special case: Megaspawn, can strafe
			case UNIT_TYPE_MEGASPAWN:
				a->animation_packs.emplace_back(&state, UString::format("%s%s", BattleUnitAnimationPack::getPrefix(),
					"mega"));
				a->allowed_movement_states.insert(AgentType::MovementState::None);
				a->allowed_movement_states.insert(AgentType::MovementState::Normal);
				a->allowed_movement_states.insert(AgentType::MovementState::Running);
				a->allowed_movement_states.insert(AgentType::MovementState::Strafing);
				a->allowed_body_states.insert(AgentType::BodyState::Standing);
				a->allowed_body_states.insert(AgentType::BodyState::Downed);
				voxelInfo[AgentType::BodyState::Standing] = { data.loftemps_height, data.loftemps_idx };
				voxelInfo[AgentType::BodyState::Downed] = { 16, data.loftemps_idx };
				break;

			// Special case: Psimorph, non-humanoid that can fly
			case UNIT_TYPE_PSIMORPH:
				a->animation_packs.emplace_back(&state, UString::format("%s%s", BattleUnitAnimationPack::getPrefix(),
					"psi"));
				a->allowed_movement_states.insert(AgentType::MovementState::None);
				a->allowed_movement_states.insert(AgentType::MovementState::Normal);
				a->allowed_movement_states.insert(AgentType::MovementState::Running);
				a->allowed_body_states.insert(AgentType::BodyState::Standing);
				a->allowed_body_states.insert(AgentType::BodyState::Flying);
				a->allowed_body_states.insert(AgentType::BodyState::Downed);
				voxelInfo[AgentType::BodyState::Standing] = { data.loftemps_height, data.loftemps_idx };
				voxelInfo[AgentType::BodyState::Flying] = { data.loftemps_height, data.loftemps_idx };
				voxelInfo[AgentType::BodyState::Downed] = { 16, data.loftemps_idx };
				break;

			// Skeletoid and Anthropod are both humanoids
			case UNIT_TYPE_SKELETOID:
			case UNIT_TYPE_ANTHROPOD:
			// Other humans
			default:
				// Skeletoid only differs from other humanods by having built-in flight capabilities
				if (i == UNIT_TYPE_SKELETOID)
				{
					a->allowed_body_states.insert(AgentType::BodyState::Flying);
				}
				// Gangsters have 2 appearances
				if (i == UNIT_TYPE_GANGSTER)
				{
					a->appearance_count = 2;
					a->animation_packs.emplace_back(&state, UString::format("%s%s", BattleUnitAnimationPack::getPrefix(),
						"unit"));
				}
				a->animation_packs.emplace_back(&state, UString::format("%s%s", BattleUnitAnimationPack::getPrefix(),
					"unit"));
				a->allowed_movement_states.insert(AgentType::MovementState::None);
				a->allowed_movement_states.insert(AgentType::MovementState::Normal);
				a->allowed_movement_states.insert(AgentType::MovementState::Running);
				a->allowed_movement_states.insert(AgentType::MovementState::Strafing);
				a->allowed_body_states.insert(AgentType::BodyState::Standing);
				a->allowed_body_states.insert(AgentType::BodyState::Kneeling);
				a->allowed_body_states.insert(AgentType::BodyState::Prone);
				a->allowed_body_states.insert(AgentType::BodyState::Jumping);
				a->allowed_body_states.insert(AgentType::BodyState::Throwing);
				a->allowed_body_states.insert(AgentType::BodyState::Downed);
				voxelInfo[AgentType::BodyState::Standing] = { data.loftemps_height, data.loftemps_idx };
				voxelInfo[AgentType::BodyState::Kneeling] = { std::max(8, data.loftemps_height - 10), data.loftemps_idx };
				voxelInfo[AgentType::BodyState::Prone] = { 16, 10};
				voxelInfo[AgentType::BodyState::Jumping] = { std::max(8, data.loftemps_height - 10), data.loftemps_idx };
				voxelInfo[AgentType::BodyState::Throwing] = { data.loftemps_height, data.loftemps_idx };
				voxelInfo[AgentType::BodyState::Downed] = { 8, data.loftemps_idx };
				// Humanoids can possibly attain flight by use of armor, therefore include this here
				voxelInfo[AgentType::BodyState::Flying] = { data.loftemps_height, data.loftemps_idx };
				break;
		}
		
		// Used shadow packs
		switch (i)
		{
			// Aliens with unique shadows
			case UNIT_TYPE_BRAINSUCKER:
				a->shadow_pack = { &state, UString::format("%s%s", BattleUnitImagePack::getPrefix(),
					"bsks") };
				break;
			case UNIT_TYPE_HYPERWORM:
				a->shadow_pack = { &state, UString::format("%s%s", BattleUnitImagePack::getPrefix(),
					"hyprs") };
				break;
			case UNIT_TYPE_SPITTER:
				a->shadow_pack = { &state, UString::format("%s%s", BattleUnitImagePack::getPrefix(),
					"spitrs") };
				break;
			case UNIT_TYPE_POPPER:
				a->shadow_pack = { &state, UString::format("%s%s", BattleUnitImagePack::getPrefix(),
					"poppers") };
				break;
			case UNIT_TYPE_MEGASPAWN:
				a->shadow_pack = { &state, UString::format("%s%s", BattleUnitImagePack::getPrefix(),
					"megas") };
				break;
			case UNIT_TYPE_PSIMORPH:
				a->shadow_pack = { &state, UString::format("%s%s", BattleUnitImagePack::getPrefix(),
					"psis") };
				break;
			
			// Aliens with no shadows
			case UNIT_TYPE_CRYSALIS:
			case UNIT_TYPE_MULTIWORM_EGG:
			case UNIT_TYPE_MULTIWORM:
			case UNIT_TYPE_QUEENSPAWN:
			case UNIT_TYPE_MICRONOID:
				break;

			// Humanoid aliens and humans
			default:
				a->shadow_pack = { &state, UString::format("%s%s", BattleUnitImagePack::getPrefix(),
					"shadow") };
				break;
		}

		// Used image packs
		a->image_packs.push_back(std::map<AgentType::BodyPart, StateRef<BattleUnitImagePack>>());
		switch (i)
		{
			case UNIT_TYPE_BIOCHEMIST:
			case UNIT_TYPE_ENGINEER:
			case UNIT_TYPE_QUANTUM_PHYSIST:
				a->image_packs[0][AgentType::BodyPart::Body] = { &state, UString::format("%s%s", 
					BattleUnitImagePack::getPrefix(), "scntst") };
				break;
			case UNIT_TYPE_GANG_LEADER:
				fillAgentImagePacksByDefault(state, a, "gangl");
				break;
			case UNIT_TYPE_CORPORATE_BOSS:
				// Game has no picture for this unit. 
				// I think rm1 (upper class male) fits him best.
				a->image_packs[0][AgentType::BodyPart::Body] = { &state, UString::format("%s%s",
					BattleUnitImagePack::getPrefix(), "rm1") };
				break;
			case UNIT_TYPE_CULT_LEADER:
				fillAgentImagePacksByDefault(state, a, "cultl");
				break;
			case UNIT_TYPE_POLITICIAN:
				// Game has no picture for this unit. 
				// Since xcom1 is the rarest seen unit, I think it fits him best
				fillAgentImagePacksByDefault(state, a, "xcom1");
				break;
			case UNIT_TYPE_CHIEF_OF_POLICE:
				// Game has no picture for this unit.
				// Game only has one police picture, and since police won't have security
				// (since their guards use police pictures), I'm using security for police chief
				// This way it will stand out
				fillAgentImagePacksByDefault(state, a, "sec");
				break;
			case UNIT_TYPE_CORPORATE_HOOD:
				// Game has no picture for this unit. 
				// Since xcom1 is the rarest seen unit, I think it fits him best
				fillAgentImagePacksByDefault(state, a, "xcom1");
				break;
			case UNIT_TYPE_POLICE:
				fillAgentImagePacksByDefault(state, a, "police");
				break;
			case UNIT_TYPE_GANGSTER:
				fillAgentImagePacksByDefault(state, a, "gang");
				a->image_packs.push_back(std::map<AgentType::BodyPart, StateRef<BattleUnitImagePack>>());
				fillAgentImagePacksByDefault(state, a, "gang2");
				break;
			case UNIT_TYPE_CULTIST:
				fillAgentImagePacksByDefault(state, a, "cult");
				break;
			case UNIT_TYPE_SECURITY:
				fillAgentImagePacksByDefault(state, a, "sec");
				break;
			case UNIT_TYPE_ANDROID:
				a->image_packs[0][AgentType::BodyPart::Body] = { &state, UString::format("%s%s",
					BattleUnitImagePack::getPrefix(), "robot") };
				a->image_packs[0][AgentType::BodyPart::Helmet] = { &state, UString::format("%s%s",
					BattleUnitImagePack::getPrefix(), "robo1") };
				a->image_packs.push_back(std::map<AgentType::BodyPart, StateRef<BattleUnitImagePack>>());
				a->image_packs[1][AgentType::BodyPart::Body] = { &state, UString::format("%s%s",
					BattleUnitImagePack::getPrefix(), "robot") };
				a->image_packs[1][AgentType::BodyPart::Helmet] = { &state, UString::format("%s%s",
					BattleUnitImagePack::getPrefix(), "robo2") };
				a->image_packs.push_back(std::map<AgentType::BodyPart, StateRef<BattleUnitImagePack>>());
				a->image_packs[2][AgentType::BodyPart::Body] = { &state, UString::format("%s%s",
					BattleUnitImagePack::getPrefix(), "robot") };
				a->image_packs[2][AgentType::BodyPart::Helmet] = { &state, UString::format("%s%s",
					BattleUnitImagePack::getPrefix(), "robo3") };
				a->image_packs.push_back(std::map<AgentType::BodyPart, StateRef<BattleUnitImagePack>>());
				a->image_packs[3][AgentType::BodyPart::Body] = { &state, UString::format("%s%s",
					BattleUnitImagePack::getPrefix(), "robot") };
				a->image_packs[3][AgentType::BodyPart::Helmet] = { &state, UString::format("%s%s",
					BattleUnitImagePack::getPrefix(), "robo4") };
				break;
			case UNIT_TYPE_GREY:
				a->image_packs[0][AgentType::BodyPart::Body] = { &state, UString::format("%s%s",
					BattleUnitImagePack::getPrefix(), "grey") };
				break;
			case UNIT_TYPE_UPPER_CLASS_FEMALE_1:
				a->image_packs[0][AgentType::BodyPart::Body] = { &state, UString::format("%s%s",
					BattleUnitImagePack::getPrefix(), "rw1") };
				break;
			case UNIT_TYPE_UPPER_CLASS_FEMALE_2:
				a->image_packs[0][AgentType::BodyPart::Body] = { &state, UString::format("%s%s",
					BattleUnitImagePack::getPrefix(), "rw2") };
				break;
			case UNIT_TYPE_UPPER_CLASS_FEMALE_3:
				a->image_packs[0][AgentType::BodyPart::Body] = { &state, UString::format("%s%s",
					BattleUnitImagePack::getPrefix(), "rw3") };
				break;
			case UNIT_TYPE_UPPER_CLASS_MALE_1:
				a->image_packs[0][AgentType::BodyPart::Body] = { &state, UString::format("%s%s",
					BattleUnitImagePack::getPrefix(), "rm1") };
				break;
			case UNIT_TYPE_UPPER_CLASS_MALE_2:
				a->image_packs[0][AgentType::BodyPart::Body] = { &state, UString::format("%s%s",
					BattleUnitImagePack::getPrefix(), "rm2") };
				break;
			case UNIT_TYPE_UPPER_CLASS_MALE_3:
				a->image_packs[0][AgentType::BodyPart::Body] = { &state, UString::format("%s%s",
					BattleUnitImagePack::getPrefix(), "rm3") };
				break;
			case UNIT_TYPE_CIVILIAN_FEMALE_1:
				a->image_packs[0][AgentType::BodyPart::Body] = { &state, UString::format("%s%s",
					BattleUnitImagePack::getPrefix(), "nw1") };
				break;
			case UNIT_TYPE_CIVILIAN_FEMALE_2:
				a->image_packs[0][AgentType::BodyPart::Body] = { &state, UString::format("%s%s",
					BattleUnitImagePack::getPrefix(), "nw2") };
				break;
			case UNIT_TYPE_CIVILIAN_FEMALE_3:
				a->image_packs[0][AgentType::BodyPart::Body] = { &state, UString::format("%s%s",
					BattleUnitImagePack::getPrefix(), "nw3") };
				break;
			case UNIT_TYPE_CIVILIAN_MALE_1:
				a->image_packs[0][AgentType::BodyPart::Body] = { &state, UString::format("%s%s",
					BattleUnitImagePack::getPrefix(), "nm1") };
				break;
			case UNIT_TYPE_CIVILIAN_MALE_2:
				a->image_packs[0][AgentType::BodyPart::Body] = { &state, UString::format("%s%s",
					BattleUnitImagePack::getPrefix(), "nm2") };
				break;
			case UNIT_TYPE_CIVILIAN_MALE_3:
				a->image_packs[0][AgentType::BodyPart::Body] = { &state, UString::format("%s%s",
					BattleUnitImagePack::getPrefix(), "nm3") };
				break;
			case UNIT_TYPE_LOWER_CLASS_MALE_1:
				a->image_packs[0][AgentType::BodyPart::Body] = { &state, UString::format("%s%s",
					BattleUnitImagePack::getPrefix(), "sm1") };
				break;
			case UNIT_TYPE_LOWER_CLASS_MALE_2:
				a->image_packs[0][AgentType::BodyPart::Body] = { &state, UString::format("%s%s",
					BattleUnitImagePack::getPrefix(), "sm2") };
				break;
			case UNIT_TYPE_LOWER_CLASS_MALE_3:
				a->image_packs[0][AgentType::BodyPart::Body] = { &state, UString::format("%s%s",
					BattleUnitImagePack::getPrefix(), "sm3") };
				break;
			case UNIT_TYPE_LOWER_CLASS_FEMALE_1:
				a->image_packs[0][AgentType::BodyPart::Body] = { &state, UString::format("%s%s",
					BattleUnitImagePack::getPrefix(), "sw1") };
				break;
			case UNIT_TYPE_LOWER_CLASS_FEMALE_2:
				a->image_packs[0][AgentType::BodyPart::Body] = { &state, UString::format("%s%s",
					BattleUnitImagePack::getPrefix(), "sw2") };
				break;
			case UNIT_TYPE_LOWER_CLASS_FEMALE_3:
				a->image_packs[0][AgentType::BodyPart::Body] = { &state, UString::format("%s%s",
					BattleUnitImagePack::getPrefix(), "sw3") };
				break;
			case UNIT_TYPE_MULTIWORM_EGG:
				a->image_packs[0][AgentType::BodyPart::Body] = { &state, UString::format("%s%s",
					BattleUnitImagePack::getPrefix(), "mwegga") };
				a->image_packs[0][AgentType::BodyPart::Helmet] = { &state, UString::format("%s%s",
					BattleUnitImagePack::getPrefix(), "mweggb") };
				a->image_packs.push_back(std::map<AgentType::BodyPart, StateRef<BattleUnitImagePack>>());
				a->image_packs[1][AgentType::BodyPart::Body] = { &state, UString::format("%s%s",
					BattleUnitImagePack::getPrefix(), "mwegga") };
				a->image_packs[1][AgentType::BodyPart::Helmet] = { &state, UString::format("%s%s",
					BattleUnitImagePack::getPrefix(), "mweggb") };
				break;
			case UNIT_TYPE_BRAINSUCKER:
				a->image_packs[0][AgentType::BodyPart::Body] = { &state, UString::format("%s%s",
					BattleUnitImagePack::getPrefix(), "bsk") };
				break;
			case UNIT_TYPE_MULTIWORM:
				a->image_packs[0][AgentType::BodyPart::Body] = { &state, UString::format("%s%s",
					BattleUnitImagePack::getPrefix(), "multi") };
				break;
			case UNIT_TYPE_HYPERWORM:
				a->image_packs[0][AgentType::BodyPart::Body] = { &state, UString::format("%s%s",
					BattleUnitImagePack::getPrefix(), "hypr") };
				break;
			case UNIT_TYPE_CRYSALIS:
				a->image_packs[0][AgentType::BodyPart::Body] = { &state, UString::format("%s%s",
					BattleUnitImagePack::getPrefix(), "chrysa") };
				a->image_packs[0][AgentType::BodyPart::Helmet] = { &state, UString::format("%s%s",
					BattleUnitImagePack::getPrefix(), "chrysb") };
				a->image_packs.push_back(std::map<AgentType::BodyPart, StateRef<BattleUnitImagePack>>());
				a->image_packs[1][AgentType::BodyPart::Body] = { &state, UString::format("%s%s",
					BattleUnitImagePack::getPrefix(), "chrysa") };
				a->image_packs[1][AgentType::BodyPart::Helmet] = { &state, UString::format("%s%s",
					BattleUnitImagePack::getPrefix(), "chrysb") };
				break;
			case UNIT_TYPE_ANTHROPOD:
				fillAgentImagePacksByDefault(state, a, "antrp");
				break;
			case UNIT_TYPE_SKELETOID:
				fillAgentImagePacksByDefault(state, a, "skel");
				break;
			case UNIT_TYPE_SPITTER:
				a->image_packs[0][AgentType::BodyPart::Body] = { &state, UString::format("%s%s",
					BattleUnitImagePack::getPrefix(), "spitr") };
				break;
			case UNIT_TYPE_POPPER:
				a->image_packs[0][AgentType::BodyPart::Body] = { &state, UString::format("%s%s",
					BattleUnitImagePack::getPrefix(), "popper") };
				break;
			case UNIT_TYPE_MEGASPAWN:
				fillAgentImagePacksByDefault(state, a, "mega");
				break;
			case UNIT_TYPE_PSIMORPH:
				a->image_packs[0][AgentType::BodyPart::Body] = { &state, UString::format("%s%s",
					BattleUnitImagePack::getPrefix(), "psi") };
				break;
			case UNIT_TYPE_QUEENSPAWN:
				a->image_packs[0][AgentType::BodyPart::Body] = { &state, UString::format("%s%s",
					BattleUnitImagePack::getPrefix(), "queena") };
				a->image_packs[0][AgentType::BodyPart::Legs] = { &state, UString::format("%s%s",
					BattleUnitImagePack::getPrefix(), "queenb") };
				break;
			case UNIT_TYPE_MICRONOID:
				a->image_packs[0][AgentType::BodyPart::Body] = { &state, UString::format("%s%s",
					BattleUnitImagePack::getPrefix(), "micro") };
				break;
		}

		// Stats 
		a->min_stats.accuracy = 100 - data.accuracy_base - data.accuracy_inc;
		a->max_stats.accuracy = 100 - data.accuracy_base;
		a->min_stats.biochem_skill = data.biochemistry_base;
		a->max_stats.biochem_skill = data.biochemistry_base + data.biochemistry_inc;
		a->min_stats.bravery = data.bravery_base * 10;
		a->max_stats.bravery = data.bravery_base * 10 + data.bravery_inc * 10;
		a->min_stats.engineering_skill = data.engineering_base;
		a->max_stats.engineering_skill = data.engineering_base + data.engineering_inc;
		a->min_stats.health = data.health_base;
		a->max_stats.health = data.health_base + data.health_inc;
		a->min_stats.physics_skill = data.quantum_physics_base;
		a->max_stats.physics_skill = data.quantum_physics_base + data.quantum_physics_inc;
		a->min_stats.psi_attack = data.psi_attack_base;
		a->max_stats.psi_attack = data.psi_attack_base + data.psi_attack_inc;
		a->min_stats.psi_defence = data.psi_defense_base;
		a->max_stats.psi_defence = data.psi_defense_base + data.psi_defense_inc;
		a->min_stats.psi_energy = data.psi_energy_base;
		a->max_stats.psi_energy = data.psi_energy_base + data.psi_energy_inc;
		a->min_stats.reactions = data.reactions_base;
		a->max_stats.reactions = data.reactions_base + data.reactions_inc;
		a->min_stats.speed = data.speed_base;
		a->max_stats.speed = data.speed_base + data.speed_inc;
		a->min_stats.stamina = data.stamina_base;
		a->max_stats.stamina = data.stamina_base + data.stamina_inc;
		a->min_stats.strength = data.strength_base;
		a->max_stats.strength = data.strength_base + data.strength_inc;

		// Queen is stationary and large so we also check height
		a->large = (data.loftemps_height > 40) ||
		           (data.movement_type == AGENT_MOVEMENT_TYPE_STANDART_LARGE) ||
		           (data.movement_type == AGENT_MOVEMENT_TYPE_FLYING_LARGE);

		// Fill voxelmaps
		for (auto entry : voxelInfo)
		{
			// FIXME: Learn how to work with voxelmaps bigger than one tile
			// Voxelmaps for prone units are twice as long (double Y axis)
			// Long units occupy the tile they're in and the tile behind them.
			// VoxelMaps for the big units are twice on every axis.
			// Voxelmaps also rotate. Stored is the voxelmap for the default unit facing only.
			// That is, direction 0, north, or {0,-1} in different notations.
			// We should either store voxelmaps for all unit facings
			// Or rotate them when calculating collision
			if (a->large || entry.second.x > 40 || entry.first == AgentType::BodyState::Prone)
				continue;

			auto vm = mksp<VoxelMap>(Vec3<int>{24, 24, 20});
			for (int slice = 0; slice < entry.second.x / 2; slice++)
			{
				auto lofString =
					UString::format("LOFTEMPS:%s:%s:%u", loftempsFile.cStr(), loftempsTab.cStr(),
					(unsigned int)entry.second.y);
				vm->slices[slice] = fw().data->loadVoxelSlice(lofString);
			}

			a->voxelMaps[entry.first] = vm;
		}

		a->armor[AgentType::BodyPart::Body] = data.armor_body;
		a->armor[AgentType::BodyPart::Helmet] = data.armor_head;
		a->armor[AgentType::BodyPart::LeftArm] = data.armor_left;
		a->armor[AgentType::BodyPart::Legs] = data.armor_leg;
		a->armor[AgentType::BodyPart::RightArm] = data.armor_right;
		a->damage_modifier = {
		    &state,
		    UString::format("%s%s", DamageModifier::getPrefix(),
		                    canon_string(data_t.damage_modifier_names->get(data.damage_modifier)))};
		a->inventory = data.inventory == 1;

		// Fill right and left hand slots present on all units
		pushEquipmentSlot(a, 0, 6, 3, 5, AgentType::EquipmentSlotType::RightHand, AgentType::AlignmentX::Centre, AgentType::AlignmentY::Centre);
		pushEquipmentSlot(a, 12, 6, 3, 5, AgentType::EquipmentSlotType::LeftHand, AgentType::AlignmentX::Centre, AgentType::AlignmentY::Centre);

		if (!data.inventory)
		{
			if (data.equipment_sets[0] != 0xff)
			{
				// Equipment sets (built-in) have complex structure with several possible items and
				// weapons and clips defined, and unit types have 5 equipment sets to choose from.
				// However, all that is irrelevant! 
				// Game only uses equipment sets for aliens who have no inventory, they are 
				// never defined for anyone else without inventory, all 5 sets are always the same 
				// and all sets only contain a built-in weapon which is always self-recharging. 
				// Therefore, we only need to store built-in weapons for both hands and that's it!
				auto es_data = data_t.agent_equipment_set_built_in->get(data.equipment_sets[0]);

				if (es_data.weapons[0].weapon_idx != 0xffffffff)
					a->built_in_weapon_right = {
					    &state, UString::format("%s%s", AEquipmentType::getPrefix(),
					                            canon_string(data_u.agent_equipment_names->get(
					                                es_data.weapons[0].weapon_idx)))};
				if (es_data.weapons[1].weapon_idx != 0xffffffff)
					a->built_in_weapon_left = {
					    &state, UString::format("%s%s", AEquipmentType::getPrefix(),
					                            canon_string(data_u.agent_equipment_names->get(
					                                es_data.weapons[1].weapon_idx)))};
			}
		}
		else
		{
			// FIXIT: Can humanoids with inventory other than X-Com wear armor in vanilla? Probably not? 
			// But why not let them for fun? :)))) Armored flying anthropod FTW!

			// Armor
			pushEquipmentSlot(a, 6, 3, 2, 2, AgentType::EquipmentSlotType::ArmorHelmet, AgentType::AlignmentX::Centre, AgentType::AlignmentY::Centre);
			pushEquipmentSlot(a, 4, 5, 2, 6, AgentType::EquipmentSlotType::ArmorRightHand, AgentType::AlignmentX::Right, AgentType::AlignmentY::Centre);
			pushEquipmentSlot(a, 6, 5, 2, 6, AgentType::EquipmentSlotType::ArmorBody, AgentType::AlignmentX::Centre, AgentType::AlignmentY::Centre);
			pushEquipmentSlot(a, 8, 5, 2, 6, AgentType::EquipmentSlotType::ArmorLeftHand, AgentType::AlignmentX::Left, AgentType::AlignmentY::Centre);
			pushEquipmentSlot(a, 4, 11, 6, 5, AgentType::EquipmentSlotType::ArmorLegs, AgentType::AlignmentX::Centre, AgentType::AlignmentY::Top);
			// Shoulders
			pushEquipmentSlot(a, 4, 2);		
			pushEquipmentSlot(a, 10, 2);
			// Belt
			for (int i = 0; i < 4; i++)
			{
				pushEquipmentSlot(a, i, 12);
				pushEquipmentSlot(a, 12 + i, 12);
			}
			pushEquipmentSlot(a, 0, 11);
			pushEquipmentSlot(a, 12 + 3, 13);
			// Special
			pushEquipmentSlot(a, 2, 14, 2, 2, AgentType::EquipmentSlotType::General, AgentType::AlignmentX::Centre, AgentType::AlignmentY::Centre);
			pushEquipmentSlot(a, 11, 14, 2, 2, AgentType::EquipmentSlotType::General, AgentType::AlignmentX::Centre, AgentType::AlignmentY::Centre);
			// Backpack
			for (int i = 0; i < 4; i++)
			{
				for (int j = 0; j < 5; j++)
				{
					pushEquipmentSlot(a, 12 + i, 0 + j);
				}
			}
		}

		a->can_improve = false;
		a->score = data.score;

		state.agent_types[id] = a;
	}
}

} // namespace OpenApoc

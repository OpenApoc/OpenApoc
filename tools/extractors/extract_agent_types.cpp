#include "framework/data.h"
#include "framework/framework.h"
#include "game/state/battle/ai/aitype.h"
#include "game/state/gamestate.h"
#include "game/state/rules/aequipmenttype.h"
#include "game/state/rules/battle/battleunitimagepack.h"
#include "game/state/shared/agent.h"
#include "library/strings_format.h"
#include "library/voxel.h"
#include "tools/extractors/common/tacp.h"
#include "tools/extractors/common/ufo2p.h"
#include "tools/extractors/extractors.h"
#include <algorithm>
#include <climits>

#define HUMAN_FEMALE_PORTRAIT_START 0
#define HUMAN_FEMALE_PORTRAIT_END 30
//#define HYBRID_FEMALE_PORTRAIT_START  30
//#define HYBRID_FEMALE_PORTRAIT_END  35
#define HUMAN_MALE_PORTRAIT_START 35
#define HUMAN_MALE_PORTRAIT_END 65
#define HYBRID_MALE_PORTRAIT_START 65
#define HYBRID_MALE_PORTRAIT_END 70
#define ANDROID_MALE_PORTRAIT_START 70
#define ANDROID_MALE_PORTRAIT_END 75
#define ALIEN_PORTRAIT_OFFSET 74

#define UNIT_TYPE_BIOCHEMIST 1
#define UNIT_TYPE_ENGINEER 2
#define UNIT_TYPE_QUANTUM_PHYSIST 3
#define UNIT_TYPE_GANG_LEADER 4
#define UNIT_TYPE_CORPORATE_BOSS 5
#define UNIT_TYPE_CULT_LEADER 6
#define UNIT_TYPE_POLITICIAN 7
#define UNIT_TYPE_CHIEF_OF_POLICE 8
#define UNIT_TYPE_CORPORATE_HOOD 9
#define UNIT_TYPE_POLICE 10
#define UNIT_TYPE_GANGSTER 11
#define UNIT_TYPE_CULTIST 12
#define UNIT_TYPE_SECURITY 13
#define UNIT_TYPE_ANDROID 14
#define UNIT_TYPE_GREY 15
#define UNIT_TYPE_UPPER_CLASS_FEMALE_1 16
#define UNIT_TYPE_UPPER_CLASS_FEMALE_2 17
#define UNIT_TYPE_UPPER_CLASS_FEMALE_3 18
#define UNIT_TYPE_UPPER_CLASS_MALE_1 19
#define UNIT_TYPE_UPPER_CLASS_MALE_2 20
#define UNIT_TYPE_UPPER_CLASS_MALE_3 21
#define UNIT_TYPE_CIVILIAN_FEMALE_1 22
#define UNIT_TYPE_CIVILIAN_FEMALE_2 23
#define UNIT_TYPE_CIVILIAN_FEMALE_3 24
#define UNIT_TYPE_CIVILIAN_MALE_1 25
#define UNIT_TYPE_CIVILIAN_MALE_2 26
#define UNIT_TYPE_CIVILIAN_MALE_3 27
#define UNIT_TYPE_LOWER_CLASS_MALE_1 28
#define UNIT_TYPE_LOWER_CLASS_MALE_2 29
#define UNIT_TYPE_LOWER_CLASS_MALE_3 30
#define UNIT_TYPE_LOWER_CLASS_FEMALE_1 31
#define UNIT_TYPE_LOWER_CLASS_FEMALE_2 32
#define UNIT_TYPE_LOWER_CLASS_FEMALE_3 33
#define UNIT_TYPE_MULTIWORM_EGG 34
#define UNIT_TYPE_BRAINSUCKER 35
#define UNIT_TYPE_MULTIWORM 36
#define UNIT_TYPE_HYPERWORM 37
#define UNIT_TYPE_CHRYSALIS 38
#define UNIT_TYPE_ANTHROPOD 39
#define UNIT_TYPE_SKELETOID 40
#define UNIT_TYPE_SPITTER 41
#define UNIT_TYPE_POPPER 42
#define UNIT_TYPE_MEGASPAWN 43
#define UNIT_TYPE_PSIMORPH 44
#define UNIT_TYPE_QUEENSPAWN 45
#define UNIT_TYPE_MICRONOID 46

namespace OpenApoc
{

void fillAgentImagePacksByDefault(GameState &state, sp<AgentType> a, UString imagePackName)
{
	a->image_packs[a->image_packs.size() - 1][BodyPart::Body] = {
	    &state, format("%s%s%s", BattleUnitImagePack::getPrefix(), imagePackName, "a")};
	a->image_packs[a->image_packs.size() - 1][BodyPart::Legs] = {
	    &state, format("%s%s%s", BattleUnitImagePack::getPrefix(), imagePackName, "b")};
	a->image_packs[a->image_packs.size() - 1][BodyPart::Helmet] = {
	    &state, format("%s%s%s", BattleUnitImagePack::getPrefix(), imagePackName, "c")};
	a->image_packs[a->image_packs.size() - 1][BodyPart::LeftArm] = {
	    &state, format("%s%s%s", BattleUnitImagePack::getPrefix(), imagePackName, "d")};
	a->image_packs[a->image_packs.size() - 1][BodyPart::RightArm] = {
	    &state, format("%s%s%s", BattleUnitImagePack::getPrefix(), imagePackName, "e")};
}

void pushEquipmentSlot(sp<AgentEquipmentLayout> a, int x, int y, int w = 1, int h = 1,
                       EquipmentSlotType type = EquipmentSlotType::General,
                       AlignmentX align_x = AlignmentX::Left, AlignmentY align_y = AlignmentY::Top)
{
	a->slots.emplace_back();
	auto &outSlot = a->slots.back();
	outSlot.type = type;
	outSlot.align_x = align_x;
	outSlot.align_y = align_y;
	outSlot.bounds = {x, y, x + w, y + h};
}

void InitialGameStateExtractor::extractAgentTypes(GameState &state) const
{
	const UString loftempsFile = "xcom3/tacdata/loftemps.dat";
	const UString loftempsTab = "xcom3/tacdata/loftemps.tab";

	auto &data_t = this->tacp;
	auto &data_u = this->ufo2p;

	// Portraits

	auto portraitSmallTabFileName = UString("xcom3/ufodata/agntico.tab");
	auto portraitSmallTabFile = fw().data->fs.open(portraitSmallTabFileName);
	if (!portraitSmallTabFile)
	{
		LogError("Failed to open small portrait TAB file \"%s\"", portraitSmallTabFileName);
		return;
	}
	size_t portraitSmallCount = portraitSmallTabFile.size() / 4;

	auto portraitLargeTabFileName = UString("xcom3/ufodata/photo.tab");
	auto portraitLargeTabFile = fw().data->fs.open(portraitLargeTabFileName);
	if (!portraitLargeTabFile)
	{
		LogError("Failed to open Large portrait TAB file \"%s\"", portraitLargeTabFileName);
		return;
	}
	size_t portraitLargeCount = portraitLargeTabFile.size() / 4;

	std::vector<AgentPortrait> portraits;

	for (unsigned i = 0; i < portraitSmallCount; i++)
	{
		auto p = AgentPortrait();
		p.icon = fw().data->loadImage(format(
		    "PCK:xcom3/ufodata/agntico.pck:xcom3/ufodata/agntico.tab:%d:xcom3/ufodata/pal_01.dat",
		    i));
		if (i < portraitLargeCount)
			p.photo = fw().data->loadImage(format(
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
		UString id = format("%s%s", AgentType::getPrefix(), canon_string(a->name));

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

		// Control and AI
		a->allowsDirectControl = true;
		switch (i)
		{
			case UNIT_TYPE_BIOCHEMIST:
			case UNIT_TYPE_ENGINEER:
			case UNIT_TYPE_QUANTUM_PHYSIST:
			case UNIT_TYPE_UPPER_CLASS_FEMALE_1:
			case UNIT_TYPE_UPPER_CLASS_FEMALE_2:
			case UNIT_TYPE_UPPER_CLASS_FEMALE_3:
			case UNIT_TYPE_UPPER_CLASS_MALE_1:
			case UNIT_TYPE_UPPER_CLASS_MALE_2:
			case UNIT_TYPE_UPPER_CLASS_MALE_3:
			case UNIT_TYPE_CIVILIAN_FEMALE_1:
			case UNIT_TYPE_CIVILIAN_FEMALE_2:
			case UNIT_TYPE_CIVILIAN_FEMALE_3:
			case UNIT_TYPE_CIVILIAN_MALE_1:
			case UNIT_TYPE_CIVILIAN_MALE_2:
			case UNIT_TYPE_CIVILIAN_MALE_3:
			case UNIT_TYPE_LOWER_CLASS_MALE_1:
			case UNIT_TYPE_LOWER_CLASS_MALE_2:
			case UNIT_TYPE_LOWER_CLASS_MALE_3:
			case UNIT_TYPE_LOWER_CLASS_FEMALE_1:
			case UNIT_TYPE_LOWER_CLASS_FEMALE_2:
			case UNIT_TYPE_LOWER_CLASS_FEMALE_3:
			case UNIT_TYPE_ANDROID:
			case UNIT_TYPE_GREY:
			case UNIT_TYPE_CORPORATE_BOSS:
				a->aiType = AIType::Civilian;
				break;
			case UNIT_TYPE_CHRYSALIS:
				a->aiType = AIType::None;
				break;
			case UNIT_TYPE_MULTIWORM:
			case UNIT_TYPE_HYPERWORM:
			case UNIT_TYPE_POPPER:
			case UNIT_TYPE_MEGASPAWN:
			case UNIT_TYPE_PSIMORPH:
			case UNIT_TYPE_QUEENSPAWN:
			case UNIT_TYPE_BRAINSUCKER:
			case UNIT_TYPE_MULTIWORM_EGG:
			case UNIT_TYPE_MICRONOID:
				a->aiType = AIType::Loner;
				break;
			case UNIT_TYPE_GANG_LEADER:
			case UNIT_TYPE_CULT_LEADER:
			case UNIT_TYPE_POLITICIAN:
			case UNIT_TYPE_CHIEF_OF_POLICE:
			case UNIT_TYPE_CORPORATE_HOOD:
			case UNIT_TYPE_POLICE:
			case UNIT_TYPE_GANGSTER:
			case UNIT_TYPE_CULTIST:
			case UNIT_TYPE_SECURITY:
			case UNIT_TYPE_ANTHROPOD:
			case UNIT_TYPE_SKELETOID:
			case UNIT_TYPE_SPITTER:
				a->aiType = AIType::Group;
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
			case UNIT_TYPE_CHRYSALIS:
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

		// Allowed body states, voxel maps and animations
		// also infiltration and growth chances
		a->appearance_count = 1;
		UString bodyTypeName = "";
		int infiltrationID = -1;
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
					// Android differs from other civilians only in that it has 4 possible
					// appearances
					// They all use same animation and differ in head image sets
					a->appearance_count = 4;
					a->animation_packs.emplace_back(
					    &state, format("%s%s", BattleUnitAnimationPack::getPrefix(), "civ"));
					a->animation_packs.emplace_back(
					    &state, format("%s%s", BattleUnitAnimationPack::getPrefix(), "civ"));
					a->animation_packs.emplace_back(
					    &state, format("%s%s", BattleUnitAnimationPack::getPrefix(), "civ"));
				}
				a->animation_packs.emplace_back(
				    &state, format("%s%s", BattleUnitAnimationPack::getPrefix(), "civ"));
				if (i == UNIT_TYPE_GREY)
				{
					bodyTypeName = "GREY";
				}
				else
				{
					bodyTypeName = "CIVILIAN";
				}
				break;

			// Stationary aliens
			case UNIT_TYPE_MULTIWORM_EGG:
				a->appearance_count = 2;
				a->animation_packs.emplace_back(
				    &state, format("%s%s", BattleUnitAnimationPack::getPrefix(), "mwegg1"));
				a->animation_packs.emplace_back(
				    &state, format("%s%s", BattleUnitAnimationPack::getPrefix(), "mwegg2"));
				bodyTypeName = "MULTIWORM_EGG";
				infiltrationID = 0;
				a->growthChance = 20;
				a->growthOptions.emplace_back(
				    100, std::pair<StateRef<AgentType>, int>({&state, "AGENTTYPE_MULTIWORM"}, 1));
				a->detectionWeight = 1;
				a->movementPercent = 40;
				break;
			case UNIT_TYPE_CHRYSALIS:
				a->appearance_count = 2;
				a->animation_packs.emplace_back(
				    &state, format("%s%s", BattleUnitAnimationPack::getPrefix(), "chrys1"));
				a->animation_packs.emplace_back(
				    &state, format("%s%s", BattleUnitAnimationPack::getPrefix(), "chrys2"));
				bodyTypeName = "CHRYSALIS";
				infiltrationID = 4;
				a->growthChance = 12;
				a->growthOptions.emplace_back(
				    20, std::pair<StateRef<AgentType>, int>({&state, "AGENTTYPE_BRAINSUCKER"}, 1));
				a->growthOptions.emplace_back(
				    60, std::pair<StateRef<AgentType>, int>({&state, "AGENTTYPE_ANTHROPOD"}, 1));
				a->growthOptions.emplace_back(
				    80, std::pair<StateRef<AgentType>, int>({&state, "AGENTTYPE_SPITTER"}, 1));
				a->growthOptions.emplace_back(
				    100, std::pair<StateRef<AgentType>, int>({&state, "AGENTTYPE_POPPER"}, 1));
				a->detectionWeight = 1;
				a->movementPercent = 0;
				break;
			case UNIT_TYPE_QUEENSPAWN:
				a->animation_packs.emplace_back(
				    &state, format("%s%s", BattleUnitAnimationPack::getPrefix(), "queen"));
				bodyTypeName = "QUEENSPAWN";
				infiltrationID = 11;
				a->growthChance = 0;
				a->detectionWeight = 20;
				a->missionObjective = true;
				a->movementPercent = 0;
				break;
			// Non-humanoid aliens
			case UNIT_TYPE_BRAINSUCKER:
				a->animation_packs.emplace_back(
				    &state, format("%sbsk", BattleUnitAnimationPack::getPrefix()));
				bodyTypeName = "BRAINSUCKER";
				infiltrationID = 1;
				a->growthChance = 20;
				a->detectionWeight = 1;
				a->growthInfiltration = 1;
				a->movementPercent = 40;
				break;
			case UNIT_TYPE_HYPERWORM:
				a->animation_packs.emplace_back(
				    &state, format("%shypr", BattleUnitAnimationPack::getPrefix()));
				bodyTypeName = "HYPERWORM";
				infiltrationID = 3;
				a->growthChance = 12;
				a->growthOptions.emplace_back(
				    100, std::pair<StateRef<AgentType>, int>({&state, "AGENTTYPE_CHRYSALIS"}, 1));
				a->detectionWeight = 1;
				a->movementPercent = 33;
				break;
			case UNIT_TYPE_SPITTER:
				a->animation_packs.emplace_back(
				    &state, format("%sspitr", BattleUnitAnimationPack::getPrefix()));
				bodyTypeName = "SPITTER";
				infiltrationID = 7;
				a->growthChance = 2;
				a->detectionWeight = 3;
				a->movementPercent = 33;
				break;
			case UNIT_TYPE_POPPER:
				a->animation_packs.emplace_back(
				    &state, format("%spopper", BattleUnitAnimationPack::getPrefix()));
				bodyTypeName = "POPPER";
				infiltrationID = 8;
				a->growthChance = 2;
				a->detectionWeight = 2;
				a->movementPercent = 33;
				break;
			case UNIT_TYPE_MICRONOID:
				a->animation_packs.emplace_back(
				    &state, format("%smicro", BattleUnitAnimationPack::getPrefix()));
				bodyTypeName = "MICRONOID";
				infiltrationID = 12;
				a->growthChance = 0;
				a->detectionWeight = 1;
				a->movementPercent = 30;
				break;
			// Special case: Multiworm, can only crawl
			case UNIT_TYPE_MULTIWORM:
				a->animation_packs.emplace_back(
				    &state, format("%s%s", BattleUnitAnimationPack::getPrefix(), "multi"));
				bodyTypeName = "MULTIWORM";
				infiltrationID = 2;
				a->growthChance = 12;
				a->growthOptions.emplace_back(
				    100, std::pair<StateRef<AgentType>, int>({&state, "AGENTTYPE_HYPERWORM"}, 4));
				a->detectionWeight = 3;
				a->movementPercent = 33;
				break;
			// Special case: Megaspawn, can strafe
			case UNIT_TYPE_MEGASPAWN:
				a->animation_packs.emplace_back(
				    &state, format("%s%s", BattleUnitAnimationPack::getPrefix(), "mega"));
				bodyTypeName = "MEGASPAWN";
				infiltrationID = 9;
				a->growthChance = 2;
				a->detectionWeight = 10;
				a->movementPercent = 0;
				break;

			// Special case: Psimorph, non-humanoid that can only fly
			case UNIT_TYPE_PSIMORPH:
				a->animation_packs.emplace_back(
				    &state, format("%s%s", BattleUnitAnimationPack::getPrefix(), "psi"));
				bodyTypeName = "PSIMORPH";
				infiltrationID = 10;
				a->growthChance = 2;
				a->detectionWeight = 8;
				a->movementPercent = 0;
				break;

			// Skeletoid and Anthropod are both humanoids
			case UNIT_TYPE_SKELETOID:
			case UNIT_TYPE_ANTHROPOD:
				infiltrationID = 5;
				a->growthChance = 2;
				a->detectionWeight = 3;
				a->movementPercent = 33;
			// Other humans
			default:
				if (i == UNIT_TYPE_SKELETOID)
				{
					bodyTypeName = "FLYING_HUMANOID";
					infiltrationID = 6;
				}
				else
				{
					bodyTypeName = "WALKING_HUMANOID";
				}
				// Gangsters have 2 appearances
				if (i == UNIT_TYPE_GANGSTER)
				{
					a->appearance_count = 2;
					a->animation_packs.emplace_back(
					    &state, format("%s%s", BattleUnitAnimationPack::getPrefix(), "unit"));
				}
				a->animation_packs.emplace_back(
				    &state, format("%s%s", BattleUnitAnimationPack::getPrefix(), "unit"));
				break;
		}

		a->bodyType = {&state,
		               format("%s%s", AgentBodyType::getPrefix(), canon_string(bodyTypeName))};

		if (infiltrationID != -1)
		{
			// infiltration
			auto idata = data_u.infiltration_speed_agent->get(infiltrationID);
			a->infiltrationSpeed = idata.speed;
		}

		// Used shadow packs
		switch (i)
		{
			// Aliens with unique shadows
			case UNIT_TYPE_BRAINSUCKER:
				a->shadow_pack = {&state, format("%s%s", BattleUnitImagePack::getPrefix(), "bsks")};
				break;
			case UNIT_TYPE_HYPERWORM:
				a->shadow_pack = {&state,
				                  format("%s%s", BattleUnitImagePack::getPrefix(), "hyprs")};
				break;
			case UNIT_TYPE_SPITTER:
				a->shadow_pack = {&state,
				                  format("%s%s", BattleUnitImagePack::getPrefix(), "spitrs")};
				break;
			case UNIT_TYPE_POPPER:
				a->shadow_pack = {&state,
				                  format("%s%s", BattleUnitImagePack::getPrefix(), "poppers")};
				break;
			case UNIT_TYPE_MEGASPAWN:
				a->shadow_pack = {&state,
				                  format("%s%s", BattleUnitImagePack::getPrefix(), "megas")};
				break;
			case UNIT_TYPE_PSIMORPH:
				a->shadow_pack = {&state, format("%s%s", BattleUnitImagePack::getPrefix(), "psis")};
				break;

			// Aliens with no shadows
			case UNIT_TYPE_CHRYSALIS:
			case UNIT_TYPE_MULTIWORM_EGG:
			case UNIT_TYPE_MULTIWORM:
			case UNIT_TYPE_QUEENSPAWN:
			case UNIT_TYPE_MICRONOID:
				break;

			// Humanoid aliens and humans
			default:
				a->shadow_pack = {&state,
				                  format("%s%s", BattleUnitImagePack::getPrefix(), "shadow")};
				break;
		}

		// Used image packs and BGs
		a->inventoryBackground = fw().data->loadImage("xcom3/tacdata/equippic/generic.pcx");
		a->image_packs.push_back(std::map<BodyPart, StateRef<BattleUnitImagePack>>());
		switch (i)
		{
			case UNIT_TYPE_BIOCHEMIST:
			case UNIT_TYPE_ENGINEER:
			case UNIT_TYPE_QUANTUM_PHYSIST:
				a->inventoryBackground = fw().data->loadImage("xcom3/tacdata/equippic/scien.pcx");
				a->image_packs[0][BodyPart::Body] = {
				    &state, format("%s%s", BattleUnitImagePack::getPrefix(), "scntst")};
				break;
			case UNIT_TYPE_GANG_LEADER:
				fillAgentImagePacksByDefault(state, a, "gangl");
				break;
			case UNIT_TYPE_CORPORATE_BOSS:
				// Game has no picture for this unit.
				// I think rm1 (upper class male) fits him best.
				a->image_packs[0][BodyPart::Body] = {
				    &state, format("%s%s", BattleUnitImagePack::getPrefix(), "rm1")};
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
				fillAgentImagePacksByDefault(state, a, "polic");
				break;
			case UNIT_TYPE_GANGSTER:
				fillAgentImagePacksByDefault(state, a, "gang");
				a->image_packs.push_back(std::map<BodyPart, StateRef<BattleUnitImagePack>>());
				fillAgentImagePacksByDefault(state, a, "gang2");
				break;
			case UNIT_TYPE_CULTIST:
				fillAgentImagePacksByDefault(state, a, "cult");
				break;
			case UNIT_TYPE_SECURITY:
				fillAgentImagePacksByDefault(state, a, "sec");
				break;
			case UNIT_TYPE_ANDROID:
				a->image_packs[0][BodyPart::Body] = {
				    &state, format("%s%s", BattleUnitImagePack::getPrefix(), "robot")};
				a->image_packs[0][BodyPart::Helmet] = {
				    &state, format("%s%s", BattleUnitImagePack::getPrefix(), "robo1")};
				a->image_packs.push_back(std::map<BodyPart, StateRef<BattleUnitImagePack>>());
				a->image_packs[1][BodyPart::Body] = {
				    &state, format("%s%s", BattleUnitImagePack::getPrefix(), "robot")};
				a->image_packs[1][BodyPart::Helmet] = {
				    &state, format("%s%s", BattleUnitImagePack::getPrefix(), "robo2")};
				a->image_packs.push_back(std::map<BodyPart, StateRef<BattleUnitImagePack>>());
				a->image_packs[2][BodyPart::Body] = {
				    &state, format("%s%s", BattleUnitImagePack::getPrefix(), "robot")};
				a->image_packs[2][BodyPart::Helmet] = {
				    &state, format("%s%s", BattleUnitImagePack::getPrefix(), "robo3")};
				a->image_packs.push_back(std::map<BodyPart, StateRef<BattleUnitImagePack>>());
				a->image_packs[3][BodyPart::Body] = {
				    &state, format("%s%s", BattleUnitImagePack::getPrefix(), "robot")};
				a->image_packs[3][BodyPart::Helmet] = {
				    &state, format("%s%s", BattleUnitImagePack::getPrefix(), "robo4")};
				break;
			case UNIT_TYPE_GREY:
				a->image_packs[0][BodyPart::Body] = {
				    &state, format("%s%s", BattleUnitImagePack::getPrefix(), "grey")};
				break;
			case UNIT_TYPE_UPPER_CLASS_FEMALE_1:
				a->image_packs[0][BodyPart::Body] = {
				    &state, format("%s%s", BattleUnitImagePack::getPrefix(), "rw1")};
				break;
			case UNIT_TYPE_UPPER_CLASS_FEMALE_2:
				a->image_packs[0][BodyPart::Body] = {
				    &state, format("%s%s", BattleUnitImagePack::getPrefix(), "rw2")};
				break;
			case UNIT_TYPE_UPPER_CLASS_FEMALE_3:
				a->image_packs[0][BodyPart::Body] = {
				    &state, format("%s%s", BattleUnitImagePack::getPrefix(), "rw3")};
				break;
			case UNIT_TYPE_UPPER_CLASS_MALE_1:
				a->image_packs[0][BodyPart::Body] = {
				    &state, format("%s%s", BattleUnitImagePack::getPrefix(), "rm1")};
				break;
			case UNIT_TYPE_UPPER_CLASS_MALE_2:
				a->image_packs[0][BodyPart::Body] = {
				    &state, format("%s%s", BattleUnitImagePack::getPrefix(), "rm2")};
				break;
			case UNIT_TYPE_UPPER_CLASS_MALE_3:
				a->image_packs[0][BodyPart::Body] = {
				    &state, format("%s%s", BattleUnitImagePack::getPrefix(), "rm3")};
				break;
			case UNIT_TYPE_CIVILIAN_FEMALE_1:
				a->image_packs[0][BodyPart::Body] = {
				    &state, format("%s%s", BattleUnitImagePack::getPrefix(), "nw1")};
				break;
			case UNIT_TYPE_CIVILIAN_FEMALE_2:
				a->image_packs[0][BodyPart::Body] = {
				    &state, format("%s%s", BattleUnitImagePack::getPrefix(), "nw2")};
				break;
			case UNIT_TYPE_CIVILIAN_FEMALE_3:
				a->image_packs[0][BodyPart::Body] = {
				    &state, format("%s%s", BattleUnitImagePack::getPrefix(), "nw3")};
				break;
			case UNIT_TYPE_CIVILIAN_MALE_1:
				a->image_packs[0][BodyPart::Body] = {
				    &state, format("%s%s", BattleUnitImagePack::getPrefix(), "nm1")};
				break;
			case UNIT_TYPE_CIVILIAN_MALE_2:
				a->image_packs[0][BodyPart::Body] = {
				    &state, format("%s%s", BattleUnitImagePack::getPrefix(), "nm2")};
				break;
			case UNIT_TYPE_CIVILIAN_MALE_3:
				a->image_packs[0][BodyPart::Body] = {
				    &state, format("%s%s", BattleUnitImagePack::getPrefix(), "nm3")};
				break;
			case UNIT_TYPE_LOWER_CLASS_MALE_1:
				a->image_packs[0][BodyPart::Body] = {
				    &state, format("%s%s", BattleUnitImagePack::getPrefix(), "sm1")};
				break;
			case UNIT_TYPE_LOWER_CLASS_MALE_2:
				a->image_packs[0][BodyPart::Body] = {
				    &state, format("%s%s", BattleUnitImagePack::getPrefix(), "sm2")};
				break;
			case UNIT_TYPE_LOWER_CLASS_MALE_3:
				a->image_packs[0][BodyPart::Body] = {
				    &state, format("%s%s", BattleUnitImagePack::getPrefix(), "sm3")};
				break;
			case UNIT_TYPE_LOWER_CLASS_FEMALE_1:
				a->image_packs[0][BodyPart::Body] = {
				    &state, format("%s%s", BattleUnitImagePack::getPrefix(), "sw1")};
				break;
			case UNIT_TYPE_LOWER_CLASS_FEMALE_2:
				a->image_packs[0][BodyPart::Body] = {
				    &state, format("%s%s", BattleUnitImagePack::getPrefix(), "sw2")};
				break;
			case UNIT_TYPE_LOWER_CLASS_FEMALE_3:
				a->image_packs[0][BodyPart::Body] = {
				    &state, format("%s%s", BattleUnitImagePack::getPrefix(), "sw3")};
				break;
			case UNIT_TYPE_MULTIWORM_EGG:
				a->inventoryBackground =
				    fw().data->loadImage("xcom3/tacdata/equippic/mwormegg.pcx");
				a->image_packs[0][BodyPart::Body] = {
				    &state, format("%s%s", BattleUnitImagePack::getPrefix(), "mwegga")};
				a->image_packs[0][BodyPart::Helmet] = {
				    &state, format("%s%s", BattleUnitImagePack::getPrefix(), "mweggb")};
				a->image_packs.push_back(std::map<BodyPart, StateRef<BattleUnitImagePack>>());
				a->image_packs[1][BodyPart::Body] = {
				    &state, format("%s%s", BattleUnitImagePack::getPrefix(), "mwegga")};
				a->image_packs[1][BodyPart::Helmet] = {
				    &state, format("%s%s", BattleUnitImagePack::getPrefix(), "mweggb")};
				break;
			case UNIT_TYPE_BRAINSUCKER:
				a->inventoryBackground = fw().data->loadImage("xcom3/tacdata/equippic/sucker.pcx");
				a->image_packs[0][BodyPart::Body] = {
				    &state, format("%s%s", BattleUnitImagePack::getPrefix(), "bsk")};
				break;
			case UNIT_TYPE_MULTIWORM:
				a->inventoryBackground = fw().data->loadImage("xcom3/tacdata/equippic/mworm.pcx");
				a->image_packs[0][BodyPart::Body] = {
				    &state, format("%s%s", BattleUnitImagePack::getPrefix(), "multi")};
				break;
			case UNIT_TYPE_HYPERWORM:
				a->inventoryBackground = fw().data->loadImage("xcom3/tacdata/equippic/hyperwm.pcx");
				a->image_packs[0][BodyPart::Body] = {
				    &state, format("%s%s", BattleUnitImagePack::getPrefix(), "hypr")};
				break;
			case UNIT_TYPE_CHRYSALIS:
				a->inventoryBackground =
				    fw().data->loadImage("xcom3/tacdata/equippic/chrysali.pcx");
				a->image_packs[0][BodyPart::Body] = {
				    &state, format("%s%s", BattleUnitImagePack::getPrefix(), "chrysa")};
				a->image_packs[0][BodyPart::Helmet] = {
				    &state, format("%s%s", BattleUnitImagePack::getPrefix(), "chrysb")};
				a->image_packs.push_back(std::map<BodyPart, StateRef<BattleUnitImagePack>>());
				a->image_packs[1][BodyPart::Body] = {
				    &state, format("%s%s", BattleUnitImagePack::getPrefix(), "chrysa")};
				a->image_packs[1][BodyPart::Helmet] = {
				    &state, format("%s%s", BattleUnitImagePack::getPrefix(), "chrysb")};
				break;
			case UNIT_TYPE_ANTHROPOD:
				a->inventoryBackground =
				    fw().data->loadImage("xcom3/tacdata/equippic/anthropd.pcx");
				fillAgentImagePacksByDefault(state, a, "antrp");
				break;
			case UNIT_TYPE_SKELETOID:
				a->inventoryBackground = fw().data->loadImage("xcom3/tacdata/equippic/skelly.pcx");
				fillAgentImagePacksByDefault(state, a, "skel");
				break;
			case UNIT_TYPE_SPITTER:
				a->inventoryBackground = fw().data->loadImage("xcom3/tacdata/equippic/spitter.pcx");
				a->image_packs[0][BodyPart::Body] = {
				    &state, format("%s%s", BattleUnitImagePack::getPrefix(), "spitr")};
				break;
			case UNIT_TYPE_POPPER:
				a->inventoryBackground = fw().data->loadImage("xcom3/tacdata/equippic/popper.pcx");
				a->image_packs[0][BodyPart::Body] = {
				    &state, format("%s%s", BattleUnitImagePack::getPrefix(), "popper")};
				break;
			case UNIT_TYPE_MEGASPAWN:
				a->inventoryBackground =
				    fw().data->loadImage("xcom3/tacdata/equippic/megatron.pcx");
				fillAgentImagePacksByDefault(state, a, "mega");
				// Megaspawn has no head (stupid!)
				a->image_packs[0][BodyPart::Helmet].clear();
				break;
			case UNIT_TYPE_PSIMORPH:
				a->inventoryBackground = fw().data->loadImage("xcom3/tacdata/equippic/psi_m.pcx");
				a->image_packs[0][BodyPart::Body] = {
				    &state, format("%s%s", BattleUnitImagePack::getPrefix(), "psi")};
				break;
			case UNIT_TYPE_QUEENSPAWN:
				a->inventoryBackground = fw().data->loadImage("xcom3/tacdata/equippic/queen.pcx");
				a->image_packs[0][BodyPart::Body] = {
				    &state, format("%s%s", BattleUnitImagePack::getPrefix(), "queena")};
				a->image_packs[0][BodyPart::Legs] = {
				    &state, format("%s%s", BattleUnitImagePack::getPrefix(), "queenb")};
				break;
			case UNIT_TYPE_MICRONOID:
				a->inventoryBackground =
				    fw().data->loadImage("xcom3/tacdata/equippic/agregate.pcx");
				a->image_packs[0][BodyPart::Body] = {
				    &state, format("%s%s", BattleUnitImagePack::getPrefix(), "micro")};
				break;
		}

		// Hazards
		if (i == UNIT_TYPE_POPPER)
		{
			a->spreadHazardDamageType = {&state, "DAMAGETYPE_SMOKE"};
			a->spreadHazardMinPower = 1;
			a->spreadHazardMaxPower = 1;
			a->spreadHazardTTLDivizor = 2;
		}

		// Sounds

		// Aliens have no damage and fatal damage sounds
		// By default, all aliens have 2 walk, 2 cry and 1 die sound
		UString walkSfxName = "";
		int walkSfxCount = 2;
		bool walkSfxNumbered = true;
		UString crySfxName = "";
		int crySfxCount = 2;
		bool crySfxNumbered = true;
		UString dieSfxName = "";
		// Assign sound names to aliens
		switch (i)
		{
			case UNIT_TYPE_MULTIWORM_EGG:
				walkSfxCount = 0;
				crySfxCount = 0;
				dieSfxName = "wormegg";
				break;
			case UNIT_TYPE_BRAINSUCKER:
				// walkSfxName = "scuttle";
				// walkSfxNumbered = false;
				// walkSfxCount = 1;
				walkSfxCount = 0;
				crySfxName = "brnsukr";
				dieSfxName = "brnsukr2";
				break;
			case UNIT_TYPE_MULTIWORM:
				walkSfxName = "multworm";
				walkSfxNumbered = false;
				crySfxName = "mworm";
				dieSfxName = "gunk";
				break;
			case UNIT_TYPE_HYPERWORM:
				walkSfxName = "hypworm";
				walkSfxCount = 1;
				crySfxCount = 0;
				dieSfxName = "hypwrm01";
				break;
			case UNIT_TYPE_CHRYSALIS:
				walkSfxCount = 0;
				crySfxName = "saliscry";
				crySfxNumbered = false;
				dieSfxName = "chrysals";
				break;
			case UNIT_TYPE_ANTHROPOD:
				walkSfxName = "anthrop";
				walkSfxCount = 1;
				crySfxName = "anthrpd";
				dieSfxName = "anthrpd1";
				break;
			case UNIT_TYPE_SKELETOID:
				walkSfxName = "skelstp";
				crySfxName = "skeletld";
				crySfxNumbered = false;
				dieSfxName = "skeletd1";
				break;
			case UNIT_TYPE_SPITTER:
				walkSfxName = "spitter";
				crySfxName = "spittr";
				dieSfxName = "spittr1";
				break;
			case UNIT_TYPE_POPPER:
				walkSfxName = "popper";
				crySfxName = "popper";
				dieSfxName = "";
				break;
			case UNIT_TYPE_MEGASPAWN:
				walkSfxName = "megtron";
				crySfxName = "megtron";
				crySfxNumbered = false;
				dieSfxName = "meghowl";
				break;
			case UNIT_TYPE_PSIMORPH:
				walkSfxCount = 0;
				crySfxName = "psimrph";
				dieSfxName = "howl";
				break;
			case UNIT_TYPE_QUEENSPAWN:
				walkSfxName = "queenspn"; // Even though queen is static
				walkSfxNumbered = false;
				crySfxName = "queensp";
				crySfxCount = 1;
				dieSfxName = "queensp2";
				break;
			case UNIT_TYPE_MICRONOID:
				walkSfxCount = 0;
				crySfxCount = 0;
				dieSfxName = "";
				break;
			// All humans
			default:
				// Humans will be assigned sounds manually
				break;
		}

		// Load sounds
		switch (i)
		{
			case UNIT_TYPE_MULTIWORM_EGG:
			case UNIT_TYPE_BRAINSUCKER:
			case UNIT_TYPE_MULTIWORM:
			case UNIT_TYPE_HYPERWORM:
			case UNIT_TYPE_CHRYSALIS:
			case UNIT_TYPE_ANTHROPOD:
			case UNIT_TYPE_SKELETOID:
			case UNIT_TYPE_SPITTER:
			case UNIT_TYPE_POPPER:
			case UNIT_TYPE_MEGASPAWN:
			case UNIT_TYPE_PSIMORPH:
			case UNIT_TYPE_QUEENSPAWN:
			case UNIT_TYPE_MICRONOID:
				if (walkSfxNumbered)
					for (int i = 1; i <= walkSfxCount; i++)
						a->walkSfx.push_back(fw().data->loadSample(format(
						    "RAWSOUND:xcom3/rawsound/tactical/aliens/movemnts/%s%d.raw:22050",
						    walkSfxName, i)));
				else
					a->walkSfx.push_back(fw().data->loadSample(
					    format("RAWSOUND:xcom3/rawsound/tactical/aliens/movemnts/%s.raw:22050",
					           walkSfxName)));
				if (crySfxNumbered)
					for (int i = 1; i <= crySfxCount; i++)
						a->crySfx.push_back(fw().data->loadSample(
						    format("RAWSOUND:xcom3/rawsound/tactical/aliens/cries/%s%d.raw:22050",
						           crySfxName, i)));
				else
					a->crySfx.push_back(fw().data->loadSample(format(
					    "RAWSOUND:xcom3/rawsound/tactical/aliens/cries/%s.raw:22050", crySfxName)));
				if (dieSfxName.length() > 0)
					a->dieSfx[AgentType::Gender::Male].push_back(fw().data->loadSample(
					    format("RAWSOUND:xcom3/rawsound/tactical/aliens/deaths/%s.raw:22050",
					           dieSfxName)));
				break;
			default:
				// Humans have no walk sounds and no cries
				for (int i = 1; i <= 3; i++)
				{
					a->damageSfx[AgentType::Gender::Male].push_back(fw().data->loadSample(
					    format("RAWSOUND:xcom3/rawsound/extra/hpainm%d.raw:22050", i)));
					a->fatalWoundSfx[AgentType::Gender::Male].push_back(fw().data->loadSample(
					    format("RAWSOUND:xcom3/rawsound/extra/hdethm%d.raw:22050", i)));
					a->dieSfx[AgentType::Gender::Male].push_back(fw().data->loadSample(
					    format("RAWSOUND:xcom3/rawsound/extra/hdethm%d.raw:22050", i)));
				}
				for (int i = 1; i <= 2; i++)
				{
					a->damageSfx[AgentType::Gender::Female].push_back(fw().data->loadSample(
					    format("RAWSOUND:xcom3/rawsound/extra/hpainf%d.raw:22050", i)));
					a->fatalWoundSfx[AgentType::Gender::Female].push_back(fw().data->loadSample(
					    format("RAWSOUND:xcom3/rawsound/extra/hdethf%d.raw:22050", i)));
					a->dieSfx[AgentType::Gender::Female].push_back(fw().data->loadSample(
					    format("RAWSOUND:xcom3/rawsound/extra/hdethf%d.raw:22050", i)));
				}
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

		a->armor[BodyPart::Body] = data.armor_body;
		a->armor[BodyPart::Helmet] = data.armor_head;
		a->armor[BodyPart::LeftArm] = data.armor_left;
		a->armor[BodyPart::Legs] = data.armor_leg;
		a->armor[BodyPart::RightArm] = data.armor_right;
		a->damage_modifier = {
		    &state, format("%s%s", DamageModifier::getPrefix(),
		                   canon_string(data_t.damage_modifier_names->get(data.damage_modifier)))};
		a->inventory = data.inventory == 1;

		UString name = "";

		if (!data.inventory)
		{
			if (id == "AGENTTYPE_BRAINSUCKER")
			{
				a->built_in_weapon_right = {&state, "AEQUIPMENTTYPE_BRAINSUCKER_WEAPON"};
				name = "BUILTIN";
			}
			else if (data.equipment_sets[0] == 0xff)
			{
				// Units with no inventory and no built-in equipment sets do not have any slots
				name = "NONE";
			}
			else
			{
				// Equipment sets (built-in) have complex structure with several possible items
				// and
				// weapons and clips defined, and unit types have 5 equipment sets to choose
				// from.
				// However, all that is irrelevant!
				// Game only uses equipment sets for aliens who have no inventory, they are
				// never defined for anyone else without inventory, all 5 sets are always the
				// same
				// and all sets only contain a built-in weapon which is always self-recharging.
				// Therefore, we only need to store built-in weapons for both hands and that's
				// it!
				auto es_data = data_t.agent_equipment_set_built_in->get(data.equipment_sets[0]);

				if (es_data.weapons[0].weapon_idx != 0xffffffff)
					a->built_in_weapon_right = {
					    &state, format("%s%s", AEquipmentType::getPrefix(),
					                   canon_string(data_u.agent_equipment_names->get(
					                       es_data.weapons[0].weapon_idx)))};
				if (id == "AGENTTYPE_MULTIWORM")
				{
					a->built_in_weapon_left = {&state, "AEQUIPMENTTYPE_MULTIWORM_BURST"};
				}
				else if (es_data.weapons[1].weapon_idx != 0xffffffff)
					a->built_in_weapon_left = {
					    &state, format("%s%s", AEquipmentType::getPrefix(),
					                   canon_string(data_u.agent_equipment_names->get(
					                       es_data.weapons[1].weapon_idx)))};
				name = "BUILTIN";
			}
		}
		else
		{
			name = "FULL";
		}
		a->equipment_layout = {
		    &state, format("%s%s", AgentEquipmentLayout::getPrefix(), canon_string(name))};

		a->score = data.score;

		// Bodies
		int liveIdx = 0;
		int deadIdx = 0;
		int liveSpace = 0;
		int deadSpace = 0;
		switch (i)
		{
			case UNIT_TYPE_ANTHROPOD:
				liveIdx = 0;
				deadIdx = 14;
				liveSpace = 1;
				deadSpace = 1;
				break;
			case UNIT_TYPE_BRAINSUCKER:
				liveIdx = 1;
				deadIdx = 15;
				liveSpace = 1;
				deadSpace = 1;
				break;
			case UNIT_TYPE_CHRYSALIS:
				liveIdx = 2;
				deadIdx = 16;
				liveSpace = 2;
				deadSpace = 2;
				break;
			case UNIT_TYPE_MEGASPAWN:
				liveIdx = 3;
				deadIdx = 17;
				liveSpace = 12;
				deadSpace = 12;
				break;
			case UNIT_TYPE_MULTIWORM_EGG:
				liveIdx = 4;
				deadIdx = 18;
				liveSpace = 2;
				deadSpace = 2;
				break;
			case UNIT_TYPE_HYPERWORM:
				liveIdx = 5;
				deadIdx = 19;
				liveSpace = 1;
				deadSpace = 1;
				break;
			case UNIT_TYPE_MULTIWORM:
				liveIdx = 6;
				deadIdx = 20;
				liveSpace = 5;
				deadSpace = 5;
				break;
			// OVerspawn 7 / ?
			case UNIT_TYPE_POPPER:
				liveIdx = 8;
				deadIdx = 21;
				liveSpace = 1;
				deadSpace = 1;
				break;
			case UNIT_TYPE_PSIMORPH:
				liveIdx = 9;
				deadIdx = 22;
				liveSpace = 5;
				deadSpace = 12;
				break;
			case UNIT_TYPE_QUEENSPAWN:
				liveIdx = 10;
				deadIdx = 23;
				liveSpace = 20;
				deadSpace = 20;
				break;
			case UNIT_TYPE_SKELETOID:
				liveIdx = 11;
				deadIdx = 24;
				liveSpace = 1;
				deadSpace = 1;
				break;
			case UNIT_TYPE_SPITTER:
				liveIdx = 12;
				deadIdx = 25;
				liveSpace = 1;
				deadSpace = 1;
				break;
			case UNIT_TYPE_MICRONOID:
				liveIdx = 13;
				deadIdx = 26;
				liveSpace = 1;
				deadSpace = 1;
				break;
		}
		switch (i)
		{
			case UNIT_TYPE_MULTIWORM_EGG:
			case UNIT_TYPE_BRAINSUCKER:
			case UNIT_TYPE_MULTIWORM:
			case UNIT_TYPE_HYPERWORM:
			case UNIT_TYPE_CHRYSALIS:
			case UNIT_TYPE_ANTHROPOD:
			case UNIT_TYPE_SKELETOID:
			case UNIT_TYPE_SPITTER:
			case UNIT_TYPE_POPPER:
			case UNIT_TYPE_MEGASPAWN:
			case UNIT_TYPE_PSIMORPH:
			case UNIT_TYPE_QUEENSPAWN:
			case UNIT_TYPE_MICRONOID:
			{
				auto liveName =
				    format("%s%s_ALIVE", AEquipmentType::getPrefix(), canon_string(a->name));
				auto deadName =
				    format("%s%s_DEAD", AEquipmentType::getPrefix(), canon_string(a->name));

				auto liveItem = mksp<AEquipmentType>();
				liveItem->bioStorage = true;
				liveItem->equipscreen_size = {6, 5};
				liveItem->equipscreen_sprite =
				    fw().data->loadImage(format("PCK:xcom3/ufodata/contico.pck:xcom3/ufodata/"
				                                "contico.tab:%d:xcom3/ufodata/research.pcx",
				                                liveIdx));
				liveItem->store_space = liveSpace;
				liveItem->type = AEquipmentType::Type::Loot;
				liveItem->bioRemains = {&state, deadName};
				liveItem->name = format("Live %s", a->name);
				liveItem->score = a->score;
				state.agent_equipment[liveName] = liveItem;
				a->liveSpeciesItem = {&state, liveName};

				auto deadItem = mksp<AEquipmentType>();
				deadItem->bioStorage = true;
				deadItem->equipscreen_size = {6, 5};
				deadItem->equipscreen_sprite =
				    fw().data->loadImage(format("PCK:xcom3/ufodata/contico.pck:xcom3/ufodata/"
				                                "contico.tab:%d:xcom3/ufodata/research.pcx",
				                                deadIdx));
				deadItem->store_space = deadSpace;
				deadItem->type = AEquipmentType::Type::Loot;
				deadItem->name = format("Dead %s", a->name);
				deadItem->score = 0;
				state.agent_equipment[deadName] = deadItem;
				a->deadSpeciesItem = {&state, deadName};

				break;
			}
			default:
				break;
		}

		state.agent_types[id] = a;
	}

	// None layout slot
	{
		UString name = "NONE";
		UString id = format("%s%s", AgentEquipmentLayout::getPrefix(), canon_string(name));

		auto a = mksp<AgentEquipmentLayout>();

		state.agent_equipment_layouts[id] = a;
	}

	// Builtin layout slot
	{
		UString name = "BUILTIN";
		UString id = format("%s%s", AgentEquipmentLayout::getPrefix(), canon_string(name));

		auto a = mksp<AgentEquipmentLayout>();
		// Located off-screen, invisible in inventory
		pushEquipmentSlot(a, 1024, 6, 3, 5, EquipmentSlotType::RightHand, AlignmentX::Centre,
		                  AlignmentY::Centre);
		pushEquipmentSlot(a, 1028, 6, 3, 5, EquipmentSlotType::LeftHand, AlignmentX::Centre,
		                  AlignmentY::Centre);

		state.agent_equipment_layouts[id] = a;
	}

	// FULL layout slot
	{
		UString name = "FULL";
		UString id = format("%s%s", AgentEquipmentLayout::getPrefix(), canon_string(name));

		auto a = mksp<AgentEquipmentLayout>();
		pushEquipmentSlot(a, 1, 6, 3, 5, EquipmentSlotType::RightHand, AlignmentX::Centre,
		                  AlignmentY::Centre);
		pushEquipmentSlot(a, 12, 6, 3, 5, EquipmentSlotType::LeftHand, AlignmentX::Centre,
		                  AlignmentY::Centre);

		// Can humanoids with inventory other than X-Com wear armor in vanilla? Probably not?
		// But why not let them for fun? :)))) Armored flying anthropod FTW!

		// Armor
		pushEquipmentSlot(a, 7, 3, 2, 2, EquipmentSlotType::ArmorHelmet, AlignmentX::Centre,
		                  AlignmentY::Centre);
		pushEquipmentSlot(a, 4, 5, 2, 6, EquipmentSlotType::ArmorRightHand, AlignmentX::Right,
		                  AlignmentY::Centre);
		pushEquipmentSlot(a, 6, 5, 4, 4, EquipmentSlotType::ArmorBody, AlignmentX::Centre,
		                  AlignmentY::Centre);
		pushEquipmentSlot(a, 10, 5, 2, 6, EquipmentSlotType::ArmorLeftHand, AlignmentX::Left,
		                  AlignmentY::Centre);
		pushEquipmentSlot(a, 6, 9, 4, 7, EquipmentSlotType::ArmorLegs, AlignmentX::Centre,
		                  AlignmentY::Top);

		// Belt #1
		for (int i = 0; i < 3; i++)
		{
			pushEquipmentSlot(a, 1 + i, 12);
		}
		pushEquipmentSlot(a, 1, 13);

		// Special
		pushEquipmentSlot(a, 3, 14, 2, 2, EquipmentSlotType::General, AlignmentX::Centre,
		                  AlignmentY::Centre);
		pushEquipmentSlot(a, 11, 14, 2, 2, EquipmentSlotType::General, AlignmentX::Centre,
		                  AlignmentY::Centre);
		// Belt #2
		for (int i = 0; i < 3; i++)
		{
			pushEquipmentSlot(a, 12 + i, 12);
		}
		pushEquipmentSlot(a, 12 + 2, 13);

		// Shoulders
		pushEquipmentSlot(a, 5, 2);
		pushEquipmentSlot(a, 10, 2);

		// Backpack
		for (int j = 0; j < 5; j++)
		{
			for (int i = 0; i < 4; i++)
			{
				pushEquipmentSlot(a, 12 + i, 0 + j);
			}
		}

		state.agent_equipment_layouts[id] = a;
	}

	// Initial aliens
	for (int difficulty = 0; difficulty < 5; difficulty++)
	{
		state.initial_aliens[difficulty].emplace_back(
		    StateRef<AgentType>(&state, "AGENTTYPE_BRAINSUCKER"), Vec2<int>{1, difficulty / 2 + 2});
		state.initial_aliens[difficulty].emplace_back(
		    StateRef<AgentType>(&state, "AGENTTYPE_ANTHROPOD"), Vec2<int>{1, difficulty / 2 + 2});
	}
}

void InitialGameStateExtractor::extractAgentBodyTypes(GameState &state) const
{
	const UString loftempsFile = "xcom3/tacdata/loftemps.dat";
	const UString loftempsTab = "xcom3/tacdata/loftemps.tab";

	for (int i = 0; i <= 46; i++)
	{
		UString name = "";
		switch (i)
		{
			// Civilians
			case UNIT_TYPE_ANDROID:
				name = "CIVILIAN";
				break;
			case UNIT_TYPE_GREY:
				name = "GREY";
				break;
			// Stationary aliens
			case UNIT_TYPE_MULTIWORM_EGG:
				name = "MULTIWORM_EGG";
				break;
			case UNIT_TYPE_CHRYSALIS:
				name = "CHRYSALIS";
				break;
			case UNIT_TYPE_QUEENSPAWN:
				name = "QUEENSPAWN";
				break;
			// Non-humanoid aliens
			case UNIT_TYPE_BRAINSUCKER:
				name = "BRAINSUCKER";
				break;
			case UNIT_TYPE_HYPERWORM:
				name = "HYPERWORM";
				break;
			case UNIT_TYPE_SPITTER:
				name = "SPITTER";
				break;
			case UNIT_TYPE_POPPER:
				name = "POPPER";
				break;
			case UNIT_TYPE_MICRONOID:
				name = "MICRONOID";
				break;
			// Special case: Multiworm, can only crawl
			case UNIT_TYPE_MULTIWORM:
				name = "MULTIWORM";
				break;
			// Special case: Megaspawn, large walker
			case UNIT_TYPE_MEGASPAWN:
				name = "MEGASPAWN";
				break;
			// Special case: Psimorph, non-humanoid that can only fly
			case UNIT_TYPE_PSIMORPH:
				name = "PSIMORPH";
				break;
			// Skeletoid and Anthropod are both humanoids
			case UNIT_TYPE_SKELETOID:
				name = "FLYING_HUMANOID";
				break;
			case UNIT_TYPE_ANTHROPOD:
				name = "WALKING_HUMANOID";
				break;
			default:
				continue;
		}

		auto a = mksp<AgentBodyType>();
		a->allowed_movement_states.insert(MovementState::None);
		a->allowed_body_states.insert(BodyState::Dead);

		UString id = format("%s%s", AgentBodyType::getPrefix(), canon_string(name));

		// Allowed facings (nothing means everything allowed)
		switch (i)
		{
			case UNIT_TYPE_CHRYSALIS:
				a->allowed_facing.push_back({});
				a->allowed_facing.push_back({});
				a->allowed_facing[0].insert({0, 1});
				a->allowed_facing[1].insert({-1, 0});
				break;
			case UNIT_TYPE_QUEENSPAWN:
				a->allowed_facing.push_back({});
				a->allowed_facing[0].insert({1, 0});
				a->allowed_facing[0].insert({1, 1});
				a->allowed_facing[0].insert({0, 1});
				a->allowed_facing[0].insert({-1, 1});
				a->allowed_facing[0].insert({-1, 0});
				break;
			default:
				break;
		}

		// Allowed body states, voxel maps and animations
		// Contains information about each body state: how high it is,
		// and what voxel map idx is specified in the game files
		std::map<BodyState, Vec2<int>> voxelInfo;
		int height = 0;
		int idx = 0;
		switch (i)
		{
			// Civilians
			case UNIT_TYPE_ANDROID:
			case UNIT_TYPE_GREY:
				height = i == UNIT_TYPE_ANDROID ? 32 : 24;
				idx = 5;
				a->allowed_movement_states.insert(MovementState::Normal);
				a->allowed_movement_states.insert(MovementState::Running);
				a->allowed_body_states.insert(BodyState::Standing);
				a->allowed_body_states.insert(BodyState::Downed);
				voxelInfo[BodyState::Standing] = {height, idx};
				voxelInfo[BodyState::Downed] = {8, idx};
				voxelInfo[BodyState::Dead] = {1, idx};
				break;

			// Stationary aliens
			case UNIT_TYPE_MULTIWORM_EGG:
			case UNIT_TYPE_CHRYSALIS:
			case UNIT_TYPE_QUEENSPAWN:
				switch (i)
				{
					case UNIT_TYPE_MULTIWORM_EGG:
						height = 18;
						idx = 10;
						break;
					case UNIT_TYPE_CHRYSALIS:
						height = 20;
						idx = 10;
						break;
					case UNIT_TYPE_QUEENSPAWN:
						height = 70;
						idx = 20;
						break;
				}
				a->allowed_fire_movement_states.insert(MovementState::None);
				a->allowed_body_states.insert(i == UNIT_TYPE_CHRYSALIS ? BodyState::Prone
				                                                       : BodyState::Standing);
				a->allowed_body_states.insert(BodyState::Downed);
				voxelInfo[i == UNIT_TYPE_CHRYSALIS ? BodyState::Prone : BodyState::Standing] = {
				    height, idx};
				voxelInfo[BodyState::Downed] = {(i == UNIT_TYPE_QUEENSPAWN) ? 16 : 8, idx};
				break;

			// Non-humanoid aliens
			case UNIT_TYPE_BRAINSUCKER:
				height = 10;
				idx = 3;
				a->allowed_movement_states.insert(MovementState::Normal);
				a->allowed_movement_states.insert(MovementState::Running);
				a->allowed_movement_states.insert(MovementState::Brainsuck);
				a->allowed_body_states.insert(BodyState::Standing);
				a->allowed_body_states.insert(BodyState::Throwing);
				a->allowed_body_states.insert(BodyState::Jumping);
				a->allowed_body_states.insert(BodyState::Downed);
				voxelInfo[BodyState::Standing] = {height, idx};
				voxelInfo[BodyState::Jumping] = {height, idx};
				voxelInfo[BodyState::Throwing] = {8, idx};
				voxelInfo[BodyState::Downed] = {8, idx};
				voxelInfo[BodyState::Dead] = {1, idx};
				break;
			case UNIT_TYPE_HYPERWORM:
			case UNIT_TYPE_SPITTER:
			case UNIT_TYPE_POPPER:
			case UNIT_TYPE_MICRONOID:
				switch (i)
				{
					case UNIT_TYPE_HYPERWORM:
						height = 10;
						idx = 4;
						a->allowed_fire_movement_states.insert(MovementState::None);
						a->allowed_fire_movement_states.insert(MovementState::Normal);
						break;
					case UNIT_TYPE_SPITTER:
						height = 32;
						idx = 5;
						a->allowed_fire_movement_states.insert(MovementState::None);
						break;
					case UNIT_TYPE_POPPER:
						height = 16;
						idx = 5;
						break;
					case UNIT_TYPE_MICRONOID:
						height = 8;
						idx = 5;
						break;
				}
				a->allowed_movement_states.insert(MovementState::Normal);
				a->allowed_movement_states.insert(MovementState::Running);
				a->allowed_body_states.insert(BodyState::Standing);
				a->allowed_body_states.insert(BodyState::Downed);
				voxelInfo[BodyState::Standing] = {height, idx};
				voxelInfo[BodyState::Downed] = {8, idx};
				voxelInfo[BodyState::Dead] = {1, idx};
				break;

			// Special case: Multiworm, can only crawl
			case UNIT_TYPE_MULTIWORM:
				height = 16;
				idx = 10;
				a->allowed_movement_states.insert(MovementState::Normal);
				a->allowed_fire_movement_states.insert(MovementState::None);
				a->allowed_body_states.insert(BodyState::Kneeling);
				a->allowed_body_states.insert(BodyState::Prone);
				a->allowed_body_states.insert(BodyState::Downed);
				voxelInfo[BodyState::Kneeling] = {height, idx};
				voxelInfo[BodyState::Prone] = {height, idx};
				voxelInfo[BodyState::Downed] = {8, idx};
				voxelInfo[BodyState::Dead] = {1, idx};
				break;

			// Special case: Megaspawn, large walker
			case UNIT_TYPE_MEGASPAWN:
				height = 70;
				idx = 19;
				a->allowed_movement_states.insert(MovementState::Normal);
				a->allowed_movement_states.insert(MovementState::Running);
				a->allowed_fire_movement_states.insert(MovementState::None);
				a->allowed_body_states.insert(BodyState::Standing);
				a->allowed_body_states.insert(BodyState::Downed);
				voxelInfo[BodyState::Standing] = {height, idx};
				voxelInfo[BodyState::Downed] = {16, idx};
				voxelInfo[BodyState::Dead] = {1, idx};
				break;

			// Special case: Psimorph, non-humanoid that can only fly
			case UNIT_TYPE_PSIMORPH:
				height = 70;
				idx = 19;
				a->allowed_movement_states.insert(MovementState::Normal);
				a->allowed_movement_states.insert(MovementState::Running);
				a->allowed_body_states.insert(BodyState::Flying);
				a->allowed_body_states.insert(BodyState::Downed);
				voxelInfo[BodyState::Flying] = {height, idx};
				voxelInfo[BodyState::Downed] = {16, idx};
				voxelInfo[BodyState::Dead] = {1, idx};
				break;

			// Skeletoid and Anthropod are both humanoids
			case UNIT_TYPE_SKELETOID:
				// Skeletoid only differs from other humanods by having built-in flight capabilities
				a->allowed_body_states.insert(BodyState::Flying);
			// Other humans
			case UNIT_TYPE_ANTHROPOD:
				height = 32;
				idx = 5;
				a->allowed_movement_states.insert(MovementState::Normal);
				a->allowed_movement_states.insert(MovementState::Running);
				a->allowed_movement_states.insert(MovementState::Strafing);
				a->allowed_movement_states.insert(MovementState::Reverse);
				a->allowed_fire_movement_states.insert(MovementState::None);
				a->allowed_fire_movement_states.insert(MovementState::Normal);
				a->allowed_body_states.insert(BodyState::Standing);
				a->allowed_body_states.insert(BodyState::Kneeling);
				a->allowed_body_states.insert(BodyState::Prone);
				a->allowed_body_states.insert(BodyState::Jumping);
				a->allowed_body_states.insert(BodyState::Throwing);
				a->allowed_body_states.insert(BodyState::Downed);
				voxelInfo[BodyState::Standing] = {height, idx};
				voxelInfo[BodyState::Kneeling] = {std::max(8, height - 10), idx};
				voxelInfo[BodyState::Prone] = {16, 10};
				voxelInfo[BodyState::Jumping] = {std::max(8, height - 10), idx};
				voxelInfo[BodyState::Throwing] = {height, idx};
				voxelInfo[BodyState::Downed] = {8, idx};
				voxelInfo[BodyState::Dead] = {1, idx};
				// Humanoids can possibly attain flight by use of armor, therefore include this here
				voxelInfo[BodyState::Flying] = {height, idx};
				break;
		}
		a->maxHeight = height;
		a->large = height > 40;

		// Alexey Andronov (Istrebitel)
		// About voxelmaps for agents...
		//
		// Game uses voxelmaps 3, 4, 5, 10, 19 and 20:
		// - 3,4,5 are circles of various sizes. They're used by most units.
		// - 10 is a vertical "log". It's used by multiworm, chrysalis and egg.
		//   It is obviously used when facing north and obviously when
		//	 the worm is moving (considered to be prone)
		// - 19 and 20 are horizontal "logs". They're used by big units
		//	 (psimorph, queen, megaspawn). They differ only in how the log is
		//   located on Y axis(19 is a bit higher than 20) so we will treat them
		//	 as identical (besides, 20 is only used by queen which is
		//	 the rarest unit in the game, only encountered once)
		//
		// Since we are using voxelmaps of the same size, we must use multiples
		// for big units and prone units (which take 2 to 4 tiles)
		// Since we are not rotating voxelmaps, we must prepare voxelmaps
		// for every of the 8 possible unit facings (or however much unit has)
		//
		// Adjustments:
		// - 3,4,5 obviously need no adjustments
		// - chrysalis and egg's are left as is, they're irrelevant
		// - 10:
		//   when facing N or S - 10 stacked twice
		//	 when facing NE or SW - 45, 102, 102, 25
		//	 when facing E or W - 19 stacked twice
		//	 when facing NW or SE - 98, 55, 35, 98
		// - 19 and 20:
		//	 when facing N or S - 19 (in each tile)
		//	 when facing NE or SW - 98 (in each tile)
		//	 when facing E or W - 10 (in each tile)
		//	 when facing NW or SE - 102 (in each tile)

		for (auto &entry : voxelInfo)
		{
			// We increase height for the purpose of voxelmaps
			// Otherwise we fire through units in front of us
			a->height[entry.first] =
			    entry.first == BodyState::Downed ? entry.second.x : entry.second.x + 4;
			a->muzzleZPosition[entry.first] = entry.second.x;

			if (a->large)
			{
				if (entry.first == BodyState::Prone)
				{
					LogError("Large units cannot go prone!");
				}
				switch (entry.second.y)
				{
					case 19:
					case 20:
					{
						// Map of facings to loftemps indexes, as described above
						static const std::map<Vec2<int>, int> facingToLoftemps = {
						    {{0, -1}, 19}, {{0, 1}, 19},  {{-1, 0}, 10},   {{1, 0}, 10},
						    {{1, -1}, 98}, {{-1, 1}, 98}, {{-1, -1}, 102}, {{1, 1}, 102},
						};

						// For each facing
						for (auto &pair : facingToLoftemps)
						{
							// Large units are 2x2x2
							a->size[entry.first][pair.first] = {2, 2, 2};

							// For each voxelmap
							a->voxelMaps[entry.first][pair.first] = std::vector<sp<VoxelMap>>(8);
							for (int x = 0; x < 2; x++)
							{
								for (int y = 0; y < 2; y++)
								{
									for (int z = 0; z < 2; z++)
									{
										// Create voxelmap
										a->voxelMaps[entry.first][pair.first]
										            [z * a->size[entry.first][pair.first].y *
										                 a->size[entry.first][pair.first].x +
										             y * a->size[entry.first][pair.first].x + x] =
										    std::make_shared<VoxelMap>(Vec3<int>{24, 24, 20});
										// Fill slices
										int limit =
										    std::max(20, (a->height[entry.first] - 40 * z) / 2);
										for (int i = 0; i < limit; i++)
										{
											a->voxelMaps[entry.first][pair.first]
											            [z * a->size[entry.first][pair.first].y *
											                 a->size[entry.first][pair.first].x +
											             y * a->size[entry.first][pair.first].x + x]
											                ->setSlice(
											                    i, fw().data->loadVoxelSlice(format(
											                           "LOFTEMPS:xcom3/tacdata/"
											                           "loftemps.dat:xcom3/"
											                           "tacdata/loftemps.tab:%d",
											                           pair.second)));
										}
									}
								}
							}
						}
					}
					break;
					default:
						LogError(
						    "Large units cannot have loftemps other than 19 or 20! encountered %d.",
						    entry.second.y);
						break;
				}
			}    // end of large unit
			else // a->large = false
			{
				switch (entry.first)
				{
					case BodyState::Dead:
					case BodyState::Downed:
					case BodyState::Flying:
					case BodyState::Jumping:
					case BodyState::Kneeling:
					case BodyState::Standing:
					case BodyState::Throwing:
						// For each facing
						for (int fx = -1; fx <= 1; fx++)
						{
							for (int fy = -1; fy <= 1; fy++)
							{
								if (fx == 0 && fy == 0)
								{
									continue;
								}
								Vec2<int> facing = {fx, fy};
								a->size[entry.first][facing] = {1, 1, 1};

								// Create voxelmap
								a->voxelMaps[entry.first][facing] = std::vector<sp<VoxelMap>>(1);
								a->voxelMaps[entry.first][facing][0] =
								    std::make_shared<VoxelMap>(Vec3<int>{24, 24, 20});
								// Fill slices
								for (int i = 0; i < (a->height[entry.first]) / 2; i++)
								{
									a->voxelMaps[entry.first][facing][0]->setSlice(
									    i, fw().data->loadVoxelSlice(format(
									           "LOFTEMPS:xcom3/tacdata/loftemps.dat:xcom3/tacdata/"
									           "loftemps.tab:%d",
									           entry.second.y)));
								}
							}
						}
						break;
					case BodyState::Prone:
					{
						// Map of facings to loftemps indexes, as described above
						static const std::map<Vec2<int>, std::vector<int>> facingToLoftemps = {
						    {{0, -1}, {10, 10}},           {{0, 1}, {10, 10}},
						    {{-1, 0}, {19, 19}},           {{1, 0}, {19, 19}},
						    {{1, -1}, {45, 102, 102, 25}}, {{-1, 1}, {45, 102, 102, 25}},
						    {{-1, -1}, {98, 55, 35, 98}},  {{1, 1}, {98, 55, 35, 98}},
						};

						// Map of facings to loftemps indexes, as described above
						static const std::map<Vec2<int>, Vec3<int>> facingToSize = {
						    {{0, -1}, {1, 2, 1}},  {{0, 1}, {1, 2, 1}},  {{-1, 0}, {2, 1, 1}},
						    {{1, 0}, {2, 1, 1}},   {{1, -1}, {2, 2, 1}}, {{-1, 1}, {2, 2, 1}},
						    {{-1, -1}, {2, 2, 1}}, {{1, 1}, {2, 2, 1}},
						};

						// For each facing
						for (auto &pair : facingToLoftemps)
						{
							// Fill unit size in this body state
							a->size[entry.first][pair.first] = facingToSize.at(pair.first);

							// Create and fill slices
							a->voxelMaps[entry.first][pair.first] =
							    std::vector<sp<VoxelMap>>(pair.second.size());
							for (unsigned int j = 0; j < pair.second.size(); j++)
							{
								a->voxelMaps[entry.first][pair.first][j] =
								    std::make_shared<VoxelMap>(Vec3<int>{24, 24, 20});
								for (int i = 0; i < a->height[entry.first] / 2; i++)
								{
									a->voxelMaps[entry.first][pair.first][j]->setSlice(
									    i, fw().data->loadVoxelSlice(format(
									           "LOFTEMPS:xcom3/tacdata/loftemps.dat:xcom3/tacdata/"
									           "loftemps.tab:%d",
									           pair.second[j])));
								}
							}
						}
					}
					break;
				} // end of switch body state
			}     // end of small unit
		}         // end of for each voxelInfo entry

		state.agent_body_types[id] = a;
	}
}

} // namespace OpenApoc

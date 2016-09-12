#include "framework/framework.h"
#include "game/state/agent.h"
#include "game/state/rules/aequipment_type.h"
#include "tools/extractors/common/tacp.h"
#include "tools/extractors/common/ufo2p.h"
#include "tools/extractors/extractors.h"
#include <climits>

namespace OpenApoc
{
void InitialGameStateExtractor::extractAgentTypes(GameState &state, Difficulty difficulty)
{
	const int HUMAN_FEMALE_PORTRAIT_START = 0;
	const int HUMAN_FEMALE_PORTRAIT_END = 30;
	const int HYBRID_FEMALE_PORTRAIT_START = 30;
	const int HYBRID_FEMALE_PORTRAIT_END = 35;
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
	const int UNIT_TYPE_UPPER_CLASS_FEMALE_1 = 16;
	const int UNIT_TYPE_UPPER_CLASS_FEMALE_2 = 17;
	const int UNIT_TYPE_UPPER_CLASS_FEMALE_3 = 18;
	const int UNIT_TYPE_CIVILIAN_FEMALE_1 = 22;
	const int UNIT_TYPE_CIVILIAN_FEMALE_2 = 23;
	const int UNIT_TYPE_CIVILIAN_FEMALE_3 = 24;
	const int UNIT_TYPE_LOWER_CLASS_FEMALE_1 = 31;
	const int UNIT_TYPE_LOWER_CLASS_FEMALE_2 = 32;
	const int UNIT_TYPE_LOWER_CLASS_FEMALE_3 = 33;

	const int UNIT_TYPE_FIRST_ALIEN_ENTRY = 34;

	const UString loftempsFile = "xcom3/TACDATA/LOFTEMPS.DAT";
	const UString loftempsTab = "xcom3/TACDATA/LOFTEMPS.TAB";

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
	for (unsigned i = 1; i < data_u.agent_types->count(); i++)
	{
		auto a = mksp<AgentType>();
		auto data = data_u.agent_types->get(i);

		a->name = data_u.agent_type_names->get(i);
		UString id = UString::format("%s%s", AgentType::getPrefix(), canon_string(a->name));

		a->id = id;

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
				a->portraits[AgentType::Gender::Female][0] = portraits[data.image];
				break;
			default:
				a->possible_genders.insert(a->possible_genders.end(), AgentType::Gender::Male);
				a->gender_chance[AgentType::Gender::Male] = 1;
				// Aliens start at id 34, their portraits start at 75,
				// first alien has image index of 1 rather than 0, so we shift by 74
				if (i >= UNIT_TYPE_FIRST_ALIEN_ENTRY)
					a->portraits[AgentType::Gender::Male][0] =
					    portraits[data.image + ALIEN_PORTRAIT_OFFSET];
				else
					a->portraits[AgentType::Gender::Male][0] = portraits[data.image];
				break;
		}

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

		switch (data.movement_type)
		{
			case AGENT_MOVEMENT_TYPE_STATIONARY:
				a->movement_type = AgentType::MovementType::Stationary;
				break;
			case AGENT_MOVEMENT_TYPE_STANDART:
				a->movement_type = AgentType::MovementType::Standart;
				break;
			case AGENT_MOVEMENT_TYPE_FLYING:
				a->movement_type = AgentType::MovementType::Flying;
				break;
			case AGENT_MOVEMENT_TYPE_STANDART_LARGE:
				a->movement_type = AgentType::MovementType::StandartLarge;
				break;
			case AGENT_MOVEMENT_TYPE_FLYING_LARGE:
				a->movement_type = AgentType::MovementType::FlyingLarge;
				break;
		}
		a->large = (data.loftemps_height > 40) ||
		           (a->movement_type == AgentType::MovementType::StandartLarge) ||
		           (a->movement_type == AgentType::MovementType::FlyingLarge);

		// FIXME: Need to scale voxelmap to fit twice the size
		a->voxelMap = a->large ? mksp<VoxelMap>(Vec3<int>{48, 48, 40})
		                       : mksp<VoxelMap>(Vec3<int>{24, 24, 20});
		if (data.loftemps_idx != 0)
		{
			for (int slice = 0; slice < data.loftemps_height / 2; slice++)
			{
				auto lofString =
				    UString::format("LOFTEMPS:%s:%s:%u", loftempsFile.cStr(), loftempsTab.cStr(),
				                    (unsigned int)data.loftemps_idx);
				a->voxelMap->slices[slice] = fw().data->loadVoxelSlice(lofString);
			}
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
		if (!data.inventory)
		{
			if (data.equipment_sets[0] != 0xff)
			{
				// Equipment sets (built-in) have complex structure with several possible items and
				// weapons and clips defined, and unit types have 5 equipment sets to choose from,
				// but all that is irrelevant. Game only uses equipment sets for aliens who have no
				// inventory, they are never defined for anyone else without inventory, all 5 sets
				// are always the same and all sets only contain a built-in weapon which is always
				// self-recharging. Therefore, we only need to store built-in weapons.
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
			// FIXME: Fill layouts for units without inventory
			// That is, give them left and right hand slots!
			// std::list<EquipmentLayoutSlot> equipment_layout_slots;
		}
		else
		{
			// FIXME: Fill layouts for units with inventory
			// std::list<EquipmentLayoutSlot> equipment_layout_slots;
		}

		a->can_improve = false;
		a->score = data.score;

		// FIXME: Extract agent animation and frames

		state.agent_types[id] = a;
	}
}

} // namespace OpenApoc

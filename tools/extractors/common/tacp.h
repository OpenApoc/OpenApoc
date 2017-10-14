#pragma once

#include "game/state/rules/aequipmenttype.h"
#include "game/state/rules/battle/damage.h"
#include "tools/extractors/common/aequipment.h"
#include "tools/extractors/common/bulletsprite.h"
#include "tools/extractors/common/canonstring.h"
#include "tools/extractors/common/datachunk.h"
#include "tools/extractors/common/strtab.h"
#include <algorithm>
#include <memory>
#include <string>

namespace OpenApoc
{

class TACP
{
  public:
	TACP(std::string fileName = "xcom3/tacexe/tacp.exe");

	std::unique_ptr<StrTab> damage_modifier_names;
	std::unique_ptr<StrTab> damage_type_names;

	std::unique_ptr<DataChunk<DamageTypeData>> damage_types;
	std::unique_ptr<DataChunk<DamageModifierData>> damage_modifiers;

	std::unique_ptr<DataChunk<AgentEquipmentData>> agent_equipment;
	std::unique_ptr<DataChunk<AgentArmorData>> agent_armor;
	std::unique_ptr<DataChunk<AgentWeaponData>> agent_weapons;
	std::unique_ptr<DataChunk<AgentGeneralData>> agent_general;
	std::unique_ptr<DataChunk<AgentPayloadData>> agent_payload;

	std::unique_ptr<DataChunk<AgentEquipmentSetBuiltInData>> agent_equipment_set_built_in;
	std::unique_ptr<DataChunk<AgentEquipmentSetScoreDataAlien>> agent_equipment_set_score_alien;
	std::unique_ptr<DataChunk<AgentEquipmentSetScoreDataHuman>> agent_equipment_set_score_human;

	std::unique_ptr<DataChunk<AgentEquipmentSetScoreRequirement>>
	    agent_equipment_set_score_requirement;

	std::unique_ptr<DataChunk<BulletSprite>> bullet_sprites;

	std::unique_ptr<DataChunk<ProjectileSprites>> projectile_sprites;

	UString getDTypeId(int idx) const
	{
		return DamageType::getPrefix() + canon_string(this->damage_type_names->get(idx));
	}

	UString getDModId(int idx) const
	{
		return DamageModifier::getPrefix() + canon_string(this->damage_modifier_names->get(idx));
	}
};

// TACP &getTACPData();

} // namespace OpenApoc

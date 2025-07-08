#include "tacp.h"
#include "framework/data.h"
#include "framework/framework.h"
#include "framework/logger.h"

#include <boost/crc.hpp>
#include <iomanip>
#include <iterator>

namespace OpenApoc
{

/* This is the crc32 of the tacp.exe found on my steam version of apoc
 * It's likely there are other executables around with different CRCs, but we
 * need to make sure the offsets are the same, then we can add them to an
 * 'allowed' list, or have a map of 'known' CRCs with offsets of the various
 * tables */
uint32_t expected_tacp_crc32 = 0xfebbe39e;

TACP::TACP(std::string file_name)
{
	auto file = fw().data->fs.open(file_name);

	if (!file)
	{
		LogError("Failed to open \"{}\"", file_name);
		exit(1);
	}

	auto data = file.readAll();
	boost::crc_32_type crc;
	crc.process_bytes(data.get(), file.size());

	auto crc32 = crc.checksum();

	if (crc32 != expected_tacp_crc32)
	{
		LogError("File \"{}\"\" has an unknown crc32 value of 0x{:08x} - expected 0x{:08x}",
		         file_name, crc32, expected_tacp_crc32);
	}

	file.seekg(0, std::ios::beg);
	file.clear();

	// hand-filling damage mod names as they are not present in the game exe
	{
		auto vec = std::vector<std::string>();
		vec.emplace_back("Human");
		vec.emplace_back("Mutant");
		vec.emplace_back("Android");
		vec.emplace_back("Alien Egg");
		vec.emplace_back("Multiworm");
		vec.emplace_back("Hyperworm");
		vec.emplace_back("Chrysalis");
		vec.emplace_back("Brainsucker");
		vec.emplace_back("Queenspawn");
		vec.emplace_back("Anthropod");
		vec.emplace_back("Psimorph");
		vec.emplace_back("Spitter");
		vec.emplace_back("Megaspawn");
		vec.emplace_back("Popper");
		vec.emplace_back("Skeletoid");
		vec.emplace_back("Micronoid Aggregate");
		vec.emplace_back("Disruptor Shield");
		vec.emplace_back("Megapol Armor");
		vec.emplace_back("Marsec Armor");
		vec.emplace_back("X-COM Disruptor Armor");
		vec.emplace_back("Terrain 1?");
		vec.emplace_back("Terrain 2?");
		vec.emplace_back("Gun Emplacement");
		this->damage_modifier_names.reset(new StrTab(vec));
	}

	this->damage_type_names.reset(
	    new StrTab(file, DAMAGE_TYPE_NAMES_OFFSET_START, DAMAGE_TYPE_NAMES_OFFSET_END, true));

	this->damage_types.reset(new DataChunk<DamageTypeData>(file, DAMAGE_TYPE_DATA_OFFSET_START,
	                                                       DAMAGE_TYPE_DATA_OFFSET_END));

	this->damage_modifiers.reset(new DataChunk<DamageModifierData>(
	    file, DAMAGE_MODIFIER_DATA_OFFSET_START, DAMAGE_MODIFIER_DATA_OFFSET_END));

	this->agent_equipment.reset(new DataChunk<AgentEquipmentData>(
	    file, AGENT_EQUIPMENT_DATA_OFFSET_START, AGENT_EQUIPMENT_DATA_OFFSET_END));

	this->agent_armor.reset(new DataChunk<AgentArmorData>(file, AGENT_ARMOR_DATA_OFFSET_START,
	                                                      AGENT_ARMOR_DATA_OFFSET_END));

	this->agent_weapons.reset(new DataChunk<AgentWeaponData>(file, AGENT_WEAPON_DATA_OFFSET_START,
	                                                         AGENT_WEAPON_DATA_OFFSET_END));

	this->agent_general.reset(new DataChunk<AgentGeneralData>(file, AGENT_GENERAL_DATA_OFFSET_START,
	                                                          AGENT_GENERAL_DATA_OFFSET_END));

	this->agent_payload.reset(new DataChunk<AgentPayloadData>(file, AGENT_PAYLOAD_DATA_OFFSET_START,
	                                                          AGENT_PAYLOAD_DATA_OFFSET_END));

	this->agent_equipment_set_built_in.reset(new DataChunk<AgentEquipmentSetBuiltInData>(
	    file, AGENT_EQUIPMENT_SET_BUILTIN_DATA_OFFSET_START,
	    AGENT_EQUIPMENT_SET_BUILTIN_DATA_OFFSET_END));

	this->agent_equipment_set_score_alien.reset(new DataChunk<AgentEquipmentSetScoreDataAlien>(
	    file, AGENT_EQUIPMENT_SET_SCORE_ALIEN_DATA_OFFSET_START,
	    AGENT_EQUIPMENT_SET_SCORE_ALIEN_DATA_OFFSET_END));

	this->agent_equipment_set_score_human.reset(new DataChunk<AgentEquipmentSetScoreDataHuman>(
	    file, AGENT_EQUIPMENT_SET_SCORE_HUMAN_DATA_OFFSET_START,
	    AGENT_EQUIPMENT_SET_SCORE_HUMAN_DATA_OFFSET_END));

	this->agent_equipment_set_score_requirement.reset(
	    new DataChunk<AgentEquipmentSetScoreRequirement>(
	        file, AGENT_EQUIPMENT_SET_SCORE_REQUIREMENT_DATA_OFFSET_START,
	        AGENT_EQUIPMENT_SET_SCORE_REQUIREMENT_DATA_OFFSET_END));

	this->bullet_sprites.reset(new DataChunk<BulletSprite>(
	    file, BULLETSPRITE_DATA_TACP_OFFSET_START, BULLETSPRITE_DATA_TACP_OFFSET_END));
	this->projectile_sprites.reset(new DataChunk<ProjectileSprites>(
	    file, PROJECTILESPRITES_DATA_TACP_OFFSET_START, PROJECTILESPRITES_DATA_TACP_OFFSET_END));
}

} // namespace OpenApoc

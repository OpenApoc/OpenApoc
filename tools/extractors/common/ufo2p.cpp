#include "ufo2p.h"
#include "framework/data.h"
#include "framework/framework.h"

#include <boost/crc.hpp>
#include <iomanip>
#include <iterator>

namespace OpenApoc
{

/* This is the crc32 of the ufo2p.exe found on my steam version of apoc
 * It's likely there are other executables around with different CRCs, but we
 * need to make sure the offsets are the same, then we can add them to an
 * 'allowed' list, or have a map of 'known' CRCs with offsets of the various
 * tables */
uint32_t expected_ufo2p_crc32 = 0x4749ffc1;

UFO2P::UFO2P(std::string file_name)
{
	auto file = fw().data->fs.open(file_name);

	if (!file)
	{
		LogError("Failed to open \"%s\"", file_name.c_str());
		exit(1);
	}
	auto data = file.readAll();
	boost::crc_32_type crc;
	crc.process_bytes(data.get(), file.size());

	auto crc32 = crc.checksum();

	if (crc32 != expected_ufo2p_crc32)
	{
		LogError("File \"%s\"\" has an unknown crc32 value of 0x%08x - expected 0x%08x",
		         file_name.c_str(), crc32, expected_ufo2p_crc32);
	}

	file.seekg(0, std::ios::beg);
	file.clear();

	this->research_data.reset(
	    new DataChunk<ResearchData>(file, RESEARCH_DATA_OFFSET_START, RESEARCH_DATA_OFFSET_END));
	this->research_names.reset(
	    new StrTab(file, RESEARCH_NAME_STRTAB_OFFSET_START, RESEARCH_NAME_STRTAB_OFFSET_END, true));
	this->research_descriptions.reset(new StrTab(file, RESEARCH_DESCRIPTION_STRTAB_OFFSET_START,
	                                             RESEARCH_DESCRIPTION_STRTAB_OFFSET_END));
	this->ufopaedia_group.reset(
	    new StrTab(file, UFOPAEDIA_GROUP_STRTAB_OFFSET_START, UFOPAEDIA_GROUP_STRTAB_OFFSET_END));

	this->organisation_data.reset(new DataChunk<OrganisationData>(
	    file, ORGANISATION_DATA_OFFSET_START, ORGANISATION_DATA_OFFSET_END));
	this->organisation_raid_loot_data.reset(new DataChunk<OrgRaidLootData>(
	    file, ORGANISATION_RAID_LOOT_DATA_OFFSET_START, ORGANISATION_RAID_LOOT_DATA_OFFSET_END));
	this->organisation_starting_relationships_data.reset(
	    new DataChunk<OrgStartingRelationshipsData>(
	        file, ORGANISATION_STARTING_RELATIONSHIPS_DATA_OFFSET_START,
	        ORGANISATION_STARTING_RELATIONSHIPS_DATA_OFFSET_END));

	this->vehicle_data.reset(
	    new DataChunk<VehicleData>(file, VEHICLE_DATA_OFFSET_START, VEHICLE_DATA_OFFSET_END));
	this->vehicle_names.reset(
	    new StrTab(file, VEHICLE_NAME_STRTAB_OFFSET_START, VEHICLE_NAME_STRTAB_OFFSET_END));

	this->organisation_names.reset(new StrTab(file, ORGANISATION_NAME_STRTAB_OFFSET_START,
	                                          ORGANISATION_NAME_STRTAB_OFFSET_END));
	this->building_names.reset(
	    new StrTab(file, BUILDING_NAME_STRTAB_OFFSET_START, BUILDING_NAME_STRTAB_OFFSET_END));

	this->building_functions.reset(new StrTab(file, BUILDING_FUNCTION_STRTAB_OFFSET_START,
	                                          BUILDING_FUNCTION_STRTAB_OFFSET_END));

	this->alien_building_names.reset(new StrTab(file, ALIEN_BUILDING_NAME_STRTAB_OFFSET_START,
	                                            ALIEN_BUILDING_NAME_STRTAB_OFFSET_END));

	this->rawsound.reset(
	    new DataChunk<RawSoundData>(file, RAWSOUND_OFFSET_START, RAWSOUND_OFFSET_END));
	this->baselayouts.reset(
	    new DataChunk<BaseLayoutData>(file, BASELAYOUT_OFFSET_START, BASELAYOUT_OFFSET_END));

	this->agent_equipment_names.reset(
	    new StrTab(file, AGENT_EQUIPMENT_NAMES_OFFSET_START, AGENT_EQUIPMENT_NAMES_OFFSET_END));

	this->agent_type_names.reset(
	    new StrTab(file, AGENT_TYPE_NAMES_OFFSET_START, AGENT_TYPE_NAMES_OFFSET_END, true));

	this->agent_types.reset(new DataChunk<AgentTypeData>(file, AGENT_TYPE_DATA_OFFSET_START,
	                                                     AGENT_TYPE_DATA_OFFSET_END));

	this->vehicle_equipment_names.reset(
	    new StrTab(file, VEHICLE_EQUIPMENT_NAMES_OFFSET_START, VEHICLE_EQUIPMENT_NAMES_OFFSET_END));

	this->vehicle_equipment.reset(new DataChunk<VehicleEquipmentData>(
	    file, VEHICLE_EQUIPMENT_DATA_OFFSET_START, VEHICLE_EQUIPMENT_DATA_OFFSET_END));
	this->vehicle_weapons.reset(new DataChunk<VehicleWeaponData>(
	    file, VEHICLE_WEAPON_DATA_OFFSET_START, VEHICLE_WEAPON_DATA_OFFSET_END));
	this->vehicle_engines.reset(new DataChunk<VehicleEngineData>(
	    file, VEHICLE_ENGINE_DATA_OFFSET_START, VEHICLE_ENGINE_DATA_OFFSET_END));
	this->vehicle_general_equipment.reset(new DataChunk<VehicleGeneralEquipmentData>(
	    file, VEHICLE_GENERAL_EQUIPMENT_DATA_OFFSET_START,
	    VEHICLE_GENERAL_EQUIPMENT_DATA_OFFSET_END));

	this->vehicle_equipment_layouts.reset(new DataChunk<VehicleEquipmentLayout>(
	    file, VEHICLE_EQUIPMENT_LAYOUT_OFFSET_START, VEHICLE_EQUIPMENT_LAYOUT_OFFSET_END));

	this->facility_names.reset(
	    new StrTab(file, FACILITY_STRTAB_OFFSET_START, FACILITY_STRTAB_OFFSET_END));
	this->facility_data.reset(
	    new DataChunk<FacilityData>(file, FACILITY_DATA_OFFSET_START, FACILITY_DATA_OFFSET_END));

	this->economy_data1.reset(
	    new DataChunk<EconomyData>(file, ECONOMY_DATA1_OFFSET_START, ECONOMY_DATA1_OFFSET_END));
	this->economy_data2.reset(
	    new DataChunk<EconomyData>(file, ECONOMY_DATA2_OFFSET_START, ECONOMY_DATA2_OFFSET_END));
	this->economy_data3.reset(
	    new DataChunk<EconomyData>(file, ECONOMY_DATA3_OFFSET_START, ECONOMY_DATA3_OFFSET_END));

	this->scenery_minimap_colour.reset(new DataChunk<SceneryMinimapColour>(
	    file, SCENERY_MINIMAP_COLOUR_DATA_OFFSET_START, SCENERY_MINIMAP_COLOUR_DATA_OFFSET_END));

	this->bullet_sprites.reset(new DataChunk<BulletSprite>(
	    file, BULLETSPRITE_DATA_UFO2P_OFFSET_START, BULLETSPRITE_DATA_UFO2P_OFFSET_END));
	this->projectile_sprites.reset(new DataChunk<ProjectileSprites>(
	    file, PROJECTILESPRITES_DATA_UFO2P_OFFSET_START, PROJECTILESPRITES_DATA_UFO2P_OFFSET_END));

	this->crew_ufo_downed.reset(
	    new DataChunk<CrewData>(file, CREW_UFO_DOWNED_OFFSET_START, CREW_UFO_DOWNED_OFFSET_END));
	this->crew_ufo_deposit.reset(
	    new DataChunk<CrewData>(file, CREW_UFO_DEPOSIT_OFFSET_START, CREW_UFO_DEPOSIT_OFFSET_END));
	this->crew_alien_building.reset(new DataChunk<CrewData>(file, CREW_ALIEN_BUILDING_OFFSET_START,
	                                                        CREW_ALIEN_BUILDING_OFFSET_END));

	this->infiltration_speed_org.reset(
	    new DataChunk<OrgInfiltrationSpeed>(file, ORGANISATION_INFILTRATION_SPEED_OFFSET_START,
	                                        ORGANISATION_INFILTRATION_SPEED_OFFSET_END));
	this->vehicle_park.reset(
	    new DataChunk<OrgVehicleParkData>(file, ORGANISATION_VEHICLE_PARK_DATA_OFFSET_START,
	                                      ORGANISATION_VEHICLE_PARK_DATA_OFFSET_END));

	this->infiltration_speed_agent.reset(new DataChunk<AgentInfiltrationSpeed>(
	    file, AGENT_INFILTRATION_SPEED_OFFSET_START, AGENT_INFILTRATION_SPEED_OFFSET_END));
	this->infiltration_speed_building.reset(new DataChunk<BuildingInfiltrationSpeed>(
	    file, BUILDING_INFILTRATION_SPEED_OFFSET_START, BUILDING_INFILTRATION_SPEED_OFFSET_END));
	this->building_cost_data.reset(new DataChunk<BuildingCostData>(
	    file, BUILDING_COST_STRUCT_OFFSET_START, BUILDING_COST_STRUCT_OFFSET_END));
}

void UFO2P::fillCrew(GameState &state, CrewData crew,
                     std::map<OpenApoc::StateRef<OpenApoc::AgentType>, int> &target)
{
	if (crew.alien_egg > 0)
	{
		target[{&state, "AGENTTYPE_MULTIWORM_EGG"}] = crew.alien_egg;
	}
	if (crew.anthropod > 0)
	{
		target[{&state, "AGENTTYPE_ANTHROPOD"}] = crew.anthropod;
	}
	if (crew.brainsucker > 0)
	{
		target[{&state, "AGENTTYPE_BRAINSUCKER"}] = crew.brainsucker;
	}
	if (crew.crysalis > 0)
	{
		target[{&state, "AGENTTYPE_CHRYSALIS"}] = crew.crysalis;
	}
	if (crew.hyperworm > 0)
	{
		target[{&state, "AGENTTYPE_HYPERWORM"}] = crew.hyperworm;
	}
	if (crew.megaspawn > 0)
	{
		target[{&state, "AGENTTYPE_MEGASPAWN"}] = crew.megaspawn;
	}
	if (crew.micronoid > 0)
	{
		target[{&state, "AGENTTYPE_MICRONOID_AGGREGATE"}] = crew.micronoid;
	}
	if (crew.multiworm > 0)
	{
		target[{&state, "AGENTTYPE_MULTIWORM"}] = crew.multiworm;
	}
	if (crew.popper > 0)
	{
		target[{&state, "AGENTTYPE_POPPER"}] = crew.popper;
	}
	if (crew.psimorph > 0)
	{
		target[{&state, "AGENTTYPE_PSIMORPH"}] = crew.psimorph;
	}
	if (crew.queenspawn > 0)
	{
		target[{&state, "AGENTTYPE_QUEENSPAWN"}] = crew.queenspawn;
	}
	if (crew.skeletoid > 0)
	{
		target[{&state, "AGENTTYPE_SKELETOID"}] = crew.skeletoid;
	}
	if (crew.spitter > 0)
	{
		target[{&state, "AGENTTYPE_SPITTER"}] = crew.spitter;
	}
}

} // namespace OpenApoc

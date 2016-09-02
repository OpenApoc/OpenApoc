#include "ufo2p.h"
#include "crc32.h"
#include "framework/framework.h"

#include <iomanip>
#include <iterator>

namespace OpenApoc
{

/* This is the crc32 of the ufo2p.exe found on my steam version of apoc
 * It's likely there are other executables around with different CRCs, but we
 * need to make sure the offsets are the same, then we can add them to an
 * 'allowed' list, or have a map of 'known' CRCs with offsets of the various
 * tables */
uint32_t expected_ufo2p_crc32 = 0x1e7e11d6;

UFO2P::UFO2P(std::string file_name)
{
	auto file = fw().data->fs.open(file_name);

	if (!file)
	{
		LogError("Failed to open \"%s\"", file_name.c_str());
		exit(1);
	}

	std::istream_iterator<uint8_t> eof;
	std::istream_iterator<uint8_t> sof(file);

	auto crc32 = crc(sof, eof);

	if (crc32 != expected_ufo2p_crc32)
	{
		LogError("File \"%s\"\" has an unknown crc32 value of 0x%08lx - expected 0x%08x",
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

	this->scenery_minimap_colour.reset(new DataChunk<SceneryMinimapColour>(
	    file, SCENERY_MINIMAP_COLOUR_DATA_OFFSET_START, SCENERY_MINIMAP_COLOUR_DATA_OFFSET_END));

	this->bullet_sprites.reset(new DataChunk<BulletSprite>(file, BULLETSPRITE_DATA_OFFSET_START,
	                                                       BULLETSPRITE_DATA_OFFSET_END));
	this->projectile_sprites.reset(new DataChunk<ProjectileSprites>(
	    file, PROJECTILESPRITES_DATA_OFFSET_START, PROJECTILESPRITES_DATA_OFFSET_END));
}

} // namespace OpenApoc

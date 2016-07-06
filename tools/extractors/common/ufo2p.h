#pragma once

#include "game/state/organisation.h"
#include "game/state/rules/facility_type.h"
#include "game/state/rules/vehicle_type.h"
#include "game/state/rules/vequipment_type.h"
#include "tools/extractors/common/audio.h"
#include "tools/extractors/common/baselayout.h"
#include "tools/extractors/common/bulletsprite.h"
#include "tools/extractors/common/canonstring.h"
#include "tools/extractors/common/datachunk.h"
#include "tools/extractors/common/facilities.h"
#include "tools/extractors/common/organisations.h"
#include "tools/extractors/common/research.h"
#include "tools/extractors/common/scenerytile.h"
#include "tools/extractors/common/strtab.h"
#include "tools/extractors/common/ufopaedia.h"
#include "tools/extractors/common/vehicle.h"
#include "tools/extractors/common/vequipment.h"
#include <algorithm>
#include <memory>
#include <string>

#define BUILDING_NAME_STRTAB_OFFSET_START 1351122
#define BUILDING_NAME_STRTAB_OFFSET_END 1353322

#define ALIEN_BUILDING_NAME_STRTAB_OFFSET_START 1355043
#define ALIEN_BUILDING_NAME_STRTAB_OFFSET_END 1355211

namespace OpenApoc
{

class UFO2P
{
  public:
	UFO2P(std::string fileName = "XCOM3/UFOEXE/UFO2P.EXE");
	std::unique_ptr<StrTab> research_names;
	std::unique_ptr<StrTab> research_descriptions;
	std::unique_ptr<DataChunk<research_data_t>> research_data;

	std::unique_ptr<StrTab> vehicle_names;
	std::unique_ptr<StrTab> organisation_names;
	std::unique_ptr<StrTab> building_names;

	std::unique_ptr<StrTab> alien_building_names;

	std::unique_ptr<DataChunk<vehicle_data_t>> vehicle_data;

	std::unique_ptr<StrTab> ufopaedia_group;

	std::unique_ptr<DataChunk<rawsound_data_t>> rawsound;
	std::unique_ptr<DataChunk<baselayout_data_t>> baselayouts;

	std::unique_ptr<StrTab> vehicle_equipment_names;

	std::unique_ptr<DataChunk<vehicle_equipment_data_t>> vehicle_equipment;
	std::unique_ptr<DataChunk<vehicle_weapon_data_t>> vehicle_weapons;
	std::unique_ptr<DataChunk<vehicle_engine_data_t>> vehicle_engines;
	std::unique_ptr<DataChunk<vehicle_general_equipment_data_t>> vehicle_general_equipment;

	std::unique_ptr<DataChunk<vehicle_equipment_layout_t>> vehicle_equipment_layouts;

	std::unique_ptr<StrTab> facility_names;
	std::unique_ptr<DataChunk<facility_data_t>> facility_data;

	std::unique_ptr<DataChunk<scenery_minimap_colour_t>> scenery_minimap_colour;

	std::unique_ptr<DataChunk<bullet_sprite_t>> bullet_sprites;
	std::unique_ptr<DataChunk<projectile_sprites_t>> projectile_sprites;

	UString get_org_id(int idx)
	{
		return Organisation::getPrefix() + canon_string(this->organisation_names->get(idx));
	}
	UString get_facility_id(int idx)
	{
		return FacilityType::getPrefix() + canon_string(this->facility_names->get(idx));
	}
	UString get_vequipment_id(int idx)
	{
		return VEquipmentType::getPrefix() + canon_string(this->vehicle_equipment_names->get(idx));
	}
	UString get_vehicle_id(int idx)
	{
		return VehicleType::getPrefix() + canon_string(this->vehicle_names->get(idx));
	}
};

UFO2P &getUFO2PData();

static inline std::string toLower(std::string input)
{
	std::transform(input.begin(), input.end(), input.begin(), ::tolower);
	return input;
}
} // namespace OpenApoc

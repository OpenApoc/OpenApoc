#pragma once

#include "audio.h"
#include "baselayout.h"
#include "canonstring.h"
#include "datachunk.h"
#include "facilities.h"
#include "organisations.h"
#include "research.h"
#include "strtab.h"
#include "ufopaedia.h"
#include "vehicle.h"
#include "vequipment.h"

#include "game/organisation.h"
#include "game/rules/facility_type.h"
#include "game/rules/vehicle_type.h"
#include "game/rules/vequipment.h"

#include <algorithm>
#include <memory>
#include <string>

#define BUILDING_NAME_STRTAB_OFFSET_START 1351122
#define BUILDING_NAME_STRTAB_OFFSET_END 1353322

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

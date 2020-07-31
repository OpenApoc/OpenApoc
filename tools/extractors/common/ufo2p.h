#pragma once

#include "game/state/rules/city/facilitytype.h"
#include "game/state/rules/city/vammotype.h"
#include "game/state/rules/city/vehicletype.h"
#include "game/state/rules/city/vequipmenttype.h"
#include "game/state/shared/organisation.h"
#include "tools/extractors/common/aequipment.h"
#include "tools/extractors/common/agent.h"
#include "tools/extractors/common/audio.h"
#include "tools/extractors/common/baselayout.h"
#include "tools/extractors/common/building.h"
#include "tools/extractors/common/bulletsprite.h"
#include "tools/extractors/common/canonstring.h"
#include "tools/extractors/common/crew.h"
#include "tools/extractors/common/datachunk.h"
#include "tools/extractors/common/economy.h"
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

namespace OpenApoc
{

class AgentType;

class UFO2P
{
  public:
	UFO2P(std::string fileName = "xcom3/ufoexe/ufo2p.exe");
	std::unique_ptr<StrTab> research_names;
	std::unique_ptr<StrTab> research_descriptions;
	std::unique_ptr<DataChunk<ResearchData>> research_data;

	std::unique_ptr<StrTab> vehicle_names;
	std::unique_ptr<StrTab> organisation_names;
	std::unique_ptr<StrTab> building_names;
	std::unique_ptr<StrTab> building_functions;
	std::unique_ptr<StrTab> alien_building_names;

	std::unique_ptr<DataChunk<VehicleData>> vehicle_data;

	std::unique_ptr<DataChunk<OrganisationData>> organisation_data;
	std::unique_ptr<DataChunk<OrgRaidLootData>> organisation_raid_loot_data;
	std::unique_ptr<DataChunk<OrgStartingRelationshipsData>>
	    organisation_starting_relationships_data;
	std::unique_ptr<DataChunk<OrgVehicleParkData>> vehicle_park;

	std::unique_ptr<StrTab> ufopaedia_group;

	std::unique_ptr<DataChunk<RawSoundData>> rawsound;
	std::unique_ptr<DataChunk<BaseLayoutData>> baselayouts;

	// Extracted from UFO2P.EXE and not TACP.EXE because here they are better formatted
	std::unique_ptr<StrTab> agent_equipment_names;

	std::unique_ptr<StrTab> agent_type_names;

	std::unique_ptr<DataChunk<AgentTypeData>> agent_types;

	std::unique_ptr<StrTab> vehicle_equipment_names;

	std::unique_ptr<DataChunk<VehicleEquipmentData>> vehicle_equipment;
	std::unique_ptr<DataChunk<VehicleWeaponData>> vehicle_weapons;
	std::unique_ptr<DataChunk<VehicleEngineData>> vehicle_engines;
	std::unique_ptr<DataChunk<VehicleGeneralEquipmentData>> vehicle_general_equipment;

	std::unique_ptr<DataChunk<VehicleEquipmentLayout>> vehicle_equipment_layouts;

	std::unique_ptr<StrTab> facility_names;
	std::unique_ptr<DataChunk<FacilityData>> facility_data;

	std::unique_ptr<DataChunk<EconomyData>> economy_data1;
	std::unique_ptr<DataChunk<EconomyData>> economy_data2;
	std::unique_ptr<DataChunk<EconomyData>> economy_data3;

	std::unique_ptr<DataChunk<SceneryMinimapColour>> scenery_minimap_colour;

	std::unique_ptr<DataChunk<BulletSprite>> bullet_sprites;
	std::unique_ptr<DataChunk<ProjectileSprites>> projectile_sprites;

	std::unique_ptr<DataChunk<CrewData>> crew_ufo_downed;
	std::unique_ptr<DataChunk<CrewData>> crew_ufo_deposit;
	std::unique_ptr<DataChunk<CrewData>> crew_alien_building;

	std::unique_ptr<DataChunk<BuildingCostData>> building_cost_data;
	std::unique_ptr<DataChunk<OrgInfiltrationSpeed>> infiltration_speed_org;
	std::unique_ptr<DataChunk<AgentInfiltrationSpeed>> infiltration_speed_agent;
	std::unique_ptr<DataChunk<BuildingInfiltrationSpeed>> infiltration_speed_building;

	UString getOrgId(int idx) const
	{
		return Organisation::getPrefix() + canon_string(this->organisation_names->get(idx));
	}
	UString getFacilityId(int idx) const
	{
		return FacilityType::getPrefix() + canon_string(this->facility_names->get(idx));
	}
	UString getVequipmentId(int idx) const
	{
		return VEquipmentType::getPrefix() + canon_string(this->vehicle_equipment_names->get(idx));
	}
	UString getVehicleId(int idx) const
	{
		return VehicleType::getPrefix() + canon_string(this->vehicle_names->get(idx));
	}

	static void fillCrew(GameState &state, CrewData crew,
	                     std::map<OpenApoc::StateRef<OpenApoc::AgentType>, int> &target);
};

UFO2P &getUFO2PData();

static inline std::string toLower(std::string input)
{
	std::transform(input.begin(), input.end(), input.begin(), ::tolower);
	return input;
}
} // namespace OpenApoc

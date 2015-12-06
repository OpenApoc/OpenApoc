#pragma once

#include "game/rules/vehicle_type.h"
#include "game/rules/buildingdef.h"
#include "game/rules/scenerytiledef.h"
#include "game/rules/vequipment.h"
#include "game/rules/facilitydef.h"
#include "game/rules/doodaddef.h"
#include "game/rules/resource_aliases.h"

#include "game/organisation.h"

#include "framework/logger.h"
#include "library/vec.h"
#include "library/sp.h"

#include <vector>
#include <map>

namespace OpenApoc
{

class Framework;
class UString;

class Rules
{
  private:
	std::map<UString, up<VehicleType>> vehicle_types;
	std::vector<BuildingDef> buildings;
	std::vector<Organisation> organisations;
	std::map<UString, SceneryTileDef> buildingTiles;
	std::map<UString, FacilityDef> facilities;
	std::map<UString, DoodadDef> doodads;
	std::map<UString, up<VEquipmentType>> vehicle_equipment;
	std::vector<UString> landingPadTiles;
	Vec3<int> citySize;
	std::vector<UString> tileIDs;
	std::shared_ptr<ResourceAliases> aliases;
	friend class RulesLoader;

  public:
	Rules(Framework &fw, const UString &rootFileName);

	const std::map<UString, up<VehicleType>> &getVehicleTypes() const { return vehicle_types; }

	const std::vector<BuildingDef> &getBuildingDefs() const { return buildings; }

	const std::vector<UString> &getLandingPadTiles() const { return landingPadTiles; }

	const std::vector<Organisation> &getOrganisations() const { return organisations; }

	const std::map<UString, FacilityDef> &getFacilityDefs() const { return facilities; }

	const std::map<UString, DoodadDef> &getDoodadDefs() const { return doodads; }

	const std::map<UString, up<VEquipmentType>> &getVehicleEquipmentTypes() const
	{
		return vehicle_equipment;
	}

	const DoodadDef &getDoodadDef(const UString &id) const
	{
		auto pair = doodads.find(id);
		if (pair != doodads.end())
		{
			return pair->second;
		}
		else
		{
			LogError("No doodads tile found with ID \"%s\"", id.c_str());
			// return _something_
			return doodads.begin()->second;
		}
	}

	const VEquipmentType &getVEquipmentType(const UString &id) const
	{
		auto pair = vehicle_equipment.find(id);
		if (pair != vehicle_equipment.end())
		{
			return *pair->second;
		}
		else
		{
			LogError("No vehicle_equipment found with ID \"%s\"", id.c_str());
			// return _something_
			return *vehicle_equipment.begin()->second;
		}
	}

	const SceneryTileDef &getSceneryTileDef(const UString &id) const
	{
		auto pair = buildingTiles.find(id);
		if (pair != buildingTiles.end())
		{
			return pair->second;
		}
		else
		{
			LogError("No building tile found with ID \"%s\"", id.c_str());
			return buildingTiles.find("0")->second;
		}
	}

	Vec3<int> &getCitySize() { return this->citySize; }

	const UString &getSceneryTileAt(Vec3<int> offset) const;
};

}; // namespace OpenApoc

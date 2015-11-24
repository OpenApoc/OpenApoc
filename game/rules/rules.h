#pragma once

#include "game/rules/vehicledef.h"
#include "game/rules/buildingdef.h"
#include "game/rules/scenerytiledef.h"
#include "game/rules/weapondef.h"
#include "game/rules/facilitydef.h"
#include "game/rules/doodaddef.h"
#include "game/rules/resource_aliases.h"

#include "game/organisation.h"

#include "framework/logger.h"
#include "library/vec.h"

#include <vector>
#include <map>

namespace OpenApoc
{

class Framework;
class UString;

class Rules
{
  private:
	std::map<UString, VehicleDefinition> vehicleDefs;
	std::vector<BuildingDef> buildings;
	std::vector<Organisation> organisations;
	std::map<UString, SceneryTileDef> buildingTiles;
	std::map<UString, WeaponDef> weapons;
	std::map<UString, FacilityDef> facilities;
	std::map<UString, DoodadDef> doodads;
	std::vector<UString> landingPadTiles;
	Vec3<int> citySize;
	std::vector<UString> tileIDs;
	std::shared_ptr<ResourceAliases> aliases;
	friend class RulesLoader;

  public:
	Rules(Framework &fw, const UString &rootFileName);

	std::map<UString, VehicleDefinition> &getVehicleDefs() { return vehicleDefs; }

	std::map<UString, WeaponDef> &getWeaponDefs() { return weapons; }

	std::vector<BuildingDef> &getBuildingDefs() { return buildings; }

	std::vector<UString> &getLandingPadTiles() { return landingPadTiles; }

	std::vector<Organisation> &getOrganisations() { return organisations; }

	std::map<UString, FacilityDef> &getFacilityDefs() { return facilities; }

	std::map<UString, DoodadDef> &getDoodadDefs() { return doodads; }

	DoodadDef &getDoodadDef(const UString &id)
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

	SceneryTileDef &getSceneryTileDef(const UString &id)
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

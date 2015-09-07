#pragma once

#include "game/rules/vehicledef.h"
#include "game/rules/organisationdef.h"
#include "game/rules/buildingdef.h"
#include "game/rules/buildingtiledef.h"
#include "game/rules/weapondef.h"

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
	std::vector<OrganisationDef> organisations;
	std::map<UString, BuildingTileDef> buildingTiles;
	std::map<UString, WeaponDef> weapons;
	std::vector<UString> landingPadTiles;
	Vec3<int> citySize;
	std::vector<UString> tileIDs;
	friend class RulesLoader;

  public:
	Rules(Framework &fw, const UString &rootFileName);

	std::map<UString, VehicleDefinition> &getVehicleDefs() { return vehicleDefs; }

	std::map<UString, WeaponDef> &getWeaponDefs() { return weapons; }

	std::vector<BuildingDef> &getBuildingDefs() { return buildings; }

	std::vector<UString> &getLandingPadTiles() { return landingPadTiles; }

	std::vector<OrganisationDef> &getOrganisationDefs() { return organisations; }

	BuildingTileDef &getBuildingTileDef(const UString &id)
	{
		auto pair = buildingTiles.find(id);
		if (pair != buildingTiles.end()) {
			return pair->second;
		} else {
			LogError("No building tile found with ID \"%s\"", id.str().c_str());
			return buildingTiles.find("0")->second;
		}
	}

	Vec3<int> &getCitySize() { return this->citySize; }

	const UString &getBuildingTileAt(Vec3<int> offset)
	{
		static const UString noTile = "";
		if (offset.x < 0 || offset.x >= citySize.x || offset.y < 0 || offset.y >= citySize.y ||
		    offset.z < 0 || offset.z >= citySize.z) {
			LogError("Trying to get tile {%d,%d,%d} in city of size {%d,%d,%d}", offset.x, offset.y,
			         offset.z, citySize.x, citySize.y, citySize.z);
			return noTile;
		}
		unsigned index = offset.z * citySize.x * citySize.y + offset.y * citySize.x + offset.x;
		if (index >= tileIDs.size()) {
			LogError("Tile {%d,%d,%d} would go over ID array! (city size {%d,%d,%d}, id offset %u, "
			         "id array size %u",
			         offset.x, offset.y, offset.z, citySize.x, citySize.y, citySize.z, index,
			         tileIDs.size());
			return noTile;
		}
		return tileIDs[index];
	}
};

} // namespace OpenApoc

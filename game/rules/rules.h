#pragma once

#include "game/rules/vehicledef.h"
#include "game/rules/organisationdef.h"
#include "game/rules/buildingdef.h"
#include "game/rules/buildingtiledef.h"

#include "framework/logger.h"
#include "library/vec.h"

#include <vector>
#include <map>

namespace OpenApoc {

class Framework;
class UString;

class Rules
{
private:
	std::map<UString, VehicleDefinition> vehicleDefs;
	std::vector<BuildingDef> buildings;
	std::vector<OrganisationDef> organisations;
	std::vector<BuildingTileDef> buildingTiles;
	Vec3<int> citySize;
	std::vector<int> tileIndices;
	friend class RulesLoader;
public:

	Rules(Framework &fw, const UString &rootFileName);

	std::map<UString, VehicleDefinition> &getVehicleDefs() 
	{
		return vehicleDefs;
	}

	std::vector<BuildingDef> &getBuildingDefs()
	{
		return buildings;
	}

	std::vector<OrganisationDef> &getOrganisationDefs()
	{
		return organisations;
	}

	std::vector<BuildingTileDef> &getBuildingTileDefs()
	{
		return buildingTiles;
	}

	Vec3<int> &getCitySize()
	{
		return this->citySize;
	}
	
	int getBuildingTileAt(Vec3<int> offset)
	{
		if (offset.x < 0 || offset.x >= citySize.x
		 || offset.y < 0 || offset.y >= citySize.y
		 || offset.z < 0 || offset.z >= citySize.z)
		{
			LogError("Trying to get tile {%d,%d,%d} in city of size {%d,%d,%d}",
				offset.x, offset.y, offset.z, citySize.x, citySize.y, citySize.z);
			return 0;
		}
		unsigned index = offset.z * citySize.x * citySize.y + offset.y * citySize.x + offset.x;
		assert(index < tileIndices.size());
		return tileIndices[index];
	}
};

}; //namespace OpenApoc

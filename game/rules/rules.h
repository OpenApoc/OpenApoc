#pragma once

#include "game/rules/vehicledef.h"

#include <map>

namespace OpenApoc {

class Framework;
class UString;

class Rules
{
private:
	std::map<UString, const VehicleDefinition> vehicleDefs;
	friend class RulesLoader;
public:

	Rules(Framework &fw, const UString &rootFileName);

	const std::map<UString, const VehicleDefinition> &getVehicleDefs() const
	{
		return vehicleDefs;
	}
};

}; //namespace OpenApoc

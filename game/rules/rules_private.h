#pragma once

#include "game/rules/rules.h"
#include "framework/trace.h"
#include <tinyxml2.h>

namespace OpenApoc
{

class RulesLoader
{
  public:
	static bool ParseRules(Framework &fw, Rules &rules, tinyxml2::XMLElement *root);

	static bool ParseVehicleType(Framework &fw, Rules &rules, tinyxml2::XMLElement *root);
	static bool ParseOrganisationDefinition(Framework &fw, Rules &rules,
	                                        tinyxml2::XMLElement *root);
	static bool ParseCityDefinition(Framework &fw, Rules &rules, tinyxml2::XMLElement *root);
	static bool ParseVehicleEquipment(Framework &fw, Rules &rules, tinyxml2::XMLElement *root);
	static bool ParseFacilityDefinition(Framework &fw, Rules &rules, tinyxml2::XMLElement *root);
	static bool ParseDoodadDefinition(Framework &fw, Rules &rules, tinyxml2::XMLElement *root);
	static bool ParseAliases(Framework &fw, Rules &rules, tinyxml2::XMLElement *root);

	// These check the validity of the ID string (Useful when creating new objects)
	static bool isValidEquipmentID(const UString &str);
	static bool isValidOrganisationID(const UString &str);
	static bool isValidStringID(const UString &str);
	static bool isValidVehicleTypeID(const UString &str);

	// These check an object actually exists with that ID (Useful when referencing)
	static bool isValidOrganisation(const Rules &rules, const UString &str);
};

}; // namespace OpenApoc

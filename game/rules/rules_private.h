#pragma once

#include "game/rules/rules.h"
#include <tinyxml2.h>

namespace OpenApoc
{

class RulesLoader
{
  public:
	static bool ParseRules(Rules &rules, tinyxml2::XMLElement *root);

	static bool ParseAliases(Rules &rules, tinyxml2::XMLElement *root);

	// These check the validity of the ID string (Useful when creating new objects)
	static bool isValidEquipmentID(const UString &str);
	static bool isValidOrganisationID(const UString &str);
	static bool isValidStringID(const UString &str);
	static bool isValidVehicleTypeID(const UString &str);

	// These check an object actually exists with that ID (Useful when referencing)
	static bool isValidOrganisation(const Rules &rules, const UString &str);
};

}; // namespace OpenApoc

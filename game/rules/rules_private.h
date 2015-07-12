#pragma once

#include "game/rules/rules.h"
#include <tinyxml2.h>

namespace OpenApoc
{

class RulesLoader
{
public:
	static bool ParseRules(Framework &fw, Rules& rules, tinyxml2::XMLElement *root);


	static bool ParseVehicleDefinition(Framework &fw, Rules &rules, tinyxml2::XMLElement *root);

};

}; //namespace OpenApoc

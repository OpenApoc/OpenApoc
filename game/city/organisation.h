#pragma once

#include "framework/includes.h"

namespace OpenApoc {

class Organisation
{
public:
	std::string name;
	Organisation(std::string name);
	static std::vector<Organisation> defaultOrganisations;
};

}; //namespace OpenApoc

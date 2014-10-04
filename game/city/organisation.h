#pragma once

#include "framework/includes.h"

class Organisation
{
public:
	std::string name;
	Organisation(std::string name);
	static std::vector<Organisation> defaultOrganisations;
};

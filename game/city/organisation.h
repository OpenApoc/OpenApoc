#pragma once

#include <string>
#include <vector>

class Organisation
{
public:
	std::string name;
	Organisation(std::string name);
	static std::vector<Organisation> defaultOrganisations;
};

#pragma once

#include "framework/includes.h"

namespace OpenApoc {

class Organisation
{
public:
	UString name;
	Organisation(UString name);
	static std::vector<Organisation> defaultOrganisations;
};

}; //namespace OpenApoc

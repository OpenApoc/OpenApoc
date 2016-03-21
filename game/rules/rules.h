#pragma once

#include "game/rules/vehicle_type.h"
#include "game/rules/resource_aliases.h"

#include "game/organisation.h"

#include "framework/logger.h"
#include "library/vec.h"
#include "library/sp.h"

#include <vector>
#include <map>

namespace OpenApoc
{

class UString;

class Rules
{
  public:
	sp<ResourceAliases> aliases;

	Rules();
	Rules(const UString &rootFileName);

	bool isValid();
};

}; // namespace OpenApoc

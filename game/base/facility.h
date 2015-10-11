#pragma once

#include "game/rules/facilitydef.h"
#include "library/vec.h"

namespace OpenApoc
{

class Facility
{
  public:
	const FacilityDef &def;
	Vec2<int> pos;
	int buildTime;

	Facility(const FacilityDef &def);
};

}; // namespace OpenApoc

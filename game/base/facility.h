#pragma once

#include "game/rules/facility_type.h"
#include "library/vec.h"

namespace OpenApoc
{

class Facility
{
  public:
	Facility(StateRef<FacilityType> type);
	Facility() = default;
	StateRef<FacilityType> type;
	Vec2<int> pos;
	int buildTime;
};

}; // namespace OpenApoc

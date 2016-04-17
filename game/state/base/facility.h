#pragma once

#include "game/state/rules/facility_type.h"
#include "library/vec.h"

namespace OpenApoc
{

class Agent;
class Lab;

class Facility
{
  public:
	Facility(StateRef<FacilityType> type);
	Facility() = default;
	StateRef<FacilityType> type;
	Vec2<int> pos;
	int buildTime;

	StateRef<Lab> lab;
};

}; // namespace OpenApoc

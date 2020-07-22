#pragma once

#include "game/state/stateobject.h"
#include "library/vec.h"

namespace OpenApoc
{

class Agent;
class Lab;
class FacilityType;

class Facility
{
  public:
	Facility(StateRef<FacilityType> type);
	Facility() = default;
	StateRef<FacilityType> type;
	Vec2<int> pos = {0, 0};
	int buildTime = 0;

	StateRef<Lab> lab;
};

}; // namespace OpenApoc

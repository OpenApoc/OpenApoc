#pragma once

#include "game/state/rules/facility_type.h"
#include "library/vec.h"

namespace OpenApoc
{

class Agent;

class Facility
{
  public:
	Facility(StateRef<FacilityType> type);
	Facility() = default;
	StateRef<FacilityType> type;
	Vec2<int> pos;
	int buildTime;

	std::list<StateRef<Agent>> assigned_agents;
};

}; // namespace OpenApoc

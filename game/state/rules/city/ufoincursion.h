#pragma once

#include "game/state/stateobject.h"
#include "library/strings.h"
#include <map>
#include <utility>
#include <vector>

namespace OpenApoc
{

class UFOIncursion : public StateObject<UFOIncursion>
{
  public:
	enum class PrimaryMission
	{
		Infiltration,
		Subversion,
		Attack,
		Overspawn
	};

	PrimaryMission primaryMission = PrimaryMission::Infiltration;
	std::vector<std::pair<UString, int>> primaryList;
	std::vector<std::pair<UString, int>> escortList;
	std::vector<std::pair<UString, int>> attackList;
	int priority = 0;
};

}; // namespace OpenApoc

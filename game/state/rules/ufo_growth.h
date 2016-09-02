#pragma once
#include "game/state/stateobject.h"
#include "library/strings.h"

namespace OpenApoc
{

class UFOGrowth : public StateObject<UFOGrowth>
{
  public:
	int week = 0;
	std::vector<std::pair<UString, int>> vehicleTypeList;
};
}; // namespace OpenApoc

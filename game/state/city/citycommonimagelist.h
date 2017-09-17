#pragma once

#include "game/state/stateobject.h"
#include "library/sp.h"
#include <vector>

namespace OpenApoc
{
class Image;

class CityCommonImageList
{
  public:
	sp<std::vector<sp<Image>>> strategyImages;
};
}

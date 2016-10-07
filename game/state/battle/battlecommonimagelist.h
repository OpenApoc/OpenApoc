#pragma once

#include "library/sp.h"
#include <vector>

namespace OpenApoc
{
class Image;

class BattleCommonImageList
{
  public:
	sp<std::vector<sp<Image>>> strategyImages;
	sp<Image> loadingImage;
	std::vector<sp<Image>> focusArrows;
};
}

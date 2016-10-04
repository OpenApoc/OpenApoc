#pragma once
#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/strings.h"
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
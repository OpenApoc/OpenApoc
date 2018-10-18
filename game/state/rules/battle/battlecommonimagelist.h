#pragma once

#include "game/state/stateobject.h"
#include "library/sp.h"
#include <vector>

namespace OpenApoc
{
class Image;
class DoodadType;

class BattleCommonImageList
{
  public:
	sp<std::vector<sp<Image>>> strategyImages;
	sp<Image> loadingImage;
	std::vector<sp<Image>> focusArrows;
	StateRef<DoodadType> burningDoodad;
};
} // namespace OpenApoc

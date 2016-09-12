#pragma once
#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/strings.h"
#include <vector>

namespace OpenApoc
{
class Image;
class BattleUnitImagePack : public StateObject<BattleUnitImagePack>
{
  public:
	std::vector<sp<Image>> images;
};
}

#pragma once
#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/strings.h"
#include "library/vec.h"
#include <vector>

namespace OpenApoc
{

class Image;

class DoodadFrame
{
  public:
	sp<Image> image;
	int time = 0;
};

class DoodadType : public StateObject<DoodadType>
{
  public:
	DoodadType() = default;
	int lifetime = 0;
	bool repeatable = false;
	Vec2<int> imageOffset = {0, 0};
	std::vector<DoodadFrame> frames;
};

} // namespace OpenApoc

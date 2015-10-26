#pragma once
#include "library/sp.h"
#include "library/strings.h"
#include <vector>
#include <map>

namespace OpenApoc
{

class Image;

class DoodadFrame
{
  public:
	sp<Image> image;
	int time;
};

class DoodadDef
{
  private:
	DoodadDef(){};
	friend class RulesLoader;

  public:
	UString ID;
	int lifetime;
	Vec2<int> imageOffset;
	std::vector<DoodadFrame> frames;
};

} // namespace OpenApoc

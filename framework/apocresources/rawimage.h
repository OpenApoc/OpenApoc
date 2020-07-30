#pragma once

#include "library/sp.h"
#include "library/strings.h"
#include "library/vec.h"

namespace OpenApoc
{

class Data;
class PaletteImage;
class ImageSet;

class RawImage
{
  public:
	static sp<PaletteImage> load(Data &data, const UString &fileName, const Vec2<int> &size);
	static sp<ImageSet> loadSet(Data &data, const UString &fileName, const Vec2<int> &size);
};

}; // namespace OpenApoc

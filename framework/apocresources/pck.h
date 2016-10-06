#pragma once

#include "library/sp.h"

namespace OpenApoc
{

class Data;
class ImageSet;
class UString;

class PCKLoader
{
  public:
	static sp<ImageSet> load(Data &data, UString PckFilename, UString TabFilename);
	static sp<ImageSet> loadStrat(Data &data, UString PckFilename, UString TabFilename);
	static sp<ImageSet> loadShadow(Data &data, UString PckFilename, UString TabFilename,
	                               unsigned shadedIdx = 244);
};

}; // namespace OpenApoc

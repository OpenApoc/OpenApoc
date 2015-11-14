
#pragma once
#include "library/sp.h"

#include "framework/includes.h"

namespace OpenApoc
{

class Data;
class ImageSet;

class PCKLoader
{
  public:
	static sp<ImageSet> load(Data &data, UString PckFilename, UString TabFilename);
	static sp<ImageSet> load_strat(Data &data, UString PckFilename, UString TabFilename);
	static sp<ImageSet> load_shadow(Data &data, UString PckFilename, UString TabFilename,
	                                uint8_t shadedIdx = 244);
};

}; // namespace OpenApoc

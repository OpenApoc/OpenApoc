
#pragma once

#include "framework/includes.h"

namespace OpenApoc
{

class Data;
class ImageSet;

class PCKLoader
{
  public:
	static std::shared_ptr<ImageSet> load(Data &data, UString PckFilename, UString TabFilename);
};

} // namespace OpenApoc

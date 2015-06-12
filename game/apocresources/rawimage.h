#pragma once

#include "library/vec.h"
#include <memory>

namespace OpenApoc
{

class Data;
class UString;
class PaletteImage;

class RawImage
{
public:
	static std::shared_ptr<PaletteImage> load(Data& data, const UString &fileName, const Vec2<int> &size);
};

}; //namespace OpenApoc

#pragma once

#include "library/sp.h"
#include "library/strings.h"

namespace pugi
{
class xml_node;
} // namespace pugi

namespace OpenApoc
{

class BitmapFont;

class ApocalypseFont
{
  public:
	static sp<BitmapFont> loadFont(const UString &fontDescPath);
};
}; // namespace OpenApoc

#pragma once

#include "library/sp.h"

namespace pugi
{
class xml_node;
} // namespace pugi

namespace OpenApoc
{

class UString;
class BitmapFont;

class ApocalypseFont
{
  public:
	static sp<BitmapFont> loadFont(const UString &fontDescPath);
};
}; // namespace OpenApoc

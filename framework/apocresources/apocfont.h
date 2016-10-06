#pragma once

#include "library/sp.h"

namespace tinyxml2
{
class XMLElement;
} // namespace tinyxml2

namespace OpenApoc
{

class BitmapFont;

class ApocalypseFont
{
  public:
	static sp<BitmapFont> loadFont(tinyxml2::XMLElement *fontElement);
};
}; // namespace OpenApoc

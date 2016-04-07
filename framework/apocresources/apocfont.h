#pragma once
#include "framework/font.h"
#include "library/sp.h"

namespace tinyxml2
{
class XMLElement;
} // namespace tinyxml2

namespace OpenApoc
{

class Image;
class Renderer;

class ApocalypseFont
{
  public:
	static sp<BitmapFont> loadFont(tinyxml2::XMLElement *fontElement);
};
}; // namespace OpenApoc

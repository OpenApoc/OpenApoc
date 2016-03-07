
#pragma once
#include "library/sp.h"

#include "framework/includes.h"
#include "framework/font.h"

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

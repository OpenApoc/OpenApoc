
#pragma once
#include "library/sp.h"

#include "framework/includes.h"
#include "framework/font.h"

namespace OpenApoc
{

class Framework;
class Image;
class Renderer;

class ApocalypseFont : public BitmapFont
{

  private:
	ApocalypseFont() {}
	std::map<UniChar, sp<PaletteImage>> fontbitmaps;
	int spacewidth;
	int fontheight;
	UString name;
	int averagecharacterwidth;
	sp<Palette> palette;

  public:
	static sp<ApocalypseFont> loadFont(Framework &fw, tinyxml2::XMLElement *fontElement);
	virtual ~ApocalypseFont();
	virtual sp<PaletteImage> getGlyph(UniChar codepoint) override;
	virtual int GetFontHeight() override;
	virtual UString getName() override;
	virtual int GetEstimateCharacters(int FitInWidth) override;
	virtual sp<Palette> getPalette();
};
}; // namespace OpenApoc

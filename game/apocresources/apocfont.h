
#pragma once

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
	std::map<UniChar, std::shared_ptr<PaletteImage>> fontbitmaps;
	int spacewidth;
	int fontheight;
	UString name;
	int averagecharacterwidth;
	std::shared_ptr<Palette> palette;

  public:
	static std::shared_ptr<ApocalypseFont> loadFont(Framework &fw,
	                                                tinyxml2::XMLElement *fontElement);
	virtual ~ApocalypseFont();
	virtual std::shared_ptr<PaletteImage> getGlyph(UniChar codepoint) override;
	virtual int GetFontHeight() override;
	virtual UString getName() override;
	virtual int GetEstimateCharacters(int FitInWidth) override;
	virtual std::shared_ptr<Palette> getPalette();
};
}; // namespace OpenApoc

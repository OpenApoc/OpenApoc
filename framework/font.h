
#pragma once
#include "library/sp.h"

#include "framework/includes.h"
#include "library/strings.h"

#define APOCFONT_ALIGN_LEFT 0
#define APOCFONT_ALIGN_CENTRE 1
#define APOCFONT_ALIGN_RIGHT 2

namespace OpenApoc
{

class PaletteImage;
class Palette;

class BitmapFont
{
  protected:
	int spacewidth;
	int fontheight;
	int averagecharacterwidth;
	std::map<UniChar, sp<PaletteImage>> fontbitmaps;
	UString name;
	sp<Palette> palette;

  public:
	virtual ~BitmapFont();
	virtual sp<PaletteImage> getGlyph(UniChar codepoint);
	virtual sp<PaletteImage> getString(const UString &Text);
	virtual int GetFontHeight();
	virtual int GetFontWidth(const UString &Text);
	virtual UString getName();
	virtual int GetEstimateCharacters(int FitInWidth);
	virtual sp<Palette> getPalette();

	/* Reads in set of "Character":"glyph description string" pairs */
	static sp<BitmapFont> loadFont(const std::map<UniChar, UString> &charMap, int spaceWidth,
	                               int fontHeight, UString fontName, sp<Palette> defaultPalette);
};

}; // namespace OpenApoc

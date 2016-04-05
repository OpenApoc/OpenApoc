
#pragma once
#include "library/sp.h"

#include "framework/includes.h"
#include "library/strings.h"

#include <list>

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
	virtual int GetFontWidth(const UString &Text);
	virtual int GetFontHeight() const;
	virtual int GetFontHeight(const UString &Text, int MaxWidth);
	virtual UString getName() const;
	virtual int GetEstimateCharacters(int FitInWidth) const;
	virtual sp<Palette> getPalette() const;
	std::list<UString> WordWrapText(const UString &Text, int MaxWidth);

	/* Reads in set of "Character":"glyph description string" pairs */
	static sp<BitmapFont> loadFont(const std::map<UniChar, UString> &charMap, int spaceWidth,
	                               int fontHeight, UString fontName, sp<Palette> defaultPalette);
};

}; // namespace OpenApoc

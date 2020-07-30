#pragma once

#include "library/sp.h"
#include "library/strings.h"
#include <map>

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
	int kerning;
	std::map<char32_t, sp<PaletteImage>> fontbitmaps;
	UString name;
	sp<Palette> palette;

  public:
	virtual ~BitmapFont();
	virtual sp<PaletteImage> getGlyph(char32_t codepoint);
	virtual sp<PaletteImage> getString(const UString &Text);
	virtual int getFontWidth(const UString &Text);
	virtual int getFontHeight() const;
	virtual int getFontHeight(const UString &Text, int MaxWidth);
	virtual UString getName() const;
	virtual int getEstimateCharacters(int FitInWidth) const;
	virtual sp<Palette> getPalette() const;
	std::list<UString> wordWrapText(const UString &Text, int MaxWidth);

	/* Reads in set of "Character":"glyph description string" pairs */
	static sp<BitmapFont> loadFont(const std::map<char32_t, UString> &charMap, int spaceWidth,
	                               int fontHeight, int kerning, UString fontName,
	                               sp<Palette> defaultPalette);
};

}; // namespace OpenApoc


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
  public:
	virtual ~BitmapFont();
	virtual sp<PaletteImage> getGlyph(UniChar codepoint) = 0;
	virtual sp<PaletteImage> getString(const UString &Text);
	virtual int GetFontHeight() = 0;
	virtual int GetFontWidth(const UString &Text);
	virtual UString getName() = 0;
	virtual int GetEstimateCharacters(int FitInWidth) = 0;
	virtual sp<Palette> getPalette() = 0;
};

}; // namespace OpenApoc

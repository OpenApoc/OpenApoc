
#pragma once

#include "framework/includes.h"
#include "library/strings.h"

#define APOCFONT_ALIGN_LEFT	0
#define APOCFONT_ALIGN_CENTRE	1
#define APOCFONT_ALIGN_RIGHT	2

namespace OpenApoc {

class PaletteImage;

class BitmapFont
{
	public:
		virtual ~BitmapFont();
		virtual std::shared_ptr<PaletteImage> getGlyph(UniChar codepoint) = 0;
		virtual std::shared_ptr<PaletteImage> getString(const UString& Text);
		virtual int GetFontHeight() = 0;
		virtual int GetFontWidth(const UString& Text);
		virtual UString getName() = 0;
		virtual int GetEstimateCharacters(int FitInWidth) = 0;
};

}; //namespace OpenApoc

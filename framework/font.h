
#pragma once

#include "framework/includes.h"
#include <unicode/unistr.h>

#define APOCFONT_ALIGN_LEFT	0
#define APOCFONT_ALIGN_CENTRE	1
#define APOCFONT_ALIGN_RIGHT	2

namespace OpenApoc {

class PaletteImage;

class BitmapFont
{
	public:
		virtual ~BitmapFont();
		virtual std::shared_ptr<PaletteImage> getGlyph(UChar codepoint) = 0;
		virtual std::shared_ptr<PaletteImage> getString(UString Text);
		virtual int GetFontHeight() = 0;
		virtual int GetFontWidth(UString Text);
		virtual UString getName() = 0;
};

}; //namespace OpenApoc

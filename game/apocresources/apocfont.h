
#pragma once

#include "framework/includes.h"
#include "framework/font.h"


namespace OpenApoc {

class Palette;
class Framework;
class Image;
class Renderer;

class ApocalypseFont : public BitmapFont
{

	private:
		ApocalypseFont(){};
		std::map<UniChar, std::shared_ptr<PaletteImage> > fontbitmaps;
		int spacewidth;
		int fontheight;
		UString name;

	public:
		static std::shared_ptr<ApocalypseFont> loadFont(Framework &fw, tinyxml2::XMLElement *fontElement);
		virtual ~ApocalypseFont();
		virtual std::shared_ptr<PaletteImage> getGlyph(UniChar codepoint);
		virtual int GetFontHeight();
		virtual UString getName();

};
}; //namespace OpenApoc

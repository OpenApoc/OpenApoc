
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
		std::map<UChar, std::shared_ptr<PaletteImage> > fontbitmaps;
		int spacewidth;
		int fontheight;
		std::string name;

	public:
		static std::shared_ptr<ApocalypseFont> loadFont(Framework &fw, tinyxml2::XMLElement *fontElement);
		virtual ~ApocalypseFont();
		virtual std::shared_ptr<PaletteImage> getGlyph(UChar codepoint);
		virtual int GetFontHeight();
		virtual std::string getName();

};
}; //namespace OpenApoc


#pragma once

#include "framework/includes.h"
#include "game/resources/ifont.h"

namespace OpenApoc {

class Palette;
class Framework;
class Image;
class Renderer;

class ApocalypseFont : public IFont
{

	private:
		std::vector<std::shared_ptr<Image> > fontbitmaps;
		std::vector<int> fontwidths;
		int spacewidth;
		int fontheight;

	public:
		static std::string FontCharacterSet;

		enum FontType
		{
			LargeFont,
			SmallFont,
			TinyFont
		};

		ApocalypseFont( Framework &fw, FontType Face, std::shared_ptr<Palette> ColourPalette );
		~ApocalypseFont();

		virtual void DrawString( Renderer &r, int X, int Y, std::string Text, int Alignment );

		virtual int GetFontHeight();
		virtual int GetFontWidth(std::string Text);

};
}; //namespace OpenApoc


#pragma once

#include "framework/includes.h"
#include "game/resources/ifont.h"

namespace OpenApoc {

class Palette;
class Framework;

class ApocalypseFont : public IFont
{

	private:
		std::vector<ALLEGRO_BITMAP*> fontbitmaps;
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

		ApocalypseFont( Framework &fw, FontType Face, Palette* ColourPalette );
		~ApocalypseFont();

		virtual void DrawString( int X, int Y, std::string Text, int Alignment );

		virtual int GetFontHeight();
		virtual int GetFontWidth(std::string Text);

		void DumpCharset();
};
}; //namespace OpenApoc

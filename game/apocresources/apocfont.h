
#pragma once

#include "../../framework/includes.h"
#include "../../library/memory.h"
#include "palette.h"

#define APOCFONT_ALIGN_LEFT	0
#define APOCFONT_ALIGN_CENTRE	1
#define APOCFONT_ALIGN_RIGHT	2

class ApocalypseFont
{

	private:
		static std::string FontCharacterSet;

		std::vector<ALLEGRO_BITMAP*> fontbitmaps;
		std::vector<int> fontwidths;
		int spacewidth;
		int fontheight;

	public:
		enum FontType
		{
			LargeFont,
			SmallFont,
			TinyFont
		};

		ApocalypseFont( FontType Face, Palette* ColourPalette );
		~ApocalypseFont();

		void DrawString( int X, int Y, std::string Text, int Alignment );

		int GetFontHeight();
		int GetFontWidth(std::string Text);

		void DumpCharset();
};

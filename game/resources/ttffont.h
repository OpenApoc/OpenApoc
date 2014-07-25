
#pragma once

#include "ifont.h"

class TTFFont : public IFont
{

	private:
		ALLEGRO_FONT* fontobject;
		int fontheight;

	public:
		TTFFont( std::string Filename, int Size );
		~TTFFont();

		virtual void DrawString( int X, int Y, std::string Text, int Alignment );

		virtual int GetFontHeight();
		virtual int GetFontWidth(std::string Text);
};


#pragma once

#include "ifont.h"
#include <allegro5/allegro_ttf.h>

namespace OpenApoc {

class TTFFont : public IFont
{

	private:
		ALLEGRO_FONT* fontobject;
		int fontheight;

	public:
		TTFFont( std::string Filename, int Size );
		~TTFFont();

		virtual void DrawString( Renderer &r, int X, int Y, std::string Text, int Alignment );

		virtual int GetFontHeight();
		virtual int GetFontWidth(std::string Text);
};

}; //namespace OpenApoc

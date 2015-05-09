#include "framework/logger.h"
#include "ttffont.h"

namespace OpenApoc {

TTFFont::TTFFont( std::string Filename, int Size )
{
	fontobject = al_load_ttf_font( Filename.c_str(), Size, 0 );
	fontheight = al_get_font_line_height( fontobject );
}

TTFFont::~TTFFont()
{
	al_destroy_font( fontobject );
}

void TTFFont::DrawString( Renderer &r, int X, int Y, std::string Text, int Alignment )
{
	LogError("NOT IMPLEMENTED");
	std::ignore = r;
	std::ignore = X;
	std::ignore = Y;
	std::ignore = Text;
	std::ignore = Alignment;
#if 0
	int xpos = X;
	int textlen = 0;

	if( Alignment != APOCFONT_ALIGN_LEFT )
	{
		textlen = GetFontWidth( Text );

		switch( Alignment )
		{
			case APOCFONT_ALIGN_CENTRE:
				xpos -= textlen / 2;
				break;
			case APOCFONT_ALIGN_RIGHT:
				xpos -= textlen;
				break;
		}
	}

	al_draw_text( fontobject, al_map_rgb( 255, 255, 255 ), xpos, Y, 0, Text.c_str() );
#endif
}

int TTFFont::GetFontHeight()
{
	return fontheight;
}

int TTFFont::GetFontWidth( std::string Text )
{
	return al_get_text_width( fontobject, Text.c_str() );
}

}; //namespace OpenApoc

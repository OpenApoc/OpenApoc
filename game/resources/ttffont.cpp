
#include "ttffont.h"

TTFFont::TTFFont( std::string Filename, int Size )
{
	fontobject = al_load_ttf_font( Filename.c_str(), Size, 0 );
	fontheight = al_get_font_line_height( fontobject );
}

TTFFont::~TTFFont()
{
	al_destroy_font( fontobject );
}

void TTFFont::DrawString( int X, int Y, std::string Text, int Alignment )
{
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
}

int TTFFont::GetFontHeight()
{
	return fontheight;
}

int TTFFont::GetFontWidth( std::string Text )
{
	return al_get_text_width( fontobject, Text.c_str() );
}

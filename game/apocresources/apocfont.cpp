
#include "apocfont.h"

namespace OpenApoc {

// TODO: Fix charstring
std::string ApocalypseFont::FontCharacterSet = "!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~¡¢£¤¥¦§¨©ª«¬®¯°±²³´µ¶·¸¹º»¼½¾¿ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖ??ÙÚÛÜÝÞŸßàáâãäåæçèéêëìíîïðñòóôõö??ùúûüýþÿ???????????????????????????????????????????????????????????????????????????????????????????Š??Ž?????š??ž??Œœ????øØ????????????????????????????????????????????????????????????????????????????????";


ApocalypseFont::ApocalypseFont( FontType Face, Palette* ColourPalette )
{
	int fontchars;
	int charmaxwidth;

	std::string datfile( "UFODATA/" );
	switch( Face )
	{
		case ApocalypseFont::LargeFont:
			datfile.append( "BIGFONT" );
			fontheight = 24;
			fontchars = 129;
			charmaxwidth = 14;
			spacewidth = 6;
			break;
		case ApocalypseFont::SmallFont:
			datfile.append( "SMALFONT" );
			fontheight = 15;
			fontchars = 140;
			charmaxwidth = 14;
			spacewidth = 3;
			break;
		case ApocalypseFont::TinyFont:
			datfile.append( "SMALLSET" );
			fontheight = 12;
			fontchars = 128;
			charmaxwidth = 8;
			spacewidth = 3;
			break;
	}
	std::string spcfile( datfile );
	datfile.append( ".DAT" );

	ALLEGRO_FILE* dathnd = DATA->load_file( datfile, "rb" );

	for( int c = 0; c < fontchars; c++ )
	{
		int w = 0;
		ALLEGRO_BITMAP* b = al_create_bitmap( charmaxwidth, fontheight );
		ALLEGRO_LOCKED_REGION* r = al_lock_bitmap( b, ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE, 0 );
		for( int y = 0; y < fontheight; y++ )
		{
			for( int x = 0; x < charmaxwidth; x++ )
			{
				int palidx = al_fgetc( dathnd );
				Colour* rowptr = (Colour*)(&((char*)r->data)[ (y * r->pitch) + (x * 4) ]);
				Colour &palcol = ColourPalette->GetColour( palidx );
				*rowptr = palcol;
				if ( palcol.a > 0 && x > w )
				{
					w = x;
				}
			}
		}
		al_unlock_bitmap( b );
		fontbitmaps.push_back( b );
		fontwidths.push_back( w + 2 );
	}

	al_fclose( dathnd );
}

ApocalypseFont::~ApocalypseFont()
{
}

void ApocalypseFont::DrawString( int X, int Y, std::string Text, int Alignment )
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

	for( unsigned int i = 0; i < Text.length(); i++ )
	{
		int charidx = FontCharacterSet.find_first_of( Text.at( i ) );
		if( charidx >= 0 && charidx < fontbitmaps.size() )
		{
			al_draw_bitmap( fontbitmaps.at( charidx ), xpos, Y, 0 );
			xpos += fontwidths.at( charidx );
		} else {
			xpos += spacewidth;
		}
	}
}

int ApocalypseFont::GetFontHeight()
{
	return fontheight;
}

int ApocalypseFont::GetFontWidth( std::string Text )
{
	int textlen = 0;
	for( unsigned int i = 0; i < Text.length(); i++ )
	{
		int charidx = FontCharacterSet.find_first_of( Text.at( i ) );
		if( charidx >= 0 && charidx < fontbitmaps.size() )
		{
			textlen += fontwidths.at( charidx );
		} else {
			textlen += spacewidth;
		}
	}
	return textlen;
}

void ApocalypseFont::DumpCharset()
{
	std::string outfile;
	char val[200];

	for( unsigned int i = 0; i < fontbitmaps.size(); i++ )
	{
		memset( (void*)val, 0, 200 );
		sprintf( (char*)&val, "%d", i );
		outfile.clear();
		outfile.append( "data/" );
		outfile.append( (char*)&val );
		outfile.append( ".png" );
		al_save_bitmap( outfile.c_str(), fontbitmaps.at( i ) );
	}
}
}; //namespace OpenApoc

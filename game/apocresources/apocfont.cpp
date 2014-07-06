
#include "apocfont.h"

// TODO: Fix charstring
std::string ApocalypseFont::FontCharacterSet = "!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}";

ApocalypseFont::ApocalypseFont( bool LargeFont )
{
	int fontrows;
	int fontchars;

	std::string datfile( "data/UFODATA/" );
	if( LargeFont )
	{
		datfile.append( "BIGFONT" );
		fontrows = 24;
		fontchars = 129;
	} else {
		datfile.append( "SMALFONT" );
		fontrows = 15;
		fontchars = 140;
	}
	std::string spcfile( datfile );
	datfile.append( ".DAT" );
	spcfile.append( ".SPC" );

	ALLEGRO_FILE* dathnd = al_fopen( datfile.c_str(), "rb" );

	for( int c = 0; c < fontchars; c++ )
	{
		int w = 0;
		ALLEGRO_BITMAP* b = al_create_bitmap( 14, fontrows );
		ALLEGRO_LOCKED_REGION* r = al_lock_bitmap( b, ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE, 0 );
		for( int y = 0; y < fontrows; y++ )
		{
			for( int x = 0; x < 14; x++ )
			{
				int palidx = al_fgetc( dathnd );
				char* rowptr = &((char*)r->data)[ (y * r->pitch) + (x * 4) ];
				if( palidx == 0 )
				{
					rowptr[0] = 0;
					rowptr[1] = 0;
					rowptr[2] = 0;
					rowptr[3] = 0;
				} else {
					rowptr[3] = 255;
					if( LargeFont )
					{
						rowptr[0] = 256 - ((palidx * 16) + palidx);
					} else {
						rowptr[0] = 255 / palidx;
					}
					rowptr[1] = rowptr[0];
					rowptr[2] = rowptr[0];
					if( x > w )
					{
						w = x;
					}
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
		for( int i = 0; i < Text.length(); i++ )
		{
			int charidx = FontCharacterSet.find_first_of( Text.at( i ) );
			if( charidx >= 0 )
			{
				textlen += fontwidths.at( charidx ); // Fix with spacing data
			} else {
				textlen += 3;
			}
		}

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

	for( int i = 0; i < Text.length(); i++ )
	{
		int charidx = FontCharacterSet.find_first_of( Text.at( i ) );
		if( charidx >= 0 )
		{
			al_draw_bitmap( fontbitmaps.at( charidx ), xpos, Y, 0 );
			xpos += fontwidths.at( charidx ); // Fix with spacing data
		} else {
			xpos += 3;
		}
	}
}

void ApocalypseFont::DumpCharset()
{
	std::string outfile;
	char val[200];

	for( int i = 0; i < fontbitmaps.size(); i++ )
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

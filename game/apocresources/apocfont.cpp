
#include "apocfont.h"

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
		ALLEGRO_BITMAP* b = al_create_bitmap( 14, fontrows );
		ALLEGRO_LOCKED_REGION* r = al_lock_bitmap( b, ALLEGRO_PIXEL_FORMAT_SINGLE_CHANNEL_8, 0 );
		for( int y = 0; y < fontrows; y++ )
		{
			al_fread( dathnd, ((void*)r->data)[ y * r->pitch ], 14 );
		}
		al_unlock_bitmap( b );
		fontbitmaps.push_back( b );
	}

	al_fclose( dathnd );
}

ApocalypseFont::~ApocalypseFont()
{
}

void ApocalypseFont::DrawString( int X, int Y, std::string Text )
{
}

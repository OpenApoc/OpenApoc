
#include "palette.h"

Palette::Palette( std::wstring Filename )
{
	unsigned char colourblock[3];

	ALLEGRO_FILE* f = DATA->load_file( Filename, "rb" );

	colours = (Colour*)malloc( (al_fsize( f ) / 3) * sizeof( Colour ) );

	int idx = 0;
	while( !al_feof( f ) )
	{
		al_fread( f, (void*)&colourblock, 3 );
		colours[idx].a = (idx == 0 ? 0 : 255);
		colours[idx].r = colourblock[2] << 2;
		colours[idx].g = colourblock[1] << 2;
		colours[idx].b = colourblock[0] << 2;
		idx++;
	}

	al_fclose( f );
}

Palette::~Palette()
{
	free( (void*)colours );
}

Colour* Palette::GetColour(int Index)
{
	return &colours[Index];
}

void Palette::SetColour(int Index, Colour* Col)
{
	colours[Index].a = Col->a;
	colours[Index].r = Col->r;
	colours[Index].g = Col->g;
	colours[Index].b = Col->b;
}

void Palette::DumpPalette( std::wstring Filename )
{
	ALLEGRO_BITMAP* b = al_create_bitmap( 16, 16 );
	ALLEGRO_LOCKED_REGION* r = al_lock_bitmap( b, ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE, 0 );
	
	int c = 0;
	for( int y = 0; y < 16; y++ )
	{
		for( int x = 0; x < 16; x++ )
		{
			Colour* c1_rowptr = (Colour*)(&((char*)r->data)[ (y * r->pitch) + (x * 4) ]);
			Colour* c1_palcol = GetColour( c );
			c1_rowptr->a = c1_palcol->a;
			c1_rowptr->r = c1_palcol->r;
			c1_rowptr->g = c1_palcol->g;
			c1_rowptr->b = c1_palcol->b;

			c++;
		}
	}

	al_unlock_bitmap( b );
	al_save_bitmap( Filename.c_str(), b );
	al_destroy_bitmap( b );
}

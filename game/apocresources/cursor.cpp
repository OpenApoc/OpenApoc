
#include "cursor.h"
#include "framework/framework.h"
#include "palette.h"

namespace OpenApoc {

Cursor::Cursor( Framework &fw, std::shared_ptr<Palette> ColourPalette )
{
	ALLEGRO_FILE* f = fw.data.load_file( "TACDATA/MOUSE.DAT", "rb" );

	while( images.size() < al_fsize( f ) / 576 )
	{
		ALLEGRO_BITMAP* b = al_create_bitmap( 24, 24 );
		ALLEGRO_LOCKED_REGION* r = al_lock_bitmap( b, ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE, 0 );
		for( int y = 0; y < 24; y++ )
		{
			for( int x = 0; x < 24; x++ )
			{
				int palidx = al_fgetc( f );
				Colour* rowptr = (Colour*)(&((char*)r->data)[ (y * r->pitch) + (x * 4) ]);
				Colour &palcol = ColourPalette->GetColour( palidx );
				*rowptr = palcol;
			}
		}
		al_unlock_bitmap( b );
		images.push_back( b );
	}

	al_fclose(f);

	CurrentType = Cursor::Normal;
	cursorx = 0;
	cursory = 0;

}

Cursor::~Cursor()
{
	while( images.size() > 0 )
	{
		al_destroy_bitmap( images.back() );
		images.pop_back();
	}
}

void Cursor::EventOccured( Event* e )
{
	if( e->Type == EVENT_MOUSE_MOVE )
	{
		cursorx = e->Data.Mouse.X;
		cursory = e->Data.Mouse.Y;
	}
}

void Cursor::Render()
{
	al_draw_bitmap( images.at( (int)CurrentType ), cursorx, cursory, 0 );
}
}; //namespace OpenApoc

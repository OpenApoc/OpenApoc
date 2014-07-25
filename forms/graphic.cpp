#include "graphic.h"

Graphic::Graphic( Control* Owner, ALLEGRO_BITMAP* Image ) : Control( Owner )
{
	image = Image;
	Size.X = al_get_bitmap_width( image );
	Size.Y = al_get_bitmap_height( image );
}

void Graphic::EventOccured( Event* e )
{
	Control::EventOccured( e );
}

void Graphic::Render()
{
	int bmpw = al_get_bitmap_width( image );
	int bmph = al_get_bitmap_height( image );
	if( bmpw == Size.X && bmph == Size.Y )
	{
		al_draw_bitmap( image, resolvedLocation.X, resolvedLocation.Y, 0 );
	} else {
		al_draw_scaled_bitmap( image, 0, 0, bmpw, bmph, resolvedLocation.X, resolvedLocation.Y, this->Size.X, this->Size.Y, 0 );
	}

	PostRender();
}

void Graphic::Update()
{
	Control::Update();
}
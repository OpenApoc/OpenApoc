#include "graphic.h"
#include "../game/resources/gamecore.h"

Graphic::Graphic( Control* Owner, std::string Image ) : Control( Owner )
{
	image_name = Image;
	image = nullptr;
}

Graphic::~Graphic()
{
}

void Graphic::EventOccured( Event* e )
{
	Control::EventOccured( e );
}

void Graphic::Render()
{
	if( image == nullptr )
	{
		image = GAMECORE->GetImage( image_name );
		if( Size.X == 0 )
		{
			Size.X = al_get_bitmap_width( image );
		}
		if( Size.Y == 0 )
		{
			Size.Y = al_get_bitmap_height( image );
		}
	}

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

void Graphic::UnloadResources()
{
	if( image != nullptr )
	{
		al_destroy_bitmap( image );
		image = nullptr;
	}
	Control::UnloadResources();
}

ALLEGRO_BITMAP* Graphic::GetImage()
{
	return image;
}

void Graphic::SetImage( ALLEGRO_BITMAP* Image )
{
	image = Image;
}


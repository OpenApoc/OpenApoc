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

void Graphic::OnRender()
{
	if( !image )
	{
		image = GAMECORE->GetImage( image_name );
		if( Size.X == 0 )
		{
			Size.X = image->width;
		}
		if( Size.Y == 0 )
		{
			Size.Y = image->height;
		}
	}

	int bmpw = image->width;
	int bmph = image->height;
	if( bmpw == Size.X && bmph == Size.Y )
	{
		image->draw(0, 0 );
	} else {
		image->drawScaled(0, 0, bmpw, bmph, 0, 0, this->Size.X, this->Size.Y);
	}
}

void Graphic::Update()
{
	Control::Update();
}

void Graphic::UnloadResources()
{
	image.reset();
	Control::UnloadResources();
}

std::shared_ptr<Image> Graphic::GetImage()
{
	return image;
}

void Graphic::SetImage( std::shared_ptr<Image> Image )
{
	image = Image;
}


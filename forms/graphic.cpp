#include "graphic.h"
#include "../game/resources/gamecore.h"

namespace OpenApoc {

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
		image = FRAMEWORK->gamecore->GetImage( image_name );
		if( Size.x == 0 )
		{
			Size.x = image->width;
		}
		if( Size.y == 0 )
		{
			Size.y = image->height;
		}
	}

	int bmpw = image->width;
	int bmph = image->height;
	if( bmpw == Size.x && bmph == Size.y )
	{
		image->draw(0, 0 );
	} else {
		image->drawScaled(0, 0, bmpw, bmph, 0, 0, this->Size.x, this->Size.y);
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

}; //namespace OpenApoc

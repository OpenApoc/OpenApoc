#include "graphic.h"
#include "game/resources/gamecore.h"
#include "framework/framework.h"

namespace OpenApoc {

Graphic::Graphic( Framework &fw, Control* Owner, std::string Image ) : Control( fw, Owner )
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
		image = fw.gamecore->GetImage( image_name );
		if( Size.x == 0 )
		{
			Size.x = image->size.x;
		}
		if( Size.y == 0 )
		{
			Size.y = image->size.y;
		}
	}

	if(Size == image->size)
	{
		fw.renderer->draw(*image, Vec2<float>{0,0});
	} else {
		fw.renderer->drawScaled(*image, Vec2<float>{0,0}, Vec2<float>{this->Size.x, this->Size.y});
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

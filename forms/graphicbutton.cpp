
#include "graphicbutton.h"
#include "../framework/framework.h"
#include "../game/resources/gamecore.h"

namespace OpenApoc {

RawSound* GraphicButton::buttonclick = nullptr;

GraphicButton::GraphicButton( Control* Owner, std::string Image, std::string ImageDepressed ) : Control( Owner )
{
	image = nullptr;
	imagedepressed = nullptr;
	imagehover = nullptr;
	image_name = Image;
	imagedepressed_name = ImageDepressed;
	imagehover_name = "";
	if( buttonclick == nullptr )
	{
		buttonclick = new RawSound( "STRATEGC/INTRFACE/BUTTON1.RAW" );
	}
}

GraphicButton::GraphicButton( Control* Owner, std::string Image, std::string ImageDepressed, std::string ImageHover ) : Control( Owner )
{
	image = nullptr;
	imagedepressed = nullptr;
	imagehover = nullptr;
	image_name = Image;
	imagedepressed_name = ImageDepressed;
	imagehover_name = ImageHover;
	if( buttonclick == nullptr )
	{
		buttonclick = new RawSound( "STRATEGC/INTRFACE/BUTTON1.RAW" );
	}
}

GraphicButton::~GraphicButton()
{
}

void GraphicButton::EventOccured( Event* e )
{
	Control::EventOccured( e );

	if( e->Type == EVENT_FORM_INTERACTION && e->Data.Forms.RaisedBy == this && e->Data.Forms.EventFlag == FormEventType::MouseDown )
	{
		buttonclick->PlaySound();
	}

	if( e->Type == EVENT_FORM_INTERACTION && e->Data.Forms.RaisedBy == this && e->Data.Forms.EventFlag == FormEventType::MouseClick )
	{
		Event* ce = new Event();
		ce->Type = e->Type;
		memcpy( (void*)&(ce->Data.Forms), (void*)&(e->Data.Forms), sizeof( FRAMEWORK_FORMS_EVENT ) );
		ce->Data.Forms.EventFlag = FormEventType::ButtonClick;
		FRAMEWORK->PushEvent( ce );
	}
}

void GraphicButton::OnRender()
{
	std::shared_ptr<Image> useimage;

	if( !image && image_name != "" )
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
	if( imagedepressed == nullptr && imagedepressed_name != "" )
	{
		imagedepressed = FRAMEWORK->gamecore->GetImage( imagedepressed_name );
		if( Size.x == 0 )
		{
			Size.x = imagedepressed->width;
		}
		if( Size.y == 0 )
		{
			Size.y = imagedepressed->height;
		}
	}
	if( imagehover == nullptr && imagehover_name != "" )
	{
		imagehover = FRAMEWORK->gamecore->GetImage( imagehover_name );
		if( Size.x == 0 )
		{
			Size.x = imagehover->width;
		}
		if( Size.y == 0 )
		{
			Size.y = imagehover->height;
		}
	}

	useimage = image;
	if( mouseDepressed )
	{
		useimage = imagedepressed;
	} else if( mouseInside && imagehover != nullptr ) {
		useimage = imagehover;
	}

	if( useimage != nullptr )
	{
		int bmpw = useimage->width;
		int bmph = useimage->height;
		if( bmpw == Size.x && bmph == Size.y )
		{
			useimage->draw(0, 0 );
		} else {
			useimage->drawScaled(0, 0, bmpw, bmph, 0, 0, this->Size.x, this->Size.y);
		}
	}
}

void GraphicButton::Update()
{
	Control::Update();
}

void GraphicButton::UnloadResources()
{
	image.reset();
	imagedepressed.reset();
	imagehover.reset();
	Control::UnloadResources();
}

std::shared_ptr<Image> GraphicButton::GetImage()
{
	return image;
}

void GraphicButton::SetImage( std::shared_ptr<Image> Image )
{
	image_name = "";
	image = Image;
}

std::shared_ptr<Image> GraphicButton::GetDepressedImage()
{
	return imagedepressed;
}

void GraphicButton::SetDepressedImage( std::shared_ptr<Image> Image )
{
	imagedepressed_name = "";
	imagedepressed = Image;
}

std::shared_ptr<Image> GraphicButton::GetHoverImage()
{
	return imagehover;
}

void GraphicButton::SetHoverImage( std::shared_ptr<Image> Image )
{
	imagehover_name = "";
	imagehover = Image;
}

}; //namespace OpenApoc

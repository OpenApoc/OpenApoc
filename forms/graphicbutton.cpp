
#include "graphicbutton.h"
#include "../framework/framework.h"
#include "../game/resources/gamecore.h"

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
	if( imagedepressed == nullptr && imagedepressed_name != "" )
	{
		imagedepressed = GAMECORE->GetImage( imagedepressed_name );
		if( Size.X == 0 )
		{
			Size.X = imagedepressed->width;
		}
		if( Size.Y == 0 )
		{
			Size.Y = imagedepressed->height;
		}
	}
	if( imagehover == nullptr && imagehover_name != "" )
	{
		imagehover = GAMECORE->GetImage( imagehover_name );
		if( Size.X == 0 )
		{
			Size.X = imagehover->width;
		}
		if( Size.Y == 0 )
		{
			Size.Y = imagehover->height;
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
		if( bmpw == Size.X && bmph == Size.Y )
		{
			useimage->draw(0, 0 );
		} else {
			useimage->drawScaled(0, 0, bmpw, bmph, 0, 0, this->Size.X, this->Size.Y);
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


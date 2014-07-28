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
}

GraphicButton::GraphicButton( Control* Owner, std::string Image, std::string ImageDepressed, std::string ImageHover ) : Control( Owner )
{
	image = nullptr;
	imagedepressed = nullptr;
	imagehover = nullptr;
	image_name = Image;
	imagedepressed_name = ImageDepressed;
	imagehover_name = ImageHover;
}

GraphicButton::~GraphicButton()
{
	UnloadResources();
	Control::~Control();
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

void GraphicButton::Render()
{
	ALLEGRO_BITMAP* useimage;

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
	if( imagedepressed == nullptr )
	{
		imagedepressed = GAMECORE->GetImage( imagedepressed_name );
	}
	if( imagehover == nullptr && imagehover_name != "" )
	{
		imagehover = GAMECORE->GetImage( imagehover_name );
	}

	useimage = image;
	if( mouseDepressed )
	{
		useimage = imagedepressed;
	} else if( mouseInside && imagehover != nullptr ) {
		useimage = imagehover;
	}

	int bmpw = al_get_bitmap_width( useimage );
	int bmph = al_get_bitmap_height( useimage );
	if( bmpw == Size.X && bmph == Size.Y )
	{
		al_draw_bitmap( useimage, resolvedLocation.X, resolvedLocation.Y, 0 );
	} else {
		al_draw_scaled_bitmap( useimage, 0, 0, bmpw, bmph, resolvedLocation.X, resolvedLocation.Y, this->Size.X, this->Size.Y, 0 );
	}

	PostRender();
}

void GraphicButton::Update()
{
	Control::Update();
}

void GraphicButton::UnloadResources()
{
	if( image != nullptr )
	{
		al_destroy_bitmap( image );
		image = nullptr;
	}
	if( imagedepressed != nullptr )
	{
		al_destroy_bitmap( imagedepressed );
		imagedepressed = nullptr;
	}
	if( imagehover != nullptr )
	{
		al_destroy_bitmap( imagehover );
		imagehover = nullptr;
	}
	Control::UnloadResources();
}

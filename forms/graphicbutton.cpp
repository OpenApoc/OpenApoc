#include "graphicbutton.h"
#include "../framework/framework.h"

RawSound* GraphicButton::buttonclick = nullptr;

GraphicButton::GraphicButton( Control* Owner, ALLEGRO_BITMAP* Image, ALLEGRO_BITMAP* ImageDepressed ) : Control( Owner )
{
	image = Image;
	imagedepressed = ImageDepressed;
	imagehover = nullptr;
	Size.X = al_get_bitmap_width( image );
	Size.Y = al_get_bitmap_height( image );
}

GraphicButton::GraphicButton( Control* Owner, ALLEGRO_BITMAP* Image, ALLEGRO_BITMAP* ImageDepressed, ALLEGRO_BITMAP* ImageHover ) : Control( Owner )
{
	image = Image;
	imagedepressed = ImageDepressed;
	imagehover = ImageHover;
	Size.X = al_get_bitmap_width( image );
	Size.Y = al_get_bitmap_height( image );
}

GraphicButton::~GraphicButton()
{
	al_destroy_bitmap( image );
	al_destroy_bitmap( imagedepressed );
	if( imagehover != nullptr )
	{
		al_destroy_bitmap( imagehover );
	}
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
	Vector2* v = GetResolvedLocation();
	ALLEGRO_BITMAP* useimage;

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
		al_draw_bitmap( useimage, v->X, v->Y, 0 );
	} else {
		al_draw_scaled_bitmap( useimage, 0, 0, bmpw, bmph, v->X, v->Y, this->Size.X, this->Size.Y, 0 );
	}
	delete v;

	PostRender();
}

void GraphicButton::Update()
{
	Control::Update();
}
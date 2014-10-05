
#include "textbutton.h"
#include "../framework/framework.h"

namespace OpenApoc {

RawSound* TextButton::buttonclick = nullptr;

TextButton::TextButton( Control* Owner, std::string Text, IFont* Font ) : Control( Owner ), text( Text ), font( Font ), TextHAlign( HorizontalAlignment::Centre ), TextVAlign( VerticalAlignment::Centre ), buttonbackground(DATA->load_image( "UI/TEXTBUTTONBACK.PNG" ))
{
	if( buttonclick == nullptr )
	{
		buttonclick = new RawSound( "STRATEGC/INTRFACE/BUTTON1.RAW" );
	}
}

TextButton::~TextButton()
{
}

void TextButton::EventOccured( Event* e )
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

void TextButton::OnRender()
{
	buttonbackground->drawScaled(0, 0, buttonbackground->width, buttonbackground->height, 0, 0, Size.x, Size.y);
	al_draw_filled_rectangle( 3,  3, Size.x - 2, Size.y - 2, al_map_rgb( 160, 160, 160 ) );

	al_draw_line( 2, 4, Size.x - 2, 3, al_map_rgb( 220, 220, 220 ), 1 );
	al_draw_line( 2,  Size.y - 4, Size.x - 2, Size.y - 4, al_map_rgb( 80, 80, 80 ), 1 );
	al_draw_line( 2, Size.y - 3, Size.x - 2, Size.y - 3, al_map_rgb( 64, 64, 64 ), 1 );
	al_draw_rectangle( 3, 3, Size.x - 2, Size.y - 2, al_map_rgb( 48, 48, 48 ), 1 );

	if( mouseDepressed && mouseInside )
	{
		al_draw_rectangle( 1, 1, Size.x - 1, Size.y - 1, al_map_rgb( 255, 255, 255 ), 2 );
	}
	
	int xpos;
	int ypos;

	switch( TextHAlign )
	{
		case HorizontalAlignment::Left:
			xpos = 0;
			break;
		case HorizontalAlignment::Centre:
			xpos = (Size.x / 2) - (font->GetFontWidth( text ) / 2);
			break;
		case HorizontalAlignment::Right:
			xpos = Size.x - font->GetFontWidth( text );
			break;
	}

	switch( TextVAlign )
	{
		case VerticalAlignment::Top:
			ypos = 0;
			break;
		case VerticalAlignment::Centre:
			ypos = (Size.y / 2) - (font->GetFontHeight() / 2);
			break;
		case VerticalAlignment::Bottom:
			ypos = Size.y - font->GetFontHeight();
			break;
	}

	font->DrawString( xpos, ypos, text, APOCFONT_ALIGN_LEFT );
}

void TextButton::Update()
{
	// No "updates"
}

void TextButton::UnloadResources()
{
}

std::string TextButton::GetText()
{
	return text;
}

void TextButton::SetText( std::string Text )
{
	text = Text;
}

}; //namespace OpenApoc

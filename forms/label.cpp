
#include "label.h"

Label::Label( Control* Owner, std::string Text, IFont* Font ) : Control( Owner ), text( Text ), font( Font ), TextHAlign( HorizontalAlignment::Left ), TextVAlign( VerticalAlignment::Top )
{
}

void Label::EventOccured( Event* e )
{
	Control::EventOccured( e );
}

void Label::Render()
{
	PreRender();

	Vector2* v = GetResolvedLocation();
	int xpos;
	int ypos;

	switch( TextHAlign )
	{
		case HorizontalAlignment::Left:
			xpos = v->X;
			break;
		case HorizontalAlignment::Centre:
			xpos = v->X + (Size.X / 2) - (font->GetFontWidth( text ) / 2);
			break;
		case HorizontalAlignment::Right:
			xpos = v->X + Size.X - font->GetFontWidth( text );
			break;
	}

	switch( TextVAlign )
	{
		case VerticalAlignment::Top:
			ypos = v->Y;
			break;
		case VerticalAlignment::Centre:
			ypos = v->Y + (Size.Y / 2) - (font->GetFontHeight() / 2);
			break;
		case VerticalAlignment::Bottom:
			ypos = v->Y + Size.Y - font->GetFontHeight();
			break;
	}


	font->DrawString( xpos, ypos, text, APOCFONT_ALIGN_LEFT );
	delete v;

	PostRender();
}

void Label::Update()
{
	// No "updates"
}

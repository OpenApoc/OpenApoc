
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

	int xpos;
	int ypos;

	switch( TextHAlign )
	{
		case HorizontalAlignment::Left:
			xpos = resolvedLocation.X;
			break;
		case HorizontalAlignment::Centre:
			xpos = resolvedLocation.X + (Size.X / 2) - (font->GetFontWidth( text ) / 2);
			break;
		case HorizontalAlignment::Right:
			xpos = resolvedLocation.X + Size.X - font->GetFontWidth( text );
			break;
	}

	switch( TextVAlign )
	{
		case VerticalAlignment::Top:
			ypos = resolvedLocation.Y;
			break;
		case VerticalAlignment::Centre:
			ypos = resolvedLocation.Y + (Size.Y / 2) - (font->GetFontHeight() / 2);
			break;
		case VerticalAlignment::Bottom:
			ypos = resolvedLocation.Y + Size.Y - font->GetFontHeight();
			break;
	}


	font->DrawString( xpos, ypos, text, APOCFONT_ALIGN_LEFT );

	// PostRender();
}

void Label::Update()
{
	// No "updates"
}


#include "label.h"

namespace OpenApoc {

Label::Label( Framework &fw, Control* Owner, std::string Text, IFont* Font ) : Control( fw, Owner ), text( Text ), font( Font ), TextHAlign( HorizontalAlignment::Left ), TextVAlign( VerticalAlignment::Top )
{
}

Label::~Label()
{
}

void Label::EventOccured( Event* e )
{
	Control::EventOccured( e );
}

void Label::OnRender()
{
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

void Label::Update()
{
	// No "updates"
}

void Label::UnloadResources()
{
}

std::string Label::GetText()
{
	return text;
}

void Label::SetText( std::string Text )
{
	text = Text;
}

}; //namespace OpenApoc

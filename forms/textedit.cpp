
#include "textedit.h"

TextEdit::TextEdit( Control* Owner, std::string Text, IFont* Font ) : Control( Owner ), text( Text ), font( Font ), TextHAlign( HorizontalAlignment::Left ), TextVAlign( VerticalAlignment::Top ), editting(false), SelectionStart(0), SelectionLength(0), caretTimer(0), caretDraw(false)
{
}

TextEdit::~TextEdit()
{
}

void TextEdit::EventOccured( Event* e )
{
	Control::EventOccured( e );

	if( e->Type == EVENT_FORM_INTERACTION )
	{
		if( e->Data.Forms.RaisedBy == this )
		{
			if( e->Data.Forms.EventFlag == FormEventType::GotFocus || e->Data.Forms.EventFlag == FormEventType::MouseClick || e->Data.Forms.EventFlag == FormEventType::KeyDown )
			{
				editting = true;
			}
			if( e->Data.Forms.EventFlag == FormEventType::LostFocus )
			{
				editting = false;
			}

			if( e->Data.Forms.EventFlag == FormEventType::KeyDown && editting )
			{
				e->Handled = true;
			}

		} else if( e->Data.Forms.EventFlag == FormEventType::MouseClick || e->Data.Forms.EventFlag == FormEventType::KeyDown ) {
			editting = false;
		}
	}

}

void TextEdit::OnRender()
{
	int xpos;
	int ypos;

	switch( TextHAlign )
	{
		case HorizontalAlignment::Left:
			xpos = 0;
			break;
		case HorizontalAlignment::Centre:
			xpos = (Size.X / 2) - (font->GetFontWidth( text ) / 2);
			break;
		case HorizontalAlignment::Right:
			xpos = Size.X - font->GetFontWidth( text );
			break;
	}

	switch( TextVAlign )
	{
		case VerticalAlignment::Top:
			ypos = 0;
			break;
		case VerticalAlignment::Centre:
			ypos = (Size.Y / 2) - (font->GetFontHeight() / 2);
			break;
		case VerticalAlignment::Bottom:
			ypos = Size.Y - font->GetFontHeight();
			break;
	}

	if( editting )
	{
		int cxpos = xpos + font->GetFontWidth( text.substr( 0, SelectionStart ) );
		int cw = font->GetFontWidth( text.substr( SelectionStart, SelectionLength ) );

		// TODO: Draw "Selected" region
		al_draw_filled_rectangle( cxpos, ypos, cxpos + cw, ypos + font->GetFontHeight(), al_map_rgb(220, 220, 220) );
		
		if( caretDraw )
		{
			al_draw_line( cxpos, ypos, cxpos, ypos + font->GetFontHeight(), al_map_rgb( 255, 255, 255 ), 1 );
		}
	}

	font->DrawString( xpos, ypos, text, APOCFONT_ALIGN_LEFT );
}

void TextEdit::Update()
{
	if( editting )
	{
		caretTimer = (caretTimer + 1) % TEXTEDITOR_CARET_TOGGLE_TIME;
		if( caretTimer == 0 )
		{
			caretDraw = !caretDraw;
		}
	}
}

void TextEdit::UnloadResources()
{
}

std::string TextEdit::GetText()
{
	return text;
}

void TextEdit::SetText( std::string Text )
{
	text = Text;
	SelectionStart = text.length();
	SelectionLength = 0;
}

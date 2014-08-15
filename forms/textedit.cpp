
#include "textedit.h"
#include <iostream>
#include <algorithm>

TextEdit::TextEdit( Control* Owner, std::string Text, IFont* Font ) : Control( Owner ), text( Text ), font( Font ), TextHAlign( HorizontalAlignment::Left ), TextVAlign( VerticalAlignment::Top ), editting(false), SelectionStart(0), caretTimer(0), caretDraw(false), editShift(false), editAltGr(false)
{
}

TextEdit::~TextEdit()
{
}

void TextEdit::EventOccured( Event* e )
{
	std::string keyname;

	Control::EventOccured( e );

	if( e->Type == EVENT_FORM_INTERACTION )
	{
		if( e->Data.Forms.RaisedBy == this )
		{
			if( e->Data.Forms.EventFlag == FormEventType::GotFocus || e->Data.Forms.EventFlag == FormEventType::MouseClick || e->Data.Forms.EventFlag == FormEventType::KeyDown )
			{
				editting = true;
				e->Handled = true;
			}
			if( e->Data.Forms.EventFlag == FormEventType::LostFocus )
			{
				editting = false;
				e->Handled = true;
			}
		} else if( e->Data.Forms.EventFlag == FormEventType::MouseClick ) {
			editting = false;
		}

		if( e->Data.Forms.EventFlag == FormEventType::KeyDown && editting )
		{

			switch( e->Data.Forms.KeyInfo.KeyCode )
			{
				case ALLEGRO_KEY_BACKSPACE:
					if( SelectionStart > 0 )
					{
						text.erase( text.begin() + SelectionStart - 1, text.begin() + SelectionStart );
						SelectionStart--;
					}
					e->Handled = true;
					break;
				case ALLEGRO_KEY_DELETE:
					if( SelectionStart < text.length() )
					{
						text.erase( text.begin() + SelectionStart, text.begin() + SelectionStart + 1 );
					}
					e->Handled = true;
					break;
				case ALLEGRO_KEY_LEFT:
					if( SelectionStart > 0 )
					{
						SelectionStart--;
					}
					e->Handled = true;
					break;
				case ALLEGRO_KEY_RIGHT:
					if( SelectionStart < text.length() )
					{
						SelectionStart++;
					}
					e->Handled = true;
					break;
				case ALLEGRO_KEY_LSHIFT:
				case ALLEGRO_KEY_RSHIFT:
					editShift = true;
					break;
				case ALLEGRO_KEY_ALTGR:
					editAltGr = true;
					break;

				case ALLEGRO_KEY_HOME:
					SelectionStart = 0;
					e->Handled = true;
					break;
				case ALLEGRO_KEY_END:
					SelectionStart = text.length();
					e->Handled = true;
					break;

				//case ALLEGRO_KEY_A:
				//case ALLEGRO_KEY_B:
				//case ALLEGRO_KEY_C:
				//case ALLEGRO_KEY_D:
				//case ALLEGRO_KEY_E:
				//case ALLEGRO_KEY_F:
				//case ALLEGRO_KEY_G:
				//case ALLEGRO_KEY_H:
				//case ALLEGRO_KEY_I:
				//case ALLEGRO_KEY_J:
				//case ALLEGRO_KEY_K:
				//case ALLEGRO_KEY_L:
				//case ALLEGRO_KEY_M:
				//case ALLEGRO_KEY_N:
				//case ALLEGRO_KEY_O:
				//case ALLEGRO_KEY_P:
				//case ALLEGRO_KEY_Q:
				//case ALLEGRO_KEY_R:
				//case ALLEGRO_KEY_S:
				//case ALLEGRO_KEY_T:
				//case ALLEGRO_KEY_U:
				//case ALLEGRO_KEY_V:
				//case ALLEGRO_KEY_W:
				//case ALLEGRO_KEY_X:
				//case ALLEGRO_KEY_Y:
				//case ALLEGRO_KEY_Z:
				//	keyname = al_keycode_to_name( e->Data.Forms.KeyInfo.KeyCode );
				//	if( !editShift ) // (e->Data.Forms.KeyInfo.Modifiers & ALLEGRO_KEYMOD_SHIFT) != ALLEGRO_KEYMOD_SHIFT ) <- Modifiers don't seem to be working?!
				//	{
				//		std::transform(keyname.begin(), keyname.end(), keyname.begin(), ::tolower);
				//	}
				//	text.insert( SelectionStart, keyname );
				//	SelectionStart++;
				//	e->Handled = true;
				//	break;

				//case ALLEGRO_KEY_0:
				//case ALLEGRO_KEY_1:
				//case ALLEGRO_KEY_2:
				//case ALLEGRO_KEY_3:
				//case ALLEGRO_KEY_4:
				//case ALLEGRO_KEY_5:
				//case ALLEGRO_KEY_6:
				//case ALLEGRO_KEY_7:
				//case ALLEGRO_KEY_8:
				//case ALLEGRO_KEY_9:
				//	keyname = al_keycode_to_name( e->Data.Forms.KeyInfo.KeyCode );
				//	if( !editShift )
				//	{
				//		std::transform(keyname.begin(), keyname.end(), keyname.begin(), ::tolower);
				//	}
				//	text.insert( SelectionStart, keyname );
				//	SelectionStart++;
				//	e->Handled = true;
				//	break;
			}

		}

		if( e->Data.Forms.EventFlag == FormEventType::KeyPress && editting )
		{
			ALLEGRO_USTR* convert = al_ustr_new("");
			al_ustr_append_chr( convert, e->Data.Forms.KeyInfo.UniChar );
			text.insert( SelectionStart, al_cstr(convert) );
			SelectionStart++;
		}

		if( e->Data.Forms.EventFlag == FormEventType::KeyUp && editting )
		{

			switch( e->Data.Forms.KeyInfo.KeyCode )
			{
				case ALLEGRO_KEY_LSHIFT:
				case ALLEGRO_KEY_RSHIFT:
					editShift = false;
					e->Handled = true;
					break;
				case ALLEGRO_KEY_ALTGR:
					editAltGr = false;
					e->Handled = true;
					break;
			}
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
		//int cw = (SelectionLength == 0 ? 0 : font->GetFontWidth( text.substr( SelectionStart, SelectionLength ) ));

		// TODO: Draw "Selected" region
		//al_draw_filled_rectangle( cxpos, ypos, cxpos + cw, ypos + font->GetFontHeight(), al_map_rgb(220, 220, 220) );
		
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
	//SelectionLength = 0;
}

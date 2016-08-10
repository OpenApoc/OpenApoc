#include "forms/textedit.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "library/sp.h"
#include <tinyxml2.h>

namespace OpenApoc
{

TextEdit::TextEdit(const UString &Text, sp<BitmapFont> font)
    : Control(), caretDraw(false), caretTimer(0), text(Text), cursor("*"), font(font),
      editting(false), editShift(false), editAltGr(false), SelectionStart(Text.length()),
      TextHAlign(HorizontalAlignment::Left), TextVAlign(VerticalAlignment::Centre)
{
	if (font)
	{
		palette = font->getPalette();
	}
}

TextEdit::~TextEdit() {}

bool TextEdit::IsFocused() const { return editting; }

void TextEdit::EventOccured(Event *e)
{
	UString keyname;

	Control::EventOccured(e);

	if (e->Type() == EVENT_FORM_INTERACTION)
	{
		if (e->Forms().RaisedBy == shared_from_this())
		{
			if (e->Forms().EventFlag == FormEventType::GotFocus ||
			    e->Forms().EventFlag == FormEventType::MouseClick ||
			    e->Forms().EventFlag == FormEventType::KeyDown)
			{
				editting = true;

				fw().Text_StartInput();
				// e->Handled = true;
				// FIXME: Should we really fall through here?
			}
		}
		if (editting)
		{
			if (e->Forms().RaisedBy == shared_from_this())
			{

				if (e->Forms().EventFlag == FormEventType::LostFocus)
				{
					editting = false;
					fw().Text_StopInput();
					RaiseEvent(FormEventType::TextEditFinish);
					// e->Handled = true;
				}
			}
			else if (e->Forms().EventFlag == FormEventType::MouseClick)
			{
				// FIXME: Due to event duplication (?), this code won't work. Can only stop editing
				// text by pressing enter.
				// editting = false;
				// fw().Text_StopInput();
				// RaiseEvent(FormEventType::TextEditFinish);
			}
			if (e->Forms().EventFlag == FormEventType::KeyDown)
			{
				LogInfo("Key pressed: %d", e->Forms().KeyInfo.KeyCode);
				switch (e->Forms().KeyInfo.KeyCode) // TODO: Check scancodes instead of keycodes?
				{
					case SDLK_BACKSPACE:
						if (SelectionStart > 0)
						{
							text.remove(SelectionStart - 1, 1);
							SelectionStart--;
							RaiseEvent(FormEventType::TextChanged);
						}
						e->Handled = true;
						break;
					case SDLK_DELETE:
						if (SelectionStart < text.length())
						{
							text.remove(SelectionStart, 1);
							RaiseEvent(FormEventType::TextChanged);
						}
						e->Handled = true;
						break;
					case SDLK_LEFT:
						if (SelectionStart > 0)
						{
							SelectionStart--;
						}
						e->Handled = true;
						break;
					case SDLK_RIGHT:
						if (SelectionStart < text.length())
						{
							SelectionStart++;
						}
						e->Handled = true;
						break;
					case SDLK_LSHIFT:
					case SDLK_RSHIFT:
						editShift = true;
						break;
					case SDLK_RALT:
						editAltGr = true;
						break;

					case SDLK_HOME:
						SelectionStart = 0;
						e->Handled = true;
						break;
					case SDLK_END:
						SelectionStart = text.length();
						e->Handled = true;
						break;

					case SDLK_RETURN:
						editting = false;
						fw().Text_StopInput();
						RaiseEvent(FormEventType::TextEditFinish);
						break;
				}
			}
			else if (e->Forms().EventFlag == FormEventType::KeyUp)
			{

				switch (e->Forms().KeyInfo.KeyCode)
				{
					case SDLK_LSHIFT:
					case SDLK_RSHIFT:
						editShift = false;
						e->Handled = true;
						break;
					case SDLK_RALT:
						editAltGr = false;
						e->Handled = true;
						break;
				}
			}
			else if (e->Forms().EventFlag == FormEventType::TextInput)
			{
				if (text.length() >= this->textMaxLength)
				{
					return;
				}

				UString inputCharacter = e->Forms().Input.Input;
				if (allowedCharacters.empty() ||
				    allowedCharacters.str().find(inputCharacter.str()) != std::string::npos)
				{
					text.insert(SelectionStart, inputCharacter);
					SelectionStart += e->Forms().Input.Input.length();
					RaiseEvent(FormEventType::TextChanged);
				}
			}
		}
	}
}

void TextEdit::OnRender()
{
	int xpos = Align(TextHAlign, Size.x, font->GetFontWidth(text));
	int ypos = Align(TextVAlign, Size.y, font->GetFontHeight());

	if (editting)
	{
		int cxpos = xpos + font->GetFontWidth(text.substr(0, SelectionStart)) + 1;

		if (cxpos < 0)
		{
			xpos += cxpos;
			cxpos = xpos + font->GetFontWidth(text.substr(0, SelectionStart)) + 1;
		}
		if (cxpos > Size.x)
		{
			xpos -= cxpos - Size.x;
			cxpos = xpos + font->GetFontWidth(text.substr(0, SelectionStart)) + 1;
		}

		if (caretDraw)
		{
			auto textImage = font->getString(cursor);
			fw().renderer->draw(textImage, Vec2<float>{cxpos, ypos});
		}
	}

	auto textImage = font->getString(text);
	fw().renderer->draw(textImage, Vec2<float>{xpos, ypos});
}

void TextEdit::Update()
{
	if (editting)
	{
		caretTimer = (caretTimer + 1) % TEXTEDITOR_CARET_TOGGLE_TIME;
		if (caretTimer == 0)
		{
			caretDraw = !caretDraw;
		}
	}
}

void TextEdit::UnloadResources() {}

UString TextEdit::GetText() const { return text; }

void TextEdit::SetText(const UString &Text)
{
	text = Text;
	SelectionStart = text.length();
	RaiseEvent(FormEventType::TextChanged);
}

void TextEdit::SetCursor(const UString &cursor) { this->cursor = cursor; }

void TextEdit::SetTextMaxSize(size_t length) { this->textMaxLength = length; }

void TextEdit::SetAllowedCharacters(const UString &allowedCharacters)
{
	this->allowedCharacters = allowedCharacters;
}

void TextEdit::RaiseEvent(FormEventType Type)
{
	//	std::ignore = Type;
	pushFormEvent(Type, nullptr);
	// this->pushFormEvent(FormEventType::TextChanged, nullptr);
}

sp<BitmapFont> TextEdit::GetFont() const { return font; }

void TextEdit::SetFont(sp<BitmapFont> NewFont) { font = NewFont; }

sp<Control> TextEdit::CopyTo(sp<Control> CopyParent)
{
	sp<TextEdit> copy;
	if (CopyParent)
	{
		copy = CopyParent->createChild<TextEdit>(this->text, this->font);
	}
	else
	{
		copy = mksp<TextEdit>(this->text, this->font);
	}
	copy->TextHAlign = this->TextHAlign;
	copy->TextVAlign = this->TextVAlign;
	CopyControlData(copy);
	return copy;
}

void TextEdit::ConfigureSelfFromXML(tinyxml2::XMLElement *Element)
{
	Control::ConfigureSelfFromXML(Element);
	tinyxml2::XMLElement *subnode;
	UString attribvalue;

	if (Element->Attribute("text") != nullptr)
	{
		text = tr(Element->Attribute("text"));
	}
	if (Element->FirstChildElement("font") != nullptr)
	{
		font = ui().GetFont(Element->FirstChildElement("font")->GetText());
	}
	subnode = Element->FirstChildElement("alignment");
	if (subnode != nullptr)
	{
		if (subnode->Attribute("horizontal") != nullptr)
		{
			attribvalue = subnode->Attribute("horizontal");
			if (attribvalue == "left")
			{
				TextHAlign = HorizontalAlignment::Left;
			}
			else if (attribvalue == "centre")
			{
				TextHAlign = HorizontalAlignment::Centre;
			}
			else if (attribvalue == "right")
			{
				TextHAlign = HorizontalAlignment::Right;
			}
		}
		if (subnode->Attribute("vertical") != nullptr)
		{
			attribvalue = subnode->Attribute("vertical");
			if (attribvalue == "top")
			{
				TextVAlign = VerticalAlignment::Top;
			}
			else if (attribvalue == "centre")
			{
				TextVAlign = VerticalAlignment::Centre;
			}
			else if (attribvalue == "bottom")
			{
				TextVAlign = VerticalAlignment::Bottom;
			}
		}
	}
}
}; // namespace OpenApoc

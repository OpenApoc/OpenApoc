#include "forms/textedit.h"
#include "dependencies/pugixml/src/pugixml.hpp"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/font.h"
#include "framework/framework.h"
#include "framework/image.h"
#include "framework/keycodes.h"
#include "framework/renderer.h"
#include "library/sp.h"
#include "library/strings_format.h"

namespace OpenApoc
{

TextEdit::TextEdit(const UString &Text, sp<BitmapFont> font)
    : Control(), caretDraw(false), caretTimer(0), text(Text), cursor("*"), font(font),
      editing(false), SelectionStart(Text.length()), TextHAlign(HorizontalAlignment::Left),
      TextVAlign(VerticalAlignment::Centre)
{
	if (font)
	{
		palette = font->getPalette();
	}
}

TextEdit::~TextEdit() = default;

bool TextEdit::isFocused() const { return editing; }

void TextEdit::eventOccured(Event *e)
{
	UString keyname;

	Control::eventOccured(e);

	if (e->type() == EVENT_FORM_INTERACTION)
	{
		if (e->forms().RaisedBy == shared_from_this())
		{
			if (e->forms().EventFlag == FormEventType::GotFocus ||
			    e->forms().EventFlag == FormEventType::MouseClick ||
			    e->forms().EventFlag == FormEventType::KeyDown)
			{
				editing = true;

				fw().textStartInput();
				// e->Handled = true;
				// FIXME: Should we really fall through here?
			}
		}
		if (editing)
		{
			if (e->forms().RaisedBy == shared_from_this())
			{

				if (e->forms().EventFlag == FormEventType::LostFocus)
				{
					editing = false;
					fw().textStopInput();
					raiseEvent(FormEventType::TextEditFinish);
					// e->Handled = true;
				}
			}
			else if (e->forms().EventFlag == FormEventType::MouseClick)
			{
				// FIXME: Due to event duplication (?), this code won't work. Can only stop editing
				// text by pressing enter.
				// editting = false;
				// fw().textStopInput();
				// raiseEvent(FormEventType::TextEditFinish);
			}
			if (e->forms().EventFlag == FormEventType::KeyDown)
			{
				LogInfo("Key pressed: %d", e->forms().KeyInfo.KeyCode);
				switch (e->forms().KeyInfo.KeyCode)
				{
					case SDLK_BACKSPACE:
						if (SelectionStart > 0)
						{
							text.remove(SelectionStart - 1, 1);
							SelectionStart--;
							raiseEvent(FormEventType::TextChanged);
						}
						e->Handled = true;
						break;
					case SDLK_DELETE:
						if (SelectionStart < text.length())
						{
							text.remove(SelectionStart, 1);
							raiseEvent(FormEventType::TextChanged);
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
					case SDLK_HOME:
						SelectionStart = 0;
						e->Handled = true;
						break;
					case SDLK_END:
						SelectionStart = text.length();
						e->Handled = true;
						break;
					case SDLK_ESCAPE:
						editing = false;
						fw().textStopInput();
						raiseEvent(FormEventType::TextEditCancel);
						break;
					case SDLK_RETURN:
						editing = false;
						fw().textStopInput();
						raiseEvent(FormEventType::TextEditFinish);
						break;
					case SDLK_v: // CTRL+V
						if (e->forms().KeyInfo.Modifiers & KMOD_CTRL)
						{
							UString clipboard = fw().textGetClipboard();

							if (text.length() + clipboard.length() >= this->textMaxLength)
							{
								return;
							}
							if (!clipboard.empty())
							{
								text.insert(SelectionStart, clipboard);
								SelectionStart += clipboard.length();
								raiseEvent(FormEventType::TextChanged);
							}
						}
						break;
				}
			}
			else if (e->forms().EventFlag == FormEventType::TextInput)
			{
				if (text.length() >= this->textMaxLength)
				{
					return;
				}

				UString inputCharacter = e->forms().Input.Input;
				if (allowedCharacters.empty() ||
				    allowedCharacters.str().find(inputCharacter.str()) != std::string::npos)
				{
					text.insert(SelectionStart, inputCharacter);
					SelectionStart += inputCharacter.length();
					raiseEvent(FormEventType::TextChanged);
				}
			}
		}
	}
}

void TextEdit::onRender()
{
	Control::onRender();

	int xpos = align(TextHAlign, Size.x, font->getFontWidth(text));
	int ypos = align(TextVAlign, Size.y, font->getFontHeight());

	if (editing)
	{
		int cxpos = xpos + font->getFontWidth(text.substr(0, SelectionStart)) + 1;

		if (cxpos < 0)
		{
			xpos += cxpos;
			cxpos = xpos + font->getFontWidth(text.substr(0, SelectionStart)) + 1;
		}
		if (cxpos > Size.x)
		{
			xpos -= cxpos - Size.x;
			cxpos = xpos + font->getFontWidth(text.substr(0, SelectionStart)) + 1;
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

void TextEdit::update()
{
	if (editing)
	{
		caretTimer = (caretTimer + 1) % TEXTEDITOR_CARET_TOGGLE_TIME;
		if (caretTimer == 0)
		{
			caretDraw = !caretDraw;
			this->setDirty();
		}
	}
}

void TextEdit::unloadResources() {}

UString TextEdit::getText() const { return text; }

void TextEdit::setText(const UString &Text)
{
	text = Text;
	SelectionStart = text.length();
	raiseEvent(FormEventType::TextChanged);
}

void TextEdit::setCursor(const UString &cursor)
{
	this->cursor = cursor;
	this->setDirty();
}

void TextEdit::setTextMaxSize(size_t length)
{
	this->textMaxLength = length;
	this->setDirty();
}

void TextEdit::setAllowedCharacters(const UString &allowedCharacters)
{
	this->allowedCharacters = allowedCharacters;
	this->setDirty();
}

void TextEdit::raiseEvent(FormEventType Type)
{
	//	std::ignore = Type;
	pushFormEvent(Type, nullptr);
	// this->pushFormEvent(FormEventType::TextChanged, nullptr);
}

sp<BitmapFont> TextEdit::getFont() const { return font; }

void TextEdit::setFont(sp<BitmapFont> NewFont)
{
	font = NewFont;
	this->setDirty();
}

sp<Control> TextEdit::copyTo(sp<Control> CopyParent)
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
	copyControlData(copy);
	return copy;
}

void TextEdit::configureSelfFromXml(pugi::xml_node *node)
{
	Control::configureSelfFromXml(node);

	if (node->attribute("text"))
	{
		text = tr(node->attribute("text").as_string());
	}
	auto fontNode = node->child("font");
	if (fontNode)
	{
		font = ui().getFont(fontNode.text().get());
	}
	auto alignmentNode = node->child("alignment");
	if (alignmentNode)
	{
		UString hAlign = alignmentNode.attribute("horizontal").as_string();
		if (hAlign == "left")
		{
			TextHAlign = HorizontalAlignment::Left;
		}
		else if (hAlign == "centre")
		{
			TextHAlign = HorizontalAlignment::Centre;
		}
		else if (hAlign == "right")
		{
			TextHAlign = HorizontalAlignment::Right;
		}
		UString vAlign = alignmentNode.attribute("vertical").as_string();
		if (vAlign == "top")
		{
			TextVAlign = VerticalAlignment::Top;
		}
		else if (vAlign == "centre")
		{
			TextVAlign = VerticalAlignment::Centre;
		}
		else if (vAlign == "bottom")
		{
			TextVAlign = VerticalAlignment::Bottom;
		}
	}
}
}; // namespace OpenApoc

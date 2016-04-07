#include "forms/label.h"
#include "framework/framework.h"
#include "game/resources/gamecore.h"
#include "library/sp.h"
#include <tinyxml2.h>

namespace OpenApoc
{

Label::Label(const UString &Text, sp<BitmapFont> font)
    : Control(), text(Text), font(font), TextHAlign(HorizontalAlignment::Left),
      TextVAlign(VerticalAlignment::Top), WordWrap(true)
{
	if (font)
	{
		palette = font->getPalette();
	}
}

Label::~Label() {}

void Label::EventOccured(Event *e) { Control::EventOccured(e); }

void Label::OnRender()
{
	int xpos;
	int ypos;
	std::list<UString> lines = font->WordWrapText(text, Size.x);

	switch (TextVAlign)
	{
		case VerticalAlignment::Top:
			ypos = 0;
			break;
		case VerticalAlignment::Centre:
			ypos = (Size.y / 2) - (font->GetFontHeight(text, Size.x) / 2);
			break;
		case VerticalAlignment::Bottom:
			ypos = Size.y - font->GetFontHeight(text, Size.x);
			break;
		default:
			LogError("Unknown TextVAlign");
			return;
	}

	while (lines.size() > 0)
	{
		switch (TextHAlign)
		{
			case HorizontalAlignment::Left:
				xpos = 0;
				break;
			case HorizontalAlignment::Centre:
				xpos = (Size.x / 2) - (font->GetFontWidth(lines.front()) / 2);
				break;
			case HorizontalAlignment::Right:
				xpos = Size.x - font->GetFontWidth(lines.front());
				break;
			default:
				LogError("Unknown TextHAlign");
				return;
		}

		auto textImage = font->getString(lines.front());
		fw().renderer->draw(textImage, Vec2<float>{xpos, ypos});

		lines.pop_front();
		ypos += font->GetFontHeight();
	}
}

void Label::Update()
{
	// No "updates"
}

void Label::UnloadResources() {}

UString Label::GetText() const { return text; }

void Label::SetText(const UString &Text) { text = Text; }

sp<BitmapFont> Label::GetFont() const { return font; }

void Label::SetFont(sp<BitmapFont> NewFont) { font = NewFont; }

sp<Control> Label::CopyTo(sp<Control> CopyParent)
{
	sp<Label> copy;
	if (CopyParent)
	{
		copy = CopyParent->createChild<Label>(this->text, this->font);
	}
	else
	{
		copy = mksp<Label>(this->text, this->font);
	}
	copy->TextHAlign = this->TextHAlign;
	copy->TextVAlign = this->TextVAlign;
	copy->WordWrap = this->WordWrap;
	CopyControlData(copy);
	return copy;
}

void Label::ConfigureFromXML(tinyxml2::XMLElement *Element)
{
	Control::ConfigureFromXML(Element);
	tinyxml2::XMLElement *subnode;
	UString attribvalue;

	if (Element->Attribute("text") != nullptr)
	{
		text = tr(Element->Attribute("text"));
	}
	if (Element->FirstChildElement("font") != nullptr)
	{
		font = fw().gamecore->GetFont(Element->FirstChildElement("font")->GetText());
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

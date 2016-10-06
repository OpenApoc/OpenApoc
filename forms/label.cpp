#include "forms/label.h"
#include "forms/ui.h"
#include "framework/framework.h"
#include "framework/image.h"
#include "library/sp.h"
#include "library/strings_format.h"
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

Label::~Label() = default;

void Label::eventOccured(Event *e) { Control::eventOccured(e); }

void Label::onRender()
{
	int xpos;
	int ypos;
	std::list<UString> lines = font->wordWrapText(text, Size.x);

	ypos = align(TextVAlign, Size.y, font->getFontHeight(text, Size.x));

	while (lines.size() > 0)
	{
		xpos = align(TextHAlign, Size.x, font->getFontWidth(lines.front()));

		auto textImage = font->getString(lines.front());
		fw().renderer->draw(textImage, Vec2<float>{xpos, ypos});

		lines.pop_front();
		ypos += font->getFontHeight();
	}
}

void Label::update()
{
	// No "updates"
}

void Label::unloadResources() {}

UString Label::getText() const { return text; }

void Label::setText(const UString &Text) { text = Text; }

sp<BitmapFont> Label::getFont() const { return font; }

void Label::setFont(sp<BitmapFont> NewFont) { font = NewFont; }

sp<Control> Label::copyTo(sp<Control> CopyParent)
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
	copyControlData(copy);
	return copy;
}

void Label::configureSelfFromXml(tinyxml2::XMLElement *Element)
{
	Control::configureSelfFromXml(Element);
	tinyxml2::XMLElement *subnode;
	UString attribvalue;

	if (Element->Attribute("text") != nullptr)
	{
		text = tr(Element->Attribute("text"));
	}
	if (Element->FirstChildElement("font") != nullptr)
	{
		font = ui().getFont(Element->FirstChildElement("font")->GetText());
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

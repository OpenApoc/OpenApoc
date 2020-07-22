#include "forms/label.h"
#include "dependencies/pugixml/src/pugixml.hpp"
#include "forms/ui.h"
#include "framework/font.h"
#include "framework/framework.h"
#include "framework/image.h"
#include "framework/renderer.h"
#include "library/sp.h"
#include "library/strings_format.h"

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
	Control::onRender();

	int xpos;
	int ypos;
	std::list<UString> lines = font->wordWrapText(text, Size.x);

	ypos = align(TextVAlign, Size.y, font->getFontHeight(text, Size.x));

	while (lines.size() > 0)
	{
		xpos = align(TextHAlign, Size.x, font->getFontWidth(lines.front()));

		auto textImage = font->getString(lines.front());
		fw().renderer->drawTinted(textImage, Vec2<float>{xpos, ypos}, Tint);

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

void Label::setText(const UString &Text)
{
	if (text == Text)
		return;
	text = Text;
	this->setDirty();
}

sp<BitmapFont> Label::getFont() const { return font; }

void Label::setFont(sp<BitmapFont> NewFont)
{
	if (font == NewFont)
		return;
	font = NewFont;
	this->setDirty();
}

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
	copy->Tint = this->Tint;
	copy->TextHAlign = this->TextHAlign;
	copy->TextVAlign = this->TextVAlign;
	copy->WordWrap = this->WordWrap;
	copyControlData(copy);
	return copy;
}

void Label::configureSelfFromXml(pugi::xml_node *node)
{
	Control::configureSelfFromXml(node);
	text = tr(node->attribute("text").as_string());

	UString tintAttribute = node->attribute("tint").as_string();
	if (!tintAttribute.empty())
	{
		if (*tintAttribute.begin() == '#')
			Tint = Colour::FromHex(tintAttribute);
		else
			Tint = Colour::FromHtmlName(tintAttribute);
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

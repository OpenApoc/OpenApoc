#include "forms/label.h"
#include "dependencies/pugixml/src/pugixml.hpp"
#include "forms/scrollbar.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/font.h"
#include "framework/framework.h"
#include "framework/image.h"
#include "framework/renderer.h"
#include "library/sp.h"
#include "library/strings_format.h"

namespace OpenApoc
{

Label::Label(const UString &Text, sp<BitmapFont> font)
    : Control(), text(Text), font(font), scrollOffset(0), TextHAlign(HorizontalAlignment::Left),
      TextVAlign(VerticalAlignment::Top), WordWrap(true)
{
	if (font)
	{
		palette = font->getPalette();
	}
}

Label::~Label() = default;

void Label::eventOccured(Event *e)
{
	Control::eventOccured(e);
	if (scroller && e->type() == EVENT_FORM_INTERACTION &&
	    e->forms().EventFlag == FormEventType::MouseMove &&
	    e->forms().RaisedBy == shared_from_this())
	{
		scroller->scrollWheel(e);
	}
}

void Label::onRender()
{
	Control::onRender();

	std::list<UString> lines = font->wordWrapText(text, Size.x);

	int ysize = font->getFontHeight(text, Size.x);
	if (scroller)
	{
		scroller->setVisible(ysize > this->Size.y && lines.size() > 1);
		scroller->setMaximum(ysize - this->Size.y);
	}

	int ypos = align(TextVAlign, Size.y, ysize);
	if (scroller && scroller->isVisible())
	{
		ypos = -scrollOffset;
	}

	for (const auto &line : lines)
	{
		int xpos = align(TextHAlign, Size.x, font->getFontWidth(line));

		auto textImage = font->getString(line);
		fw().renderer->drawTinted(textImage, Vec2<float>{xpos, ypos}, Tint);

		ypos += font->getFontHeight();
	}
}

void Label::update()
{
	Control::update();

	if (scroller && scroller->getValue() != this->scrollOffset)
	{
		this->scrollOffset = scroller->getValue();
		this->setDirty();
	}
}

void Label::unloadResources() {}

UString Label::getText() const { return text; }

void Label::setText(const UString &Text)
{
	if (text == Text)
		return;

	text = Text;

	if (scroller)
		scroller->setValue(0);

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
	sp<ScrollBar> scrollCopy;
	if (scroller)
	{
		scrollCopy = std::dynamic_pointer_cast<ScrollBar>(scroller->lastCopiedTo.lock());
	}

	if (CopyParent)
	{
		copy = CopyParent->createChild<Label>(this->text, this->font);
	}
	else
	{
		copy = mksp<Label>(this->text, this->font);
	}
	copy->scroller = scrollCopy;
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

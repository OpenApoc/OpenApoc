#include "forms/ticker.h"
#include "forms/ui.h"
#include "framework/framework.h"
#include "library/sp.h"
#include <tinyxml2.h>

namespace OpenApoc
{

Ticker::Ticker(sp<BitmapFont> font)
    : Control(), animating(false), animTimer(0), displayTimer(0), font(font),
      TextHAlign(HorizontalAlignment::Left), TextVAlign(VerticalAlignment::Top)
{
	if (font)
	{
		palette = font->getPalette();
	}
}

Ticker::~Ticker() = default;

void Ticker::EventOccured(Event *e) { Control::EventOccured(e); }

void Ticker::OnRender()
{
	int xpos;
	int ypos;
	if (!animating)
	{
		xpos = Align(TextHAlign, Size.x, font->GetFontWidth(text));
		ypos = 0;

		auto textImage = font->getString(text);
		fw().renderer->draw(textImage, Vec2<float>{xpos, ypos});
	}
	else
	{
		UString out = text;
		xpos = Align(TextHAlign, Size.x, font->GetFontWidth(out));
		ypos = 0 - animTimer / 4;
		auto outImage = font->getString(out);
		fw().renderer->draw(outImage, Vec2<float>{xpos, ypos});

		UString in = messages.front();
		xpos = Align(TextHAlign, Size.x, font->GetFontWidth(in));
		ypos = 15 - animTimer / 4;
		auto inImage = font->getString(in);
		fw().renderer->draw(inImage, Vec2<float>{xpos, ypos});
	}
}

void Ticker::Update()
{
	if (text.empty() && messages.empty())
		return;

	if (animating)
	{
		animTimer++;
		if (animTimer >= ANIM_TICKS)
		{
			animTimer = 0;
			animating = false;
			if (messages.empty())
			{
				text = "";
			}
			else
			{
				text = messages.front();
				messages.pop();
			}
		}
	}
	else
	{
		displayTimer++;
		if ((messages.empty() && displayTimer >= DISPLAY_TICKS) ||
		    (!messages.empty() && (displayTimer >= ANIM_TICKS * 2 || text.empty())))
		{
			displayTimer = 0;
			animating = true;
		}
	}
}

void Ticker::UnloadResources() {}

void Ticker::AddMessage(const UString &Text) { messages.emplace(Text); }

sp<BitmapFont> Ticker::GetFont() const { return font; }

void Ticker::SetFont(sp<BitmapFont> NewFont) { font = NewFont; }

sp<Control> Ticker::CopyTo(sp<Control> CopyParent)
{
	sp<Ticker> copy;
	if (CopyParent)
	{
		copy = CopyParent->createChild<Ticker>(this->font);
	}
	else
	{
		copy = mksp<Ticker>(this->font);
	}
	copy->TextHAlign = this->TextHAlign;
	copy->TextVAlign = this->TextVAlign;
	CopyControlData(copy);
	return copy;
}

void Ticker::ConfigureSelfFromXML(tinyxml2::XMLElement *Element)
{
	Control::ConfigureSelfFromXML(Element);
	tinyxml2::XMLElement *subnode;
	UString attribvalue;

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

#include "forms/textbutton.h"
#include "dependencies/pugixml/src/pugixml.hpp"
#include "forms/label.h"
#include "forms/ui.h"
#include "framework/data.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/image.h"
#include "framework/renderer.h"
#include "framework/sound.h"
#include "library/sp.h"
#include "library/strings_format.h"

namespace OpenApoc
{

TextButton::TextButton(const UString &Text, sp<BitmapFont> font)
    : Control(), buttonclick(fw().data->loadSample(
                     "RAWSOUND:xcom3/rawsound/strategc/intrface/button1.raw:22050")),
      buttonbackground(fw().data->loadImage("ui/textbuttonback.png")),
      TextHAlign(HorizontalAlignment::Centre), TextVAlign(VerticalAlignment::Centre),
      RenderStyle(ButtonRenderStyle::Menu)
{
	isClickable = true;
	label = mksp<Label>(Text, font);
}

TextButton::~TextButton() = default;

void TextButton::eventOccured(Event *e)
{
	Control::eventOccured(e);

	if (e->type() == EVENT_FORM_INTERACTION && e->forms().RaisedBy == shared_from_this() &&
	    e->forms().EventFlag == FormEventType::MouseDown)
	{
		fw().soundBackend->playSample(buttonclick);
	}

	if (e->type() == EVENT_FORM_INTERACTION && e->forms().RaisedBy == shared_from_this() &&
	    e->forms().EventFlag == FormEventType::MouseClick)
	{
		this->pushFormEvent(FormEventType::ButtonClick, e);
	}
}

void TextButton::onRender()
{
	Control::onRender();

	if (label->getParent() == nullptr)
	{
		label->setParent(shared_from_this());
		label->Size = this->Size;
		label->TextHAlign = this->TextHAlign;
		label->TextVAlign = this->TextVAlign;
	}

	if (cached == nullptr || cached->size != Vec2<unsigned int>{Size})
	{
		cached.reset(new Surface{Vec2<unsigned int>{Size}});

		RendererSurfaceBinding b(*fw().renderer, cached);
		fw().renderer->clear();

		switch (RenderStyle)
		{
			case ButtonRenderStyle::Flat:
				fw().renderer->drawFilledRect(Vec2<float>{0, 0}, Size, BackgroundColour);
				break;
			case ButtonRenderStyle::Bevel:
				fw().renderer->drawFilledRect(Vec2<float>{0, 0}, Size, Colour{112, 112, 112});
				fw().renderer->drawRect(Vec2<float>{0, 0}, Size, Colour{16, 16, 16});
				fw().renderer->drawRect(Vec2<float>{2, 2}, Size - 2, Colour{64, 64, 64});
				fw().renderer->drawRect(Vec2<float>{1, 1}, Size - 2, Colour{212, 212, 212});
				break;
			case ButtonRenderStyle::Menu:
				fw().renderer->drawScaled(buttonbackground, Vec2<float>{0, 0}, Size);
				fw().renderer->drawRect(Vec2<float>{2, 2}, Size - 4, Colour{48, 48, 48});
				fw().renderer->drawFilledRect(Vec2<float>{3, 3}, Size - 6, Colour{160, 160, 160});
				fw().renderer->drawLine(Vec2<float>{3, 3}, Vec2<float>{Size.x - 3, 3},
				                        Colour{220, 220, 220});
				fw().renderer->drawLine(Vec2<float>{3, Size.y - 5},
				                        Vec2<float>{Size.x - 3, Size.y - 5}, Colour{100, 100, 100});
				fw().renderer->drawLine(Vec2<float>{3, Size.y - 4},
				                        Vec2<float>{Size.x - 3, Size.y - 4}, Colour{64, 64, 64});
				break;
		}
	}
	fw().renderer->draw(cached, Vec2<float>{0, 0});

	if (mouseDepressed && mouseInside)
	{
		switch (RenderStyle)
		{
			case ButtonRenderStyle::Flat:
				fw().renderer->drawFilledRect(Vec2<float>{0, 0}, Vec2<float>{Size.x, Size.y},
				                              Colour{255, 255, 255});
				break;
			case ButtonRenderStyle::Bevel:
				fw().renderer->drawRect(Vec2<float>{0, 0}, Vec2<float>{Size.x, Size.y},
				                        Colour{16, 16, 16}, 2.0f);
				fw().renderer->drawRect(Vec2<float>{3, 3}, Vec2<float>{Size.x - 4, Size.y - 4},
				                        Colour{64, 64, 64});
				fw().renderer->drawRect(Vec2<float>{2, 2}, Vec2<float>{Size.x - 4, Size.y - 4},
				                        Colour{212, 212, 212});
				break;
			case ButtonRenderStyle::Menu:
				fw().renderer->drawRect(Vec2<float>{0, 0}, Vec2<float>{Size.x, Size.y},
				                        Colour{255, 255, 255}, 2);
				break;
		}
	}
}

void TextButton::update()
{
	// No "updates"
}

void TextButton::unloadResources() {}

UString TextButton::getText() const { return label->getText(); }

void TextButton::setText(const UString &Text) { label->setText(Text); }

sp<BitmapFont> TextButton::getFont() const { return label->getFont(); }

void TextButton::setFont(sp<BitmapFont> NewFont) { label->setFont(NewFont); }

sp<Control> TextButton::copyTo(sp<Control> CopyParent)
{
	sp<TextButton> copy;
	if (CopyParent)
	{
		copy = CopyParent->createChild<TextButton>(label->getText(), label->getFont());
	}
	else
	{
		copy = mksp<TextButton>(label->getText(), label->getFont());
	}
	copy->TextHAlign = this->TextHAlign;
	copy->TextVAlign = this->TextVAlign;
	copy->RenderStyle = this->RenderStyle;
	copyControlData(copy);
	return copy;
}

void TextButton::configureSelfFromXml(pugi::xml_node *node)
{
	Control::configureSelfFromXml(node);

	if (node->attribute("text"))
	{
		label->setText(tr(node->attribute("text").as_string()));
	}
	auto fontNode = node->child("font");
	if (fontNode)
	{
		label->setFont(ui().getFont(fontNode.text().get()));
	}
}
}; // namespace OpenApoc

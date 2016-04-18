#include "forms/textbutton.h"
#include "forms/label.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "library/sp.h"
#include <tinyxml2.h>

namespace OpenApoc
{

TextButton::TextButton(const UString &Text, sp<BitmapFont> font)
    : Control(), buttonclick(fw().data->load_sample(
                     "RAWSOUND:xcom3/RAWSOUND/STRATEGC/INTRFACE/BUTTON1.RAW:22050")),
      buttonbackground(fw().data->load_image("UI/TEXTBUTTONBACK.PNG")),
      TextHAlign(HorizontalAlignment::Centre), TextVAlign(VerticalAlignment::Centre),
      RenderStyle(ButtonRenderStyle::Menu)
{
	label = mksp<Label>(Text, font);
}

TextButton::~TextButton() {}

void TextButton::EventOccured(Event *e)
{
	Control::EventOccured(e);

	if (e->Type() == EVENT_FORM_INTERACTION && e->Forms().RaisedBy == shared_from_this() &&
	    e->Forms().EventFlag == FormEventType::MouseDown)
	{
		fw().soundBackend->playSample(buttonclick);
	}

	if (e->Type() == EVENT_FORM_INTERACTION && e->Forms().RaisedBy == shared_from_this() &&
	    e->Forms().EventFlag == FormEventType::MouseClick)
	{
		this->pushFormEvent(FormEventType::ButtonClick, e);
	}
}

void TextButton::OnRender()
{
	if (label->GetParent() == nullptr)
	{
		label->SetParent(shared_from_this());
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

void TextButton::Update()
{
	// No "updates"
}

void TextButton::UnloadResources() {}

UString TextButton::GetText() const { return label->GetText(); }

void TextButton::SetText(const UString &Text) { label->SetText(Text); }

sp<BitmapFont> TextButton::GetFont() const { return label->GetFont(); }

void TextButton::SetFont(sp<BitmapFont> NewFont) { label->SetFont(NewFont); }

sp<Control> TextButton::CopyTo(sp<Control> CopyParent)
{
	sp<TextButton> copy;
	if (CopyParent)
	{
		copy = CopyParent->createChild<TextButton>(label->GetText(), label->GetFont());
	}
	else
	{
		copy = mksp<TextButton>(label->GetText(), label->GetFont());
	}
	copy->TextHAlign = this->TextHAlign;
	copy->TextVAlign = this->TextVAlign;
	copy->RenderStyle = this->RenderStyle;
	CopyControlData(copy);
	return copy;
}

void TextButton::ConfigureFromXML(tinyxml2::XMLElement *Element)
{
	Control::ConfigureFromXML(Element);

	if (Element->Attribute("text") != nullptr)
	{
		label->SetText(tr(Element->Attribute("text")));
	}
	if (Element->FirstChildElement("font") != nullptr)
	{
		label->SetFont(ui().GetFont(Element->FirstChildElement("font")->GetText()));
	}
}
}; // namespace OpenApoc

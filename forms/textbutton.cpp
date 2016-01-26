#include "library/sp.h"

#include "forms/textbutton.h"
#include "framework/framework.h"
#include "game/resources/gamecore.h"
#include "forms/label.h"

namespace OpenApoc
{

TextButton::TextButton(UString Text, sp<BitmapFont> font)
    : Control(), cached(nullptr),
      buttonclick(
          fw().data->load_sample("RAWSOUND:xcom3/RAWSOUND/STRATEGC/INTRFACE/BUTTON1.RAW:22050")),
      buttonbackground(fw().data->load_image("UI/TEXTBUTTONBACK.PNG")),
      TextHAlign(HorizontalAlignment::Centre), TextVAlign(VerticalAlignment::Centre),
      RenderStyle(TextButtonRenderStyles::MenuButtonStyle)
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

	if (cached == nullptr || cached->size != Vec2<unsigned int>{Size.x, Size.y})
	{
		cached.reset(new Surface{Vec2<unsigned int>{Size.x, Size.y}});

		RendererSurfaceBinding b(*fw().renderer, cached);
		fw().renderer->clear();

		switch (RenderStyle)
		{
			case TextButtonRenderStyles::SolidButtonStyle:
				fw().renderer->drawFilledRect(Vec2<float>{0, 0}, Vec2<float>{Size.x, Size.y},
				                              BackgroundColour);
				break;
			case TextButtonRenderStyles::MenuButtonStyle:
				fw().renderer->drawScaled(buttonbackground, Vec2<float>{0, 0},
				                          Vec2<float>{Size.x, Size.y});
				fw().renderer->drawRect(Vec2<float>{2, 2}, Vec2<float>{Size.x - 4, Size.y - 4},
				                        Colour{48, 48, 48});
				fw().renderer->drawFilledRect(
				    Vec2<float>{3, 3}, Vec2<float>{Size.x - 6, Size.y - 6}, Colour{160, 160, 160});
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
			case TextButtonRenderStyles::SolidButtonStyle:
				fw().renderer->drawFilledRect(Vec2<float>{0, 0}, Vec2<float>{Size.x, Size.y},
				                              Colour{255, 255, 255});
				break;
			case TextButtonRenderStyles::MenuButtonStyle:
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

void TextButton::SetText(UString Text) { label->SetText(Text); }

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
		label->SetFont(fw().gamecore->GetFont(Element->FirstChildElement("font")->GetText()));
	}
}
}; // namespace OpenApoc

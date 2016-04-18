#include "forms/graphicbutton.h"
#include "forms/scrollbar.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "library/sp.h"
#include <tinyxml2.h>

namespace OpenApoc
{

GraphicButton::GraphicButton(sp<Image> image, sp<Image> imageDepressed, sp<Image> imageHover)
    : Control(), image(image), imagedepressed(imageDepressed), imagehover(imageHover),
      buttonclick(
          fw().data->load_sample("RAWSOUND:xcom3/RAWSOUND/STRATEGC/INTRFACE/BUTTON1.RAW:22050"))
{
}

GraphicButton::~GraphicButton() {}

void GraphicButton::EventOccured(Event *e)
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

		if (ScrollBarPrev != nullptr)
		{
			ScrollBarPrev->ScrollPrev();
		}

		if (ScrollBarNext != nullptr)
		{
			ScrollBarNext->ScrollNext();
		}
	}
}

void GraphicButton::OnRender()
{
	sp<Image> useimage;

	if (image)
	{
		if (Size.x == 0)
		{
			Size.x = image->size.x;
		}
		if (Size.y == 0)
		{
			Size.y = image->size.y;
		}
	}

	useimage = image;
	if (mouseDepressed)
	{
		useimage = imagedepressed;
	}
	else if (mouseInside && imagehover != nullptr)
	{
		useimage = imagehover;
	}

	if (useimage != nullptr)
	{
		if (Vec2<unsigned int>{Size.x, Size.y} != useimage->size)
		{
			fw().renderer->draw(useimage, Vec2<float>{0, 0});
		}
		else
		{
			fw().renderer->drawScaled(useimage, Vec2<float>{0, 0},
			                          Vec2<float>{this->Size.x, this->Size.y});
		}
	}
}

void GraphicButton::Update() { Control::Update(); }

void GraphicButton::UnloadResources()
{
	image.reset();
	imagedepressed.reset();
	imagehover.reset();
	Control::UnloadResources();
}

sp<Image> GraphicButton::GetImage() const { return image; }

void GraphicButton::SetImage(sp<Image> Image) { image = Image; }

sp<Image> GraphicButton::GetDepressedImage() const { return imagedepressed; }

void GraphicButton::SetDepressedImage(sp<Image> Image) { imagedepressed = Image; }

sp<Image> GraphicButton::GetHoverImage() const { return imagehover; }

void GraphicButton::SetHoverImage(sp<Image> Image) { imagehover = Image; }

sp<Control> GraphicButton::CopyTo(sp<Control> CopyParent)
{
	sp<GraphicButton> copy;
	if (CopyParent)
	{
		copy = CopyParent->createChild<GraphicButton>(this->image, this->imagedepressed,
		                                              this->imagehover);
	}
	else
	{
		copy = mksp<GraphicButton>(this->image, this->imagedepressed, this->imagehover);
	}
	if (this->ScrollBarPrev)
	{
		copy->ScrollBarPrev =
		    std::dynamic_pointer_cast<ScrollBar>(ScrollBarPrev->lastCopiedTo.lock());
	}
	if (this->ScrollBarNext)
	{
		copy->ScrollBarNext =
		    std::dynamic_pointer_cast<ScrollBar>(ScrollBarNext->lastCopiedTo.lock());
	}
	CopyControlData(copy);
	return copy;
}

void GraphicButton::ConfigureFromXML(tinyxml2::XMLElement *Element)
{
	Control::ConfigureFromXML(Element);
	if (Element->FirstChildElement("image") != nullptr)
	{
		image = fw().data->load_image(Element->FirstChildElement("image")->GetText());
	}
	if (Element->FirstChildElement("imagedepressed") != nullptr)
	{
		imagedepressed =
		    fw().data->load_image(Element->FirstChildElement("imagedepressed")->GetText());
	}
	if (Element->FirstChildElement("imagehover") != nullptr)
	{
		imagehover = fw().data->load_image(Element->FirstChildElement("imagehover")->GetText());
	}
}
}; // namespace OpenApoc

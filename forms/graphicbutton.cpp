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
          fw().data->loadSample("RAWSOUND:xcom3/RAWSOUND/STRATEGC/INTRFACE/BUTTON1.RAW:22050"))
{
}

GraphicButton::~GraphicButton() = default;

void GraphicButton::eventOccured(Event *e)
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

		if (ScrollBarPrev != nullptr)
		{
			ScrollBarPrev->scrollPrev();
		}

		if (ScrollBarNext != nullptr)
		{
			ScrollBarNext->scrollNext();
		}
	}
}

void GraphicButton::onRender()
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

void GraphicButton::update() { Control::update(); }

void GraphicButton::unloadResources()
{
	image.reset();
	imagedepressed.reset();
	imagehover.reset();
	Control::unloadResources();
}

sp<Image> GraphicButton::getImage() const { return image; }

void GraphicButton::setImage(sp<Image> Image) { image = Image; }

sp<Image> GraphicButton::getDepressedImage() const { return imagedepressed; }

void GraphicButton::setDepressedImage(sp<Image> Image) { imagedepressed = Image; }

sp<Image> GraphicButton::getHoverImage() const { return imagehover; }

void GraphicButton::setHoverImage(sp<Image> Image) { imagehover = Image; }

sp<Control> GraphicButton::copyTo(sp<Control> CopyParent)
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
	copyControlData(copy);
	return copy;
}

void GraphicButton::configureSelfFromXml(tinyxml2::XMLElement *Element)
{
	Control::configureSelfFromXml(Element);
	if (Element->FirstChildElement("image") != nullptr)
	{
		image = fw().data->loadImage(Element->FirstChildElement("image")->GetText());
	}
	if (Element->FirstChildElement("imagedepressed") != nullptr)
	{
		imagedepressed =
		    fw().data->loadImage(Element->FirstChildElement("imagedepressed")->GetText());
	}
	if (Element->FirstChildElement("imagehover") != nullptr)
	{
		imagehover = fw().data->loadImage(Element->FirstChildElement("imagehover")->GetText());
	}
}
}; // namespace OpenApoc

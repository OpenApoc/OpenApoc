#include "forms/graphicbutton.h"
#include "dependencies/pugixml/src/pugixml.hpp"
#include "forms/scrollbar.h"
#include "framework/data.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/image.h"
#include "framework/renderer.h"
#include "framework/sound.h"
#include "library/sp.h"

namespace OpenApoc
{

GraphicButton::GraphicButton(sp<Image> image, sp<Image> imageDepressed, sp<Image> imageHover)
    : Control(), image(image), imagedepressed(imageDepressed), imagehover(imageHover),
      buttonclick(
          fw().data->loadSample("RAWSOUND:xcom3/rawsound/strategc/intrface/button1.raw:22050"))
{
	isClickable = true;
}

GraphicButton::~GraphicButton() = default;

bool GraphicButton::click()
{
	if (!Control::click())
	{
		return false;
	}
	if (buttonclick)
	{
		fw().soundBackend->playSample(buttonclick);
	}
	return true;
}

void GraphicButton::eventOccured(Event *e)
{
	Control::eventOccured(e);

	if (e->type() == EVENT_FORM_INTERACTION && e->forms().RaisedBy == shared_from_this() &&
	    e->forms().EventFlag == FormEventType::MouseDown)
	{
		if (buttonclick)
		{
			fw().soundBackend->playSample(buttonclick);
		}
	}

	if (e->type() == EVENT_FORM_INTERACTION && e->forms().RaisedBy == shared_from_this() &&
	    e->forms().EventFlag == FormEventType::MouseClick)
	{
		this->pushFormEvent(FormEventType::ButtonClick, e);

		if (ScrollBarPrev != nullptr)
		{
			ScrollBarPrev->scrollPrev(!scrollLarge);
		}

		if (ScrollBarNext != nullptr)
		{
			ScrollBarNext->scrollNext(!scrollLarge);
		}
	}
}

void GraphicButton::onRender()
{
	Control::onRender();

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

sp<Sample> GraphicButton::getClickSound() const { return buttonclick; }

void GraphicButton::setClickSound(sp<Sample> sample) { buttonclick = sample; }

sp<Image> GraphicButton::getImage() const { return image; }

void GraphicButton::setImage(sp<Image> Image)
{
	image = Image;
	this->setDirty();
}

sp<Image> GraphicButton::getDepressedImage() const { return imagedepressed; }

void GraphicButton::setDepressedImage(sp<Image> Image)
{
	imagedepressed = Image;
	this->setDirty();
}

sp<Image> GraphicButton::getHoverImage() const { return imagehover; }

void GraphicButton::setHoverImage(sp<Image> Image)
{
	imagehover = Image;
	this->setDirty();
}

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
	copy->scrollLarge = scrollLarge;
	copyControlData(copy);
	return copy;
}

void GraphicButton::configureSelfFromXml(pugi::xml_node *node)
{
	Control::configureSelfFromXml(node);

	if (auto scrollLarge = node->attribute("scrolllarge"))
	{
		this->scrollLarge = scrollLarge.as_bool();
	}
	auto imageNode = node->child("image");
	if (imageNode)
	{
		image = fw().data->loadImage(imageNode.text().get());
	}
	auto imageDepressedNode = node->child("imagedepressed");
	if (imageDepressedNode)
	{
		imagedepressed = fw().data->loadImage(imageDepressedNode.text().get());
	}
	auto imageHoverNode = node->child("imagehover");
	if (imageHoverNode)
	{
		imagehover = fw().data->loadImage(imageHoverNode.text().get());
	}
}
}; // namespace OpenApoc

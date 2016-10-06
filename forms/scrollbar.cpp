#include "forms/scrollbar.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/image.h"
#include <tinyxml2.h>

namespace OpenApoc
{

ScrollBar::ScrollBar()
    : Control(), capture(false), grippersize(1), segmentsize(1),
      gripperbutton(fw().data->loadImage(
          "PCK:xcom3/ufodata/newbut.pck:xcom3/ufodata/newbut.tab:4:ui/menuopt.pal")),
      buttonerror(fw().data->loadSample("RAWSOUND:xcom3/rawsound/extra/textbeep.raw:22050")),
      Value(0), BarOrientation(Orientation::Vertical), RenderStyle(ScrollBarRenderStyle::Menu),
      GripperColour(220, 192, 192), Minimum(0), Maximum(10), LargeChange(2)
{
	// LoadResources();
}

ScrollBar::~ScrollBar() = default;

void ScrollBar::loadResources() {}

bool ScrollBar::setValue(int newValue)
{
	newValue = std::max(newValue, Minimum);
	newValue = std::min(newValue, Maximum);
	if (newValue == Value)
		return false;

	this->pushFormEvent(FormEventType::ScrollBarChange, nullptr);
	Value = newValue;
	return true;
}

void ScrollBar::scrollPrev()
{
	if (!setValue(Value - LargeChange))
	{
		fw().soundBackend->playSample(buttonerror);
	}
}

void ScrollBar::scrollNext()
{
	if (!setValue(Value + LargeChange))
	{
		fw().soundBackend->playSample(buttonerror);
	}
}

void ScrollBar::eventOccured(Event *e)
{
	Control::eventOccured(e);

	int mousePosition = 0;
	if (e->type() == EVENT_FORM_INTERACTION)
	{
		switch (BarOrientation)
		{
			case Orientation::Vertical:
				mousePosition = e->forms().MouseInfo.Y;
				break;
			case Orientation::Horizontal:
				mousePosition = e->forms().MouseInfo.X;
				break;
		}
	}

	if (e->type() == EVENT_FORM_INTERACTION && e->forms().RaisedBy == shared_from_this() &&
	    e->forms().EventFlag == FormEventType::MouseDown)
	{
		if (mousePosition >= (segmentsize * (Value - Minimum)) + grippersize)
		{
			scrollNext();
		}
		else if (mousePosition <= segmentsize * (Value - Minimum))
		{
			scrollPrev();
		}
		else
		{
			capture = true;
		}
	}

	if (e->type() == EVENT_FORM_INTERACTION &&
	    (capture || e->forms().RaisedBy == shared_from_this()) &&
	    e->forms().EventFlag == FormEventType::MouseUp)
	{
		capture = false;
	}

	if (e->type() == EVENT_FORM_INTERACTION && e->forms().RaisedBy == shared_from_this() &&
	    e->forms().EventFlag == FormEventType::MouseMove && capture)
	{
		this->setValue(static_cast<int>(mousePosition / segmentsize));
	}
}

void ScrollBar::onRender()
{
	// LoadResources();
	if (Minimum == Maximum)
		return;

	int pos = static_cast<int>(segmentsize * (Value - Minimum));
	Vec2<float> newpos, newsize;
	switch (BarOrientation)
	{
		case Orientation::Vertical:
			newpos = {0, pos};
			newsize = {Size.x, grippersize};
			break;
		case Orientation::Horizontal:
			newpos = {pos, 0};
			newsize = {grippersize, Size.y};
			break;
	}

	switch (RenderStyle)
	{
		case ScrollBarRenderStyle::Flat:
			fw().renderer->drawFilledRect(newpos, newsize, GripperColour);
			break;
		case ScrollBarRenderStyle::Menu:
			fw().renderer->draw(gripperbutton, newpos);
			break;
	}
}

void ScrollBar::update()
{
	Control::update();

	int size;
	if (Size.x < Size.y)
	{
		BarOrientation = Orientation::Vertical;
		size = Size.y;
	}
	else
	{
		BarOrientation = Orientation::Horizontal;
		size = Size.x;
	}

	int segments = (Maximum - Minimum) + 1;
	segmentsize = size / static_cast<float>(segments);
	grippersize = segmentsize;
	if (grippersize < 16.0f)
	{
		grippersize = 16.0f;
		segmentsize = (size - grippersize) / static_cast<float>(segments - 1);
	}
	if (RenderStyle == ScrollBarRenderStyle::Menu)
	{
		grippersize = gripperbutton->size.x;
		segmentsize = (size - grippersize) / static_cast<float>(segments - 1);
	}
}

void ScrollBar::unloadResources() { Control::unloadResources(); }

sp<Control> ScrollBar::copyTo(sp<Control> CopyParent)
{
	sp<ScrollBar> copy;
	if (CopyParent)
	{
		copy = CopyParent->createChild<ScrollBar>();
	}
	else
	{
		copy = mksp<ScrollBar>();
	}
	copy->Value = this->Value;
	copy->Maximum = this->Maximum;
	copy->Minimum = this->Minimum;
	copy->GripperColour = this->GripperColour;
	copy->LargeChange = this->LargeChange;
	copy->RenderStyle = this->RenderStyle;
	copyControlData(copy);
	return copy;
}

void ScrollBar::configureSelfFromXml(tinyxml2::XMLElement *Element)
{
	Control::configureSelfFromXml(Element);
	tinyxml2::XMLElement *subnode;
	UString attribvalue;

	subnode = Element->FirstChildElement("grippercolour");
	if (subnode != nullptr)
	{
		if (subnode->Attribute("a") != nullptr && UString(subnode->Attribute("a")) != "")
		{
			GripperColour = Colour(
			    Strings::toU8(subnode->Attribute("r")), Strings::toU8(subnode->Attribute("g")),
			    Strings::toU8(subnode->Attribute("b")), Strings::toU8(subnode->Attribute("a")));
		}
		else
		{
			GripperColour = Colour(Strings::toU8(subnode->Attribute("r")),
			                       Strings::toU8(subnode->Attribute("g")),
			                       Strings::toU8(subnode->Attribute("b")));
		}
	}
	subnode = Element->FirstChildElement("range");
	if (subnode != nullptr)
	{
		if (subnode->Attribute("min") != nullptr && UString(subnode->Attribute("min")) != "")
		{
			Minimum = Strings::toInteger(subnode->Attribute("min"));
		}
		if (subnode->Attribute("max") != nullptr && UString(subnode->Attribute("max")) != "")
		{
			Maximum = Strings::toInteger(subnode->Attribute("max"));
		}
	}
}
}; // namespace OpenApoc

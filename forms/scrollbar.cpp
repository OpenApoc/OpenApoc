#include "forms/scrollbar.h"
#include "dependencies/pugixml/src/pugixml.hpp"
#include "framework/configfile.h"
#include "framework/data.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/image.h"
#include "framework/renderer.h"
#include "framework/sound.h"

namespace OpenApoc
{

ScrollBar::ScrollBar(sp<Image> gripperImage)
    : Control(), capture(false), grippersize(1), segmentsize(1), gripperbutton(gripperImage),
      buttonerror(fw().data->loadSample("RAWSOUND:xcom3/rawsound/extra/textbeep.raw:22050")),
      Value(0), BarOrientation(Orientation::Vertical), Minimum(0), Maximum(10),
      RenderStyle(ScrollBarRenderStyle::Menu), GripperColour(220, 192, 192), ScrollChange(1),
      ScrollPercent(10)
{
	isClickable = true;
	if (!gripperbutton)
		gripperbutton = fw().data->loadImage(
		    "PCK:xcom3/ufodata/newbut.pck:xcom3/ufodata/newbut.tab:4:ui/menuopt.pal");
	// loadResources();
}

ScrollBar::~ScrollBar() = default;

void ScrollBar::loadResources() {}

bool ScrollBar::setValue(int newValue)
{
	newValue = std::max(newValue, Minimum);
	newValue = std::min(newValue, Maximum);
	if (newValue == Value)
	{
		return false;
	}

	Value = newValue;
	this->pushFormEvent(FormEventType::ScrollBarChange, nullptr);
	setDirty();
	return true;
}

bool ScrollBar::setMinimum(int newMininum)
{
	if (Minimum == newMininum)
	{
		return false;
	}
	Minimum = newMininum;
	setValue(Value);
	updateScrollChangeValue();
	setDirty();
	return true;
}

bool ScrollBar::setMaximum(int newMaximum)
{
	if (Maximum == newMaximum)
	{
		return false;
	}
	Maximum = newMaximum;
	setValue(Value);
	updateScrollChangeValue();
	setDirty();
	return true;
}

void ScrollBar::scrollPrev(int amount)
{
	if (!setValue(Value - (amount != 0 ? amount : ScrollChange)))
	{
		if (!config().getBool("OpenApoc.NewFeature.NoScrollSounds"))
		{
			fw().soundBackend->playSample(buttonerror);
		}
	}
}

void ScrollBar::scrollNext(int amount)
{
	if (!setValue(Value + (amount != 0 ? amount : ScrollChange)))
	{
		if (!config().getBool("OpenApoc.NewFeature.NoScrollSounds"))
		{
			fw().soundBackend->playSample(buttonerror);
		}
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
				// MouseInfo.X/Y is relative to the control that raised it
				// make it relative to this control instead
				mousePosition = e->forms().MouseInfo.Y +
				                e->forms().RaisedBy->getLocationOnScreen().y - resolvedLocation.y;
				break;
			case Orientation::Horizontal:
				mousePosition = e->forms().MouseInfo.X +
				                e->forms().RaisedBy->getLocationOnScreen().x - resolvedLocation.x;
				break;
		}

		int pos = static_cast<int>(segmentsize * (Value - Minimum));
		if (e->forms().RaisedBy == shared_from_this() &&
		    e->forms().EventFlag == FormEventType::MouseDown)
		{
			if (mousePosition >= pos + grippersize)
			{
				scrollNext();
			}
			else if (mousePosition <= pos)
			{
				scrollPrev();
			}
			else
			{
				capture = true;
			}
		}

		if ((capture || e->forms().RaisedBy == shared_from_this()) &&
		    e->forms().EventFlag == FormEventType::MouseUp)
		{
			capture = false;
		}

		if (e->forms().EventFlag == FormEventType::MouseMove && capture)
		{
			this->setValue(static_cast<int>(mousePosition / segmentsize) + Minimum);
		}

		if (e->forms().EventFlag == FormEventType::MouseMove &&
		    e->forms().RaisedBy == shared_from_this())
		{
			if (BarOrientation == Orientation::Vertical)
			{
				scrollWheel(e);
			}
		}
	}
}

void ScrollBar::onRender()
{
	Control::onRender();

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
			switch (BarOrientation)
			{
				case Orientation::Vertical:
					newpos.x = (Size.x - grippersize) / 2;
					break;
				case Orientation::Horizontal:
					newpos.y = (Size.y - grippersize) / 2;
					break;
			}
			fw().renderer->draw(gripperbutton, newpos);
			break;
	}
}

void ScrollBar::updateScrollChangeValue()
{
	ScrollChange =
	    static_cast<int>(std::max((Maximum - Minimum + 2) * (float)ScrollPercent / 100.0f, 1.0f));
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
		copy = CopyParent->createChild<ScrollBar>(gripperbutton);
	}
	else
	{
		copy = mksp<ScrollBar>();
	}
	copy->Value = this->Value;
	copy->Maximum = this->Maximum;
	copy->Minimum = this->Minimum;
	copy->GripperColour = this->GripperColour;
	copy->ScrollChange = this->ScrollChange;
	copy->ScrollPercent = this->ScrollPercent;
	copy->RenderStyle = this->RenderStyle;
	copyControlData(copy);
	return copy;
}

void ScrollBar::configureSelfFromXml(pugi::xml_node *node)
{
	Control::configureSelfFromXml(node);

	if (auto scrollPercent = node->attribute("scrollpercent"))
	{
		this->ScrollPercent = scrollPercent.as_int();
	}
	if (auto scrollChange = node->attribute("scrollchange"))
	{
		this->ScrollChange = scrollChange.as_int();
	}
	auto gripperImageNode = node->child("gripperimage");
	if (gripperImageNode)
	{
		gripperbutton = fw().data->loadImage(gripperImageNode.text().get());
	}

	auto gripperColourNode = node->child("grippercolour");
	if (gripperColourNode)
	{
		uint8_t r = gripperColourNode.attribute("r").as_uint(0);
		uint8_t g = gripperColourNode.attribute("g").as_uint(0);
		uint8_t b = gripperColourNode.attribute("b").as_uint(0);
		uint8_t a = gripperColourNode.attribute("a").as_uint(255);
		GripperColour = {r, g, b, a};
	}
	auto rangeNode = node->child("range");
	if (rangeNode)
	{
		if (rangeNode.attribute("min"))
		{
			Minimum = rangeNode.attribute("min").as_int();
		}
		if (rangeNode.attribute("max"))
		{
			Maximum = rangeNode.attribute("max").as_int();
		}
	}
}

void ScrollBar::scrollWheel(Event *e)
{
	// FIXME: Scrolling amount should match wheel amount
	// Should wheel orientation match as well? Who has horizontal scrolls??
	int wheelDelta = e->forms().MouseInfo.WheelVertical + e->forms().MouseInfo.WheelHorizontal;
	if (wheelDelta > 0)
	{
		scrollPrev();
	}
	else if (wheelDelta < 0)
	{
		scrollNext();
	}
}

}; // namespace OpenApoc

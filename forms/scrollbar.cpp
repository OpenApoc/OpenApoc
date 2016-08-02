#include "forms/scrollbar.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/includes.h"
#include <tinyxml2.h>

namespace OpenApoc
{

ScrollBar::ScrollBar()
    : Control(), capture(false), grippersize(1), segmentsize(1),
      gripperbutton(fw().data->load_image(
          "PCK:XCOM3/UFODATA/NEWBUT.PCK:XCOM3/UFODATA/NEWBUT.TAB:4:UI/menuopt.pal")),
      buttonerror(fw().data->load_sample("RAWSOUND:xcom3/RAWSOUND/EXTRA/TEXTBEEP.RAW:22050")),
      Value(0), BarOrientation(Orientation::Vertical), RenderStyle(ScrollBarRenderStyle::Menu),
      GripperColour(220, 192, 192), Minimum(0), Maximum(10), LargeChange(2)
{
	// LoadResources();
}

ScrollBar::~ScrollBar() {}

void ScrollBar::LoadResources() {}

bool ScrollBar::SetValue(int newValue)
{
	newValue = std::max(newValue, Minimum);
	newValue = std::min(newValue, Maximum);
	if (newValue == Value)
		return false;

	this->pushFormEvent(FormEventType::ScrollBarChange, nullptr);
	Value = newValue;
	return true;
}

void ScrollBar::ScrollPrev()
{
	if (!SetValue(Value - LargeChange))
	{
		fw().soundBackend->playSample(buttonerror);
	}
}

void ScrollBar::ScrollNext()
{
	if (!SetValue(Value + LargeChange))
	{
		fw().soundBackend->playSample(buttonerror);
	}
}

void ScrollBar::EventOccured(Event *e)
{
	Control::EventOccured(e);

	int mousePosition = 0;
	if (e->Type() == EVENT_FORM_INTERACTION)
	{
		switch (BarOrientation)
		{
			case Orientation::Vertical:
				mousePosition = e->Forms().MouseInfo.Y;
				break;
			case Orientation::Horizontal:
				mousePosition = e->Forms().MouseInfo.X;
				break;
		}
	}

	if (e->Type() == EVENT_FORM_INTERACTION && e->Forms().RaisedBy == shared_from_this() &&
	    e->Forms().EventFlag == FormEventType::MouseDown)
	{
		if (mousePosition >= (segmentsize * (Value - Minimum)) + grippersize)
		{
			ScrollNext();
		}
		else if (mousePosition <= segmentsize * (Value - Minimum))
		{
			ScrollPrev();
		}
		else
		{
			capture = true;
		}
	}

	if (e->Type() == EVENT_FORM_INTERACTION &&
	    (capture || e->Forms().RaisedBy == shared_from_this()) &&
	    e->Forms().EventFlag == FormEventType::MouseUp)
	{
		capture = false;
	}

	if (e->Type() == EVENT_FORM_INTERACTION && e->Forms().RaisedBy == shared_from_this() &&
	    e->Forms().EventFlag == FormEventType::MouseMove && capture)
	{
		this->SetValue(static_cast<int>(mousePosition / segmentsize));
	}
}

void ScrollBar::OnRender()
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

void ScrollBar::Update()
{
	Control::Update();

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

void ScrollBar::UnloadResources() { Control::UnloadResources(); }

sp<Control> ScrollBar::CopyTo(sp<Control> CopyParent)
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
	CopyControlData(copy);
	return copy;
}

void ScrollBar::ConfigureSelfFromXML(tinyxml2::XMLElement *Element)
{
	Control::ConfigureSelfFromXML(Element);
	tinyxml2::XMLElement *subnode;
	UString attribvalue;

	subnode = Element->FirstChildElement("grippercolour");
	if (subnode != nullptr)
	{
		if (subnode->Attribute("a") != nullptr && UString(subnode->Attribute("a")) != "")
		{
			GripperColour = Colour(
			    Strings::ToU8(subnode->Attribute("r")), Strings::ToU8(subnode->Attribute("g")),
			    Strings::ToU8(subnode->Attribute("b")), Strings::ToU8(subnode->Attribute("a")));
		}
		else
		{
			GripperColour = Colour(Strings::ToU8(subnode->Attribute("r")),
			                       Strings::ToU8(subnode->Attribute("g")),
			                       Strings::ToU8(subnode->Attribute("b")));
		}
	}
	subnode = Element->FirstChildElement("range");
	if (subnode != nullptr)
	{
		if (subnode->Attribute("min") != nullptr && UString(subnode->Attribute("min")) != "")
		{
			Minimum = Strings::ToInteger(subnode->Attribute("min"));
		}
		if (subnode->Attribute("max") != nullptr && UString(subnode->Attribute("max")) != "")
		{
			Maximum = Strings::ToInteger(subnode->Attribute("max"));
		}
	}
}
}; // namespace OpenApoc

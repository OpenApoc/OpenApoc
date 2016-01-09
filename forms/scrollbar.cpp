
#include "forms/scrollbar.h"
#include "framework/framework.h"
#include "game/resources/gamecore.h"
#include "framework/includes.h"

namespace OpenApoc
{

ScrollBar::ScrollBar(Control *Owner)
    : Control(Owner), capture(false), grippersize(1), segmentsize(1),
      gripperbutton(fw().data->load_image(
          "PCK:XCOM3/UFODATA/NEWBUT.PCK:XCOM3/UFODATA/NEWBUT.TAB:4:UI/menuopt.pal")),
      Value(0), BarOrientation(Orientation::Vertical),
      RenderStyle(ScrollBarRenderStyles::MenuButtonStyle), GripperColour(220, 192, 192), Minimum(0),
      Maximum(10), LargeChange(2)
{
	// LoadResources();
}

ScrollBar::~ScrollBar() {}

void ScrollBar::LoadResources() {}

void ScrollBar::SetValue(int newValue)
{
	newValue = std::max(newValue, Minimum);
	newValue = std::min(newValue, Maximum);
	if (newValue == Value)
		return;

	auto e = new Event();
	e->Type = EVENT_FORM_INTERACTION;
	e->Data.Forms.RaisedBy = this;
	e->Data.Forms.EventFlag = FormEventType::ScrollBarChange;
	fw().PushEvent(e);
	Value = newValue;
}

void ScrollBar::EventOccured(Event *e)
{
	Control::EventOccured(e);

	int mousePosition = 0;
	switch (BarOrientation)
	{
		case Orientation::Vertical:
			mousePosition = e->Data.Forms.MouseInfo.Y;
			break;
		case Orientation::Horizontal:
			mousePosition = e->Data.Forms.MouseInfo.X;
			break;
	}

	if (e->Type == EVENT_FORM_INTERACTION && e->Data.Forms.RaisedBy == this &&
	    e->Data.Forms.EventFlag == FormEventType::MouseDown)
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

	if (e->Type == EVENT_FORM_INTERACTION && (capture || e->Data.Forms.RaisedBy == this) &&
	    e->Data.Forms.EventFlag == FormEventType::MouseUp)
	{
		capture = false;
	}

	if (e->Type == EVENT_FORM_INTERACTION && e->Data.Forms.RaisedBy == this &&
	    e->Data.Forms.EventFlag == FormEventType::MouseMove && capture)
	{
		this->SetValue(static_cast<int>(mousePosition / segmentsize));
	}
}

void ScrollBar::OnRender()
{
	// LoadResources();

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
		case ScrollBarRenderStyles::SolidButtonStyle:
			fw().renderer->drawFilledRect(newpos, newsize, GripperColour);
			break;
		case ScrollBarRenderStyles::MenuButtonStyle:
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
	if (RenderStyle == ScrollBarRenderStyles::MenuButtonStyle)
	{
		grippersize = gripperbutton->size.x;
		segmentsize = (size - grippersize) / static_cast<float>(segments - 1);
	}
}

void ScrollBar::UnloadResources() { Control::UnloadResources(); }

Control *ScrollBar::CopyTo(Control *CopyParent)
{
	ScrollBar *copy = new ScrollBar(CopyParent);
	copy->Value = this->Value;
	copy->Maximum = this->Maximum;
	copy->Minimum = this->Minimum;
	copy->GripperColour = this->GripperColour;
	copy->LargeChange = this->LargeChange;
	copy->RenderStyle = this->RenderStyle;
	CopyControlData(copy);
	return copy;
}

	void ScrollBar::ConfigureFromXML(tinyxml2::XMLElement* Element)
	{
		Control::ConfigureFromXML(Element);
		tinyxml2::XMLElement *subnode;
		UString attribvalue;

		subnode = Element->FirstChildElement("grippercolour");
		if (subnode != nullptr)
		{
			if (subnode->Attribute("a") != nullptr && UString(subnode->Attribute("a")) != "")
			{
				GripperColour = Colour(Strings::ToU8(subnode->Attribute("r")),
					Strings::ToU8(subnode->Attribute("g")),
					Strings::ToU8(subnode->Attribute("b")),
					Strings::ToU8(subnode->Attribute("a")));
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
			if (subnode->Attribute("min") != nullptr &&
				UString(subnode->Attribute("min")) != "")
			{
				Minimum = Strings::ToInteger(subnode->Attribute("min"));
			}
			if (subnode->Attribute("max") != nullptr &&
				UString(subnode->Attribute("max")) != "")
			{
				Maximum = Strings::ToInteger(subnode->Attribute("max"));
			}
		}
	}
}; // namespace OpenApoc

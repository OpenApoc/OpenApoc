
#include "forms/vscrollbar.h"
#include "framework/framework.h"
#include "game/resources/gamecore.h"

namespace OpenApoc
{

VScrollBar::VScrollBar(Framework &fw, Control *Owner)
    : Control(fw, Owner), capture(false), GripperColour(220, 192, 192), Minimum(0), Maximum(10),
      Value(0), LargeChange(2), AssociatedControl(nullptr)
{
	// LoadResources();
}

VScrollBar::VScrollBar(Framework &fw, Control *Owner, Control *AssociateWith)
    : Control(fw, Owner), capture(false), GripperColour(220, 192, 192), Minimum(0), Maximum(10),
      Value(0), LargeChange(2)
{
	// LoadResources();
	AssociatedControl = AssociateWith;
}

VScrollBar::~VScrollBar() {}

void VScrollBar::LoadResources() {}

void VScrollBar::EventOccured(Event *e)
{
	Control::EventOccured(e);

	if (e->Type == EVENT_FORM_INTERACTION && e->Data.Forms.RaisedBy == this &&
	    e->Data.Forms.EventFlag == FormEventType::MouseDown)
	{
		int segments = (Maximum - Minimum) + 1;
		float segmentsize = Size.y / static_cast<float>(segments);
		float grippersize = segmentsize;
		if (grippersize < 16.0f)
		{
			grippersize = 16.0f;
			segmentsize = (Size.y - grippersize) / static_cast<float>(segments - 1);
		}

		if (e->Data.Forms.MouseInfo.Y >= (segmentsize * (Value - Minimum)) + grippersize)
		{
			Value = std::min(Maximum, Value + LargeChange);
			auto ce = new Event();
			ce->Type = e->Type;
			ce->Data.Forms = e->Data.Forms;
			ce->Data.Forms.EventFlag = FormEventType::ScrollBarChange;
			fw.PushEvent(ce);
		}
		else if (e->Data.Forms.MouseInfo.Y <= segmentsize * (Value - Minimum))
		{
			Value = std::max(Minimum, Value - LargeChange);
			auto ce = new Event();
			ce->Type = e->Type;
			ce->Data.Forms = e->Data.Forms;
			ce->Data.Forms.EventFlag = FormEventType::ScrollBarChange;
			fw.PushEvent(ce);
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
		int segments = (Maximum - Minimum) + 1;
		float segmentsize = Size.y / static_cast<float>(segments);
		Value =
		    std::max(Minimum, Minimum + static_cast<int>(e->Data.Forms.MouseInfo.Y / segmentsize));
		auto ce = new Event();
		ce->Type = e->Type;
		ce->Data.Forms = e->Data.Forms;
		ce->Data.Forms.EventFlag = FormEventType::ScrollBarChange;
		fw.PushEvent(ce);
	}

	if (e->Type == EVENT_FORM_INTERACTION && e->Data.Forms.RaisedBy == this &&
	    e->Data.Forms.EventFlag == FormEventType::MouseClick)
	{
	}
}

void VScrollBar::OnRender()
{
	// LoadResources();

	int segments = (Maximum - Minimum) + 1;
	float segmentsize = Size.y / static_cast<float>(segments);
	float grippersize = segmentsize;
	if (grippersize < 16.0f)
	{
		grippersize = 16.0f;
		segmentsize = (Size.y - grippersize) / static_cast<float>(segments - 1);
	}

	int ypos = segmentsize * (Value - Minimum);
	fw.renderer->drawFilledRect(Vec2<float>{0, ypos}, Vec2<float>{Size.x, grippersize},
	                            GripperColour);
}

void VScrollBar::Update()
{
	Control::Update();
	if (Value > Maximum)
	{
		Value = Maximum;
	}
	if (Value < Minimum)
	{
		Value = Minimum;
	}
}

void VScrollBar::UnloadResources() { Control::UnloadResources(); }

Control *VScrollBar::CopyTo(Control *CopyParent)
{
	VScrollBar *copy = new VScrollBar(fw, CopyParent);
	copy->Value = this->Value;
	copy->Maximum = this->Maximum;
	copy->Minimum = this->Minimum;
	copy->GripperColour = this->GripperColour;
	copy->LargeChange = this->LargeChange;
	CopyControlData((Control *)copy);
	return (Control *)copy;
}

}; // namespace OpenApoc

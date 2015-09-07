
#include "forms/hscrollbar.h"
#include "framework/framework.h"
#include "game/resources/gamecore.h"
#include "framework/includes.h"

namespace OpenApoc
{

HScrollBar::HScrollBar(Framework &fw, Control *Owner)
    : Control(fw, Owner), capture(false), GripperColour(220, 192, 192), Minimum(0), Maximum(10),
      Value(0), LargeChange(2)
{
	// LoadResources();
}

HScrollBar::~HScrollBar() {}

void HScrollBar::LoadResources() {}

void HScrollBar::SetValue(int newValue)
{
	newValue = std::max(newValue, Minimum);
	newValue = std::min(newValue, Maximum);
	if (newValue == Value) {
		return;
	}

	auto e = new Event();
	e->Type = EVENT_FORM_INTERACTION;
	e->Data.Forms.RaisedBy = this;
	e->Data.Forms.EventFlag = FormEventType::ScrollBarChange;
	fw.PushEvent(e);
	Value = newValue;
}

void HScrollBar::EventOccured(Event *e)
{
	Control::EventOccured(e);

	if (e->Type == EVENT_FORM_INTERACTION && e->Data.Forms.RaisedBy == this &&
	    e->Data.Forms.EventFlag == FormEventType::MouseDown) {
		int segments = (Maximum - Minimum) + 1;
		float segmentsize = Size.x / (float)segments;
		float grippersize = segmentsize;
		if (grippersize < 16.0f) {
			grippersize = 16.0f;
			segmentsize = (Size.x - grippersize) / (float)(segments - 1);
		}

		if (e->Data.Forms.MouseInfo.X >= (segmentsize * (Value - Minimum)) + grippersize) {
			this->SetValue(Value + LargeChange);
		} else if (e->Data.Forms.MouseInfo.X <= segmentsize * (Value - Minimum)) {
			this->SetValue(Value - LargeChange);
		} else {
			capture = true;
		}
	}

	if (e->Type == EVENT_FORM_INTERACTION && (capture || e->Data.Forms.RaisedBy == this) &&
	    e->Data.Forms.EventFlag == FormEventType::MouseUp) {
		capture = false;
	}

	if (e->Type == EVENT_FORM_INTERACTION && e->Data.Forms.RaisedBy == this &&
	    e->Data.Forms.EventFlag == FormEventType::MouseMove && capture) {
		int segments = (Maximum - Minimum) + 1;
		float segmentsize = Size.x / (float)segments;
		this->SetValue(e->Data.Forms.MouseInfo.X / segmentsize);
	}

	if (e->Type == EVENT_FORM_INTERACTION && e->Data.Forms.RaisedBy == this &&
	    e->Data.Forms.EventFlag == FormEventType::MouseClick) {
	}
}

void HScrollBar::OnRender()
{
	// LoadResources();

	int segments = (Maximum - Minimum) + 1;
	float segmentsize = Size.x / (float)segments;
	float grippersize = segmentsize;
	if (grippersize < 16.0f) {
		grippersize = 16.0f;
		segmentsize = (Size.x - grippersize) / (float)(segments - 1);
	}

	int xpos = segmentsize * (Value - Minimum);
	fw.renderer->drawFilledRect(Vec2<float>{xpos, 0}, Vec2<float>{grippersize, Size.y},
	                            GripperColour);
}

void HScrollBar::Update() { Control::Update(); }

void HScrollBar::UnloadResources() { Control::UnloadResources(); }

} // namespace OpenApoc

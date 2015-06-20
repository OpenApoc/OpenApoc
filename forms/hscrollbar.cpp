
#include "forms/hscrollbar.h"
#include "framework/framework.h"
#include "game/resources/gamecore.h"
#include "framework/includes.h"

namespace OpenApoc {

HScrollBar::HScrollBar( Framework &fw, Control* Owner ) : Control( fw, Owner ), capture(false), GripperColour( 220, 192, 192 ), Minimum(0), Maximum(10), Value(0),  LargeChange(2)
{
	//LoadResources();
}

HScrollBar::~HScrollBar()
{
}

void HScrollBar::LoadResources()
{
}

void HScrollBar::EventOccured( Event* e )
{
	Control::EventOccured( e );

	if( e->Type == EVENT_FORM_INTERACTION && e->Data.Forms.RaisedBy == this && e->Data.Forms.EventFlag == FormEventType::MouseDown )
	{
		int segments = (Maximum - Minimum) + 1;
		float segmentsize = Size.x / (float)segments;
		float grippersize = segmentsize;
		if (grippersize < 16.0f)
		{
			grippersize = 16.0f;
			segmentsize = (Size.x - grippersize) / (float)(segments - 1);
		}

		if( e->Data.Forms.MouseInfo.X >= (segmentsize * (Value - Minimum)) + grippersize )
		{
			Value = Maths::Min(Maximum, Value + LargeChange);
			Event* ce = new Event();
			ce->Type = e->Type;
			memcpy( (void*)&(ce->Data.Forms), (void*)&(e->Data.Forms), sizeof( FRAMEWORK_FORMS_EVENT ) );
			ce->Data.Forms.EventFlag = FormEventType::ScrollBarChange;
			fw.PushEvent( ce );
		} else if ( e->Data.Forms.MouseInfo.X <= segmentsize * (Value - Minimum) ) {
			Value = Maths::Max(Minimum, Value - LargeChange);
			Event* ce = new Event();
			ce->Type = e->Type;
			memcpy( (void*)&(ce->Data.Forms), (void*)&(e->Data.Forms), sizeof( FRAMEWORK_FORMS_EVENT ) );
			ce->Data.Forms.EventFlag = FormEventType::ScrollBarChange;
			fw.PushEvent( ce );
		} else {
			capture = true;
		}
	}

	if( e->Type == EVENT_FORM_INTERACTION && (capture || e->Data.Forms.RaisedBy == this) && e->Data.Forms.EventFlag == FormEventType::MouseUp )
	{
		capture = false;
	}

	if( e->Type == EVENT_FORM_INTERACTION && e->Data.Forms.RaisedBy == this && e->Data.Forms.EventFlag == FormEventType::MouseMove && capture )
	{
		int segments = (Maximum - Minimum) + 1;
		float segmentsize = Size.x / (float)segments;
		Value = Maths::Max(Minimum, Minimum + (int)(e->Data.Forms.MouseInfo.X / segmentsize));
		Event* ce = new Event();
		ce->Type = e->Type;
		memcpy( (void*)&(ce->Data.Forms), (void*)&(e->Data.Forms), sizeof( FRAMEWORK_FORMS_EVENT ) );
		ce->Data.Forms.EventFlag = FormEventType::ScrollBarChange;
		fw.PushEvent( ce );
	}

	if( e->Type == EVENT_FORM_INTERACTION && e->Data.Forms.RaisedBy == this && e->Data.Forms.EventFlag == FormEventType::MouseClick )
	{
	}
}

void HScrollBar::OnRender()
{
	//LoadResources();

	int segments = (Maximum - Minimum) + 1;
	float segmentsize = Size.x / (float)segments;
	float grippersize = segmentsize;
	if (grippersize < 16.0f)
	{
		grippersize = 16.0f;
		segmentsize = (Size.x - grippersize) / (float)(segments - 1);
	}

	int xpos = segmentsize * (Value - Minimum);
	fw.renderer->drawRect(Vec2<float>{xpos, 0}, Vec2<float>{grippersize, Size.y},
		GripperColour);
}

void HScrollBar::Update()
{
	Control::Update();
}

void HScrollBar::UnloadResources()
{
	Control::UnloadResources();
}

}; //namespace OpenApoc

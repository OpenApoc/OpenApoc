#include "library/sp.h"

#include "forms/checkbox.h"
#include "framework/framework.h"
#include "game/resources/gamecore.h"

namespace OpenApoc
{

CheckBox::CheckBox(Framework &fw, Control *Owner) : Control(fw, Owner), Checked(false)
{
	LoadResources();
	this->buttonclick =
	    fw.data->load_sample("RAWSOUND:xcom3/RAWSOUND/STRATEGC/INTRFACE/BUTTON1.RAW:22050");
}

CheckBox::~CheckBox() {}

void CheckBox::LoadResources()
{
	if (!imagechecked)
	{
		imagechecked = fw.gamecore->GetImage(
		    "PCK:xcom3/UFODATA/NEWBUT.PCK:xcom3/UFODATA/NEWBUT.TAB:65:UI/UI_PALETTE.PNG");
		if (Size.x == 0)
		{
			Size.x = imagechecked->size.x;
		}
		if (Size.y == 0)
		{
			Size.y = imagechecked->size.y;
		}
	}
	if (!imageunchecked)
	{
		imageunchecked = fw.gamecore->GetImage(
		    "PCK:xcom3/UFODATA/NEWBUT.PCK:xcom3/UFODATA/NEWBUT.TAB:64:UI/UI_PALETTE.PNG");
	}
}

void CheckBox::EventOccured(Event *e)
{
	Control::EventOccured(e);

	if (e->Type == EVENT_FORM_INTERACTION && e->Data.Forms.RaisedBy == this &&
	    e->Data.Forms.EventFlag == FormEventType::MouseDown)
	{
		fw.soundBackend->playSample(buttonclick);
	}

	if (e->Type == EVENT_FORM_INTERACTION && e->Data.Forms.RaisedBy == this &&
	    e->Data.Forms.EventFlag == FormEventType::MouseClick)
	{
		Checked = !Checked;
		auto ce = new Event();
		ce->Type = e->Type;
		ce->Data.Forms = e->Data.Forms;
		ce->Data.Forms.EventFlag = FormEventType::CheckBoxChange;
		fw.PushEvent(ce);
	}
}

void CheckBox::OnRender()
{
	sp<Image> useimage;

	LoadResources();

	useimage = (Checked ? imagechecked : imageunchecked);

	if (useimage != nullptr)
	{
		if (useimage->size == Vec2<unsigned int>{Size.x, Size.y})
		{
			fw.renderer->draw(useimage, Vec2<float>{0, 0});
		}
		else
		{
			fw.renderer->drawScaled(useimage, Vec2<float>{0, 0},
			                        Vec2<float>{this->Size.x, this->Size.y});
		}
	}
}

void CheckBox::Update() { Control::Update(); }

void CheckBox::UnloadResources()
{
	imagechecked.reset();
	imageunchecked.reset();
	Control::UnloadResources();
}

Control *CheckBox::CopyTo(Control *CopyParent)
{
	CheckBox *copy = new CheckBox(fw, CopyParent);
	copy->Checked = this->Checked;
	CopyControlData((Control *)copy);
	return (Control *)copy;
}

}; // namespace OpenApoc

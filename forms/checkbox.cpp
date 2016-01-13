#include "library/sp.h"

#include "forms/checkbox.h"
#include "framework/framework.h"

namespace OpenApoc
{

CheckBox::CheckBox(Control *Owner, sp<Image> ImageChecked, sp<Image> ImageUnchecked)
    : Control(Owner), imagechecked(ImageChecked), imageunchecked(ImageUnchecked),
      buttonclick(
          fw().data->load_sample("RAWSOUND:xcom3/RAWSOUND/STRATEGC/INTRFACE/BUTTON1.RAW:22050")),
      Checked(false)
{
	LoadResources();
}

CheckBox::~CheckBox() {}

void CheckBox::LoadResources()
{
	if (!imagechecked)
	{
		imagechecked = fw().data->load_image(
		    "PCK:xcom3/UFODATA/NEWBUT.PCK:xcom3/UFODATA/NEWBUT.TAB:65:UI/menuopt.pal");
	}
	if (!imageunchecked)
	{
		imageunchecked = fw().data->load_image(
		    "PCK:xcom3/UFODATA/NEWBUT.PCK:xcom3/UFODATA/NEWBUT.TAB:64:UI/menuopt.pal");
	}
	if (imagechecked)
	{
		if (Size.x == 0)
		{
			Size.x = imagechecked->size.x;
		}
		if (Size.y == 0)
		{
			Size.y = imagechecked->size.y;
		}
	}
}

void CheckBox::EventOccured(Event *e)
{
	Control::EventOccured(e);

	if (e->Type == EVENT_FORM_INTERACTION && e->Data.Forms.RaisedBy == this &&
	    e->Data.Forms.EventFlag == FormEventType::MouseDown)
	{
		fw().soundBackend->playSample(buttonclick);
	}

	if (e->Type == EVENT_FORM_INTERACTION && e->Data.Forms.RaisedBy == this &&
	    e->Data.Forms.EventFlag == FormEventType::MouseClick)
	{
		SetChecked(!IsChecked());
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
			fw().renderer->draw(useimage, Vec2<float>{0, 0});
		}
		else
		{
			fw().renderer->drawScaled(useimage, Vec2<float>{0, 0},
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

void CheckBox::SetChecked(bool checked)
{
	if (Checked == checked)
		return;
	auto e = new Event();
	e->Type = EVENT_FORM_INTERACTION;
	e->Data.Forms.RaisedBy = this;
	e->Data.Forms.EventFlag = FormEventType::CheckBoxChange;
	fw().PushEvent(e);
	Checked = checked;
}

Control *CheckBox::CopyTo(Control *CopyParent)
{
	CheckBox *copy = new CheckBox(CopyParent, imagechecked, imageunchecked);
	copy->Checked = this->Checked;
	CopyControlData(copy);
	return copy;
}

void CheckBox::ConfigureFromXML(tinyxml2::XMLElement *Element)
{
	Control::ConfigureFromXML(Element);
	if (Element->FirstChildElement("image") != nullptr)
	{
		imageunchecked = fw().data->load_image(Element->FirstChildElement("image")->GetText());
	}
	if (Element->FirstChildElement("imagechecked") != nullptr)
	{
		imagechecked = fw().data->load_image(Element->FirstChildElement("imagechecked")->GetText());
	}
}
}; // namespace OpenApoc

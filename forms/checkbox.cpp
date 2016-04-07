#include "forms/checkbox.h"
#include "framework/framework.h"
#include "library/sp.h"
#include <tinyxml2.h>

namespace OpenApoc
{

CheckBox::CheckBox(sp<Image> ImageChecked, sp<Image> ImageUnchecked)
    : Control(), imagechecked(ImageChecked), imageunchecked(ImageUnchecked),
      buttonclick(
          fw().data->load_sample("RAWSOUND:xcom3/RAWSOUND/STRATEGC/INTRFACE/BUTTON1.RAW:22050")),
      Checked(false)
{
}

CheckBox::~CheckBox() {}

void CheckBox::EventOccured(Event *e)
{
	Control::EventOccured(e);

	if (e->Type() == EVENT_FORM_INTERACTION && e->Forms().RaisedBy == shared_from_this() &&
	    e->Forms().EventFlag == FormEventType::MouseDown)
	{
		fw().soundBackend->playSample(buttonclick);
	}

	if (e->Type() == EVENT_FORM_INTERACTION && e->Forms().RaisedBy == shared_from_this() &&
	    e->Forms().EventFlag == FormEventType::MouseClick)
	{
		SetChecked(!IsChecked());
	}
}

void CheckBox::OnRender()
{
	sp<Image> useimage;

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
	Checked = checked;
	this->pushFormEvent(FormEventType::CheckBoxChange, nullptr);
	if (checked)
	{
		this->pushFormEvent(FormEventType::CheckBoxSelected, nullptr);
	}
	else
	{
		this->pushFormEvent(FormEventType::CheckBoxDeSelected, nullptr);
	}
}

sp<Control> CheckBox::CopyTo(sp<Control> CopyParent)
{
	sp<CheckBox> copy;
	if (CopyParent)
	{
		copy = CopyParent->createChild<CheckBox>(imagechecked, imageunchecked);
	}
	else
	{
		copy = mksp<CheckBox>(imagechecked, imageunchecked);
	}
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

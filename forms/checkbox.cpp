#include "forms/checkbox.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "library/sp.h"
#include <tinyxml2.h>

namespace OpenApoc
{

CheckBox::CheckBox(sp<Image> ImageChecked, sp<Image> ImageUnchecked)
    : Control(), imagechecked(ImageChecked), imageunchecked(ImageUnchecked),
      buttonclick(
          fw().data->loadSample("RAWSOUND:xcom3/rawsound/strategc/intrface/button1.raw:22050")),
      Checked(false)
{
}

CheckBox::~CheckBox() = default;

void CheckBox::eventOccured(Event *e)
{
	Control::eventOccured(e);

	if (e->type() == EVENT_FORM_INTERACTION && e->forms().RaisedBy == shared_from_this() &&
	    e->forms().EventFlag == FormEventType::MouseDown)
	{
		if (buttonclick)
		{
			fw().soundBackend->playSample(buttonclick);
		}
	}

	if (e->type() == EVENT_FORM_INTERACTION && e->forms().RaisedBy == shared_from_this() &&
	    e->forms().EventFlag == FormEventType::MouseClick)
	{
		setChecked(!isChecked());
	}
}

void CheckBox::onRender()
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

void CheckBox::update() { Control::update(); }

void CheckBox::unloadResources()
{
	imagechecked.reset();
	imageunchecked.reset();
	Control::unloadResources();
}

sp<Sample> CheckBox::getClickSound() const { return buttonclick; }

void CheckBox::setClickSound(sp<Sample> sample) { buttonclick = sample; }

void CheckBox::setChecked(bool checked)
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

sp<Control> CheckBox::copyTo(sp<Control> CopyParent)
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
	copyControlData(copy);
	return copy;
}

void CheckBox::configureSelfFromXml(tinyxml2::XMLElement *Element)
{
	Control::configureSelfFromXml(Element);
	if (Element->FirstChildElement("image") != nullptr)
	{
		imageunchecked = fw().data->loadImage(Element->FirstChildElement("image")->GetText());
	}
	if (Element->FirstChildElement("imagechecked") != nullptr)
	{
		imagechecked = fw().data->loadImage(Element->FirstChildElement("imagechecked")->GetText());
	}
}
}; // namespace OpenApoc

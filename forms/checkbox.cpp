#include "forms/checkbox.h"
#include "dependencies/pugixml/src/pugixml.hpp"
#include "framework/data.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/image.h"
#include "framework/renderer.h"
#include "framework/sound.h"
#include "library/sp.h"

namespace OpenApoc
{

CheckBox::CheckBox(sp<Image> ImageChecked, sp<Image> ImageUnchecked)
    : Control(), imagechecked(ImageChecked), imageunchecked(ImageUnchecked),
      buttonclick(
          fw().data->loadSample("RAWSOUND:xcom3/rawsound/strategc/intrface/button1.raw:22050")),
      Checked(false)
{
	isClickable = true;
}

CheckBox::~CheckBox() = default;

bool CheckBox::click()
{
	if (!Control::click())
	{
		return false;
	}
	if (buttonclick)
	{
		fw().soundBackend->playSample(buttonclick);
	}
	return true;
}

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
	Control::onRender();

	sp<Image> useimage;

	useimage = (Checked ? imagechecked : imageunchecked);

	if (useimage != nullptr)
	{
		fw().renderer->draw(useimage, Vec2<float>{0, 0});
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

void CheckBox::configureSelfFromXml(pugi::xml_node *node)
{
	Control::configureSelfFromXml(node);
	if (node->child("image"))
	{
		imageunchecked = fw().data->loadImage(node->child("image").text().get());
	}
	if (node->child("imagechecked"))
	{
		imagechecked = fw().data->loadImage(node->child("imagechecked").text().get());
	}
}
}; // namespace OpenApoc

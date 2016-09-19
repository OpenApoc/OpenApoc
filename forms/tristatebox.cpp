#include "forms/tristatebox.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "library/sp.h"
#include <tinyxml2.h>

namespace OpenApoc
{
TriStateBox::TriStateBox(sp<Image> Image1, sp<Image> Image2, sp<Image> Image3)
    : Control(), image1(Image1), image2(Image2), image3(Image3),
      buttonclick(
          fw().data->loadSample("RAWSOUND:xcom3/rawsound/strategc/intrface/button1.raw:22050")),
      State(1)
{
}

TriStateBox::~TriStateBox() = default;

void TriStateBox::eventOccured(Event *e)
{
	Control::eventOccured(e);

	if (e->type() == EVENT_FORM_INTERACTION && e->forms().RaisedBy == shared_from_this() &&
	    e->forms().EventFlag == FormEventType::MouseDown)
	{
		fw().soundBackend->playSample(buttonclick);
	}

	if (e->type() == EVENT_FORM_INTERACTION && e->forms().RaisedBy == shared_from_this() &&
	    e->forms().EventFlag == FormEventType::MouseClick)
	{
		nextState();
	}
}

void TriStateBox::onRender()
{
	sp<Image> useimage;

	switch (State)
	{
		case 1:
			useimage = image1;
			break;
		case 2:
			useimage = image2;
			break;
		case 3:
			useimage = image3;
			break;
	}

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

void TriStateBox::update() { Control::update(); }

void TriStateBox::unloadResources()
{
	image1.reset();
	image2.reset();
	image3.reset();
	Control::unloadResources();
}

void TriStateBox::setState(int state)
{
	if (State == state)
		return;
	State = state;
	this->pushFormEvent(FormEventType::TriStateBoxChange, nullptr);
	switch (State)
	{
		case 1:
			this->pushFormEvent(FormEventType::TriStateBoxState1Selected, nullptr);
			break;
		case 2:
			this->pushFormEvent(FormEventType::TriStateBoxState2Selected, nullptr);
			break;
		case 3:
			this->pushFormEvent(FormEventType::TriStateBoxState3Selected, nullptr);
			break;
	}
}

void TriStateBox::nextState()
{
	switch (State)
	{
		case 1:
			setState(2);
			break;
		case 2:
			setState(3);
			break;
		case 3:
			setState(1);
			break;
	}
}

int TriStateBox::getState() { return State; }

sp<Control> TriStateBox::copyTo(sp<Control> CopyParent)
{
	sp<TriStateBox> copy;
	if (CopyParent)
	{
		copy = CopyParent->createChild<TriStateBox>(image1, image2, image3);
	}
	else
	{
		copy = mksp<TriStateBox>(image1, image2, image3);
	}
	copy->State = this->State;
	copyControlData(copy);
	return copy;
}

void TriStateBox::configureSelfFromXml(tinyxml2::XMLElement *Element)
{
	Control::configureSelfFromXml(Element);
	if (Element->FirstChildElement("image") != nullptr)
	{
		image1 = fw().data->loadImage(Element->FirstChildElement("image")->GetText());
	}
	if (Element->FirstChildElement("image2") != nullptr)
	{
		image2 = fw().data->loadImage(Element->FirstChildElement("image2")->GetText());
	}
	if (Element->FirstChildElement("image3") != nullptr)
	{
		image3 = fw().data->loadImage(Element->FirstChildElement("image3")->GetText());
	}
}
}; // namespace OpenApoc

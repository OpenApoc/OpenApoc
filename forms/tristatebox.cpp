#include "forms/tristatebox.h"
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
TriStateBox::TriStateBox(sp<Image> Image1, sp<Image> Image2, sp<Image> Image3)
    : Control(), image1(Image1), image2(Image2), image3(Image3),
      buttonclick(
          fw().data->loadSample("RAWSOUND:xcom3/rawsound/strategc/intrface/button1.raw:22050")),
      State(1)
{
}

TriStateBox::~TriStateBox() = default;

bool TriStateBox::click()
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

void TriStateBox::eventOccured(Event *e)
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
		nextState();
	}
}

void TriStateBox::onRender()
{
	Control::onRender();

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

void TriStateBox::configureSelfFromXml(pugi::xml_node *node)
{
	Control::configureSelfFromXml(node);
	auto imageNode = node->child("image");
	if (imageNode)
	{
		image1 = fw().data->loadImage(imageNode.text().get());
	}
	auto imageNode2 = node->child("image2");
	if (imageNode2)
	{
		image2 = fw().data->loadImage(imageNode2.text().get());
	}
	auto imageNode3 = node->child("image3");
	if (imageNode3)
	{
		image3 = fw().data->loadImage(imageNode3.text().get());
	}
}
}; // namespace OpenApoc

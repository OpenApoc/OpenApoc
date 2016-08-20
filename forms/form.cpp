#include "form.h"
#include <tinyxml2.h>

namespace OpenApoc
{

Form::Form() : Control() {}

Form::~Form() = default;

void Form::readFormStyle(tinyxml2::XMLElement *FormConfiguration)
{
	if (FormConfiguration == nullptr)
	{
		return;
	}

	tinyxml2::XMLElement *node;
	tinyxml2::XMLElement *usenode = nullptr;
	UString nodename;

	if (FormConfiguration->Attribute("id"))
		this->Name = FormConfiguration->Attribute("id");

	for (node = FormConfiguration->FirstChildElement(); node != nullptr;
	     node = node->NextSiblingElement())
	{
		nodename = node->Name();
		if (nodename == "style")
		{
			// TODO: Determine best "style" based on minwidth and minheight attributes
			usenode = node;
		}
	}

	if (usenode != nullptr)
	{
		configureFromXml(usenode);
		resolveLocation();
	}
}

void Form::eventOccured(Event *e) { Control::eventOccured(e); }

void Form::onRender() {}

void Form::update()
{
	Control::update();
	resolveLocation();
}

void Form::unloadResources() { Control::unloadResources(); }

sp<Control> Form::copyTo(sp<Control> CopyParent)
{
	std::ignore = CopyParent;
	auto copy = mksp<Form>();
	copyControlData(copy);
	return copy;
}

}; // namespace OpenApoc

#include "form.h"

namespace OpenApoc
{

Form::Form() : Control() {}

Form::~Form() {}

void Form::ReadFormStyle(tinyxml2::XMLElement *FormConfiguration)
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
		ConfigureFromXML(usenode);
		ResolveLocation();
	}
}

void Form::EventOccured(Event *e) { Control::EventOccured(e); }

void Form::OnRender() {}

void Form::Update()
{
	Control::Update();
	ResolveLocation();
}

void Form::UnloadResources() { Control::UnloadResources(); }

sp<Control> Form::CopyTo(sp<Control> CopyParent)
{
	std::ignore = CopyParent;
	auto copy = mksp<Form>();
	CopyControlData(copy);
	return copy;
}

}; // namespace OpenApoc

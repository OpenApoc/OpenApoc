
#include "ufopaediaentry.h"

namespace OpenApoc
{

UfopaediaEntry::UfopaediaEntry(tinyxml2::XMLElement *Element)
{
	UString nodename;

	if (Element->Attribute("id") != nullptr && UString(Element->Attribute("id")) != "")
	{
		ID = Element->Attribute("id");
	}

	if (Element->Attribute("name") != nullptr && UString(Element->Attribute("name")) != "")
	{
		Title = Element->Attribute("name");
	}

	tinyxml2::XMLElement *node;
	for (node = Element->FirstChildElement(); node != nullptr; node = node->NextSiblingElement())
	{
		nodename = node->Name();

		if (nodename == "backgroundimage")
		{
			BackgroundImageFilename = node->GetText();
		}
		if (nodename == "text_info")
		{
			BodyInformation = node->GetText();
		}
		if (nodename == "dynamic_data")
		{
			DynamicDataMode = node->GetText();
		}
	}
}

UfopaediaEntry::~UfopaediaEntry() {}
};
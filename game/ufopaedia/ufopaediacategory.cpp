
#include "ufopaediacategory.h"


namespace OpenApoc {

UfopaediaCategory::UfopaediaCategory(tinyxml2::XMLElement* Element)
{
	UString nodename;

	if( Element->Attribute("id") != nullptr && UString(Element->Attribute("id")) != "" )
	{
		ID = Element->Attribute("id");
	}

	tinyxml2::XMLElement* node;
	for( node = Element->FirstChildElement(); node != nullptr; node = node->NextSiblingElement() )
	{
		nodename = node->Name();

		if( nodename == "backgroundimage" )
		{
			BackgroundImageFilename = node->GetText();
		}
		if( nodename == "title" )
		{
			Title = node->GetText();
		}
		if( nodename == "text_info" )
		{
			BodyInformation = node->GetText();
		}
		if( nodename == "entries" )
		{
			tinyxml2::XMLElement* node2;
			for( node2 = node->FirstChildElement(); node2 != nullptr; node2 = node2->NextSiblingElement() )
			{
				std::shared_ptr<UfopaediaEntry> newentry = std::make_shared<UfopaediaEntry>( node2 );
				Entries.push_back( newentry );
			}
		}
	}
}

UfopaediaCategory::~UfopaediaCategory()
{
}

}; //namespace OpenApoc

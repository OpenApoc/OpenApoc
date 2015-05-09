#include "form.h"

namespace OpenApoc {

Form::Form( Framework &fw, tinyxml2::XMLElement* FormConfiguration ) : Control( fw, nullptr )
{
	if( FormConfiguration == nullptr )
	{
		return;
	}

	tinyxml2::XMLElement* node;
	tinyxml2::XMLElement* usenode = nullptr;
	std::string nodename;

	for( node = FormConfiguration->FirstChildElement(); node != nullptr; node = node->NextSiblingElement() )
	{
		nodename = node->Name();
		if( nodename == "style" )
		{
			// TODO: Determine best "style" based on minwidth and minheight attributes
			usenode = node;
		}
	}

	if( usenode != nullptr )
	{
		ConfigureFromXML( usenode );
		ResolveLocation();
	}

}

Form::~Form()
{
}

void Form::EventOccured( Event* e )
{
	Control::EventOccured( e );
}

void Form::OnRender()
{
}

void Form::Update()
{
	Control::Update();
	ResolveLocation();
}

void Form::UnloadResources()
{
	Control::UnloadResources();
}

}; //namespace OpenApoc

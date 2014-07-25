
#include "gamecore.h"
#include "../../framework/framework.h"

GameCore* GameCore::ActiveGame = nullptr;

GameCore::GameCore(std::string CoreXMLFilename, std::string Language)
{
	language = Language;
	ParseXMLDoc( CoreXMLFilename );

	ActiveGame = this;
}

GameCore::~GameCore()
{

}

void GameCore::ParseXMLDoc( std::string XMLFilename )
{
	tinyxml2::XMLDocument doc;
	tinyxml2::XMLElement* node;
	std::string actualfile = DATA->GetActualFilename(XMLFilename);

	doc.LoadFile( actualfile.c_str() );
	node = doc.RootElement();

	std::string nodename = node->Name();

	if( nodename == "openapoc" )
	{
		for( node = node->FirstChildElement(); node != nullptr; node = node->NextSiblingElement() )
		{
			nodename = node->Name();
			if( nodename == "game" )
			{
				ParseGameXML( node );
			}
			if( nodename == "string" || nodename == "fontcharacters" )
			{
				ParseStringXML( node );
			}
			if( nodename == "form" )
			{
				ParseFormXML( node );
			}
		}
	}
}

void GameCore::ParseGameXML( tinyxml2::XMLElement* Source )
{
	tinyxml2::XMLElement* node;
	std::string nodename;

	for( node = Source->FirstChildElement(); node != nullptr; node = node->NextSiblingElement() )
	{
		nodename = node->Name();
		if( nodename == "title" )
		{
			FRAMEWORK->Display_SetTitle( node->GetText() );
		}
		if( nodename == "include" )
		{
			ParseXMLDoc( node->GetText() );
		}
	}
}

void GameCore::ParseStringXML( tinyxml2::XMLElement* Source )
{
	std::string nodename = Source->Name();
	if( nodename == "fontcharacters" )
	{
		ApocalypseFont::FontCharacterSet = Source->GetText();
		return;
	}

	if( nodename == "string" )
	{
		if( Source->FirstChildElement(language.c_str()) != nullptr )
		{
			languagetext[Source->Attribute("id")] = Source->FirstChildElement(language.c_str())->GetText();
		}
	}
}

void GameCore::ParseFormXML( tinyxml2::XMLElement* Source )
{
	forms[Source->Attribute("id")] = new Form( Source );
}

std::string GameCore::GetString(std::string ID)
{
	std::string s = languagetext[ID];
	if( s == "" )
	{
		s = ID;
	}
	return s;
}

Form* GameCore::GetForm(std::string ID)
{
	return forms[ID];
}

ALLEGRO_BITMAP* GameCore::GetImage(std::string ImageData)
{
	return nullptr;
}

IFont* GameCore::GetFont(std::string FontData)
{
	IFont* font = fonts[FontData];

	// Use cached font
	if( font != nullptr )
	{
		return font;
	}

	return font;
}


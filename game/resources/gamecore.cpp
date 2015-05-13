
#include "game/resources/gamecore.h"
#include "framework/framework.h"

namespace OpenApoc {

GameCore::GameCore(Framework &fw)
	: languagetext(), fonts(), forms(), fw(fw), vehicleFactory(fw)
{
	Loaded = false;
}

void GameCore::Load(std::string CoreXMLFilename, std::string Language)
{
	assert(Loaded == false);
	language = Language;
	ParseXMLDoc( CoreXMLFilename );
	DebugModeEnabled = false;

	MouseCursor = new ApocCursor( fw, fw.gamecore->GetPalette( "xcom3/tacdata/TACTICAL.PAL" ) );

	Loaded = true;
}

GameCore::~GameCore()
{
	for (auto & form : forms)
		delete form.second;
	delete MouseCursor;
}

void GameCore::ParseXMLDoc( std::string XMLFilename )
{
	tinyxml2::XMLDocument doc;
	tinyxml2::XMLElement* node;
	std::string actualfile = fw.data->GetActualFilename(XMLFilename.c_str());

	if (actualfile == "")
	{
		LogError("Failed to read XML file \"%s\"", XMLFilename.c_str());
		return;
	}
	LogInfo("Loading XML file \"%s\" - found at \"%s\"", XMLFilename.c_str(), actualfile.c_str());

	doc.LoadFile( actualfile.c_str() );
	node = doc.RootElement();

	if (!node)
	{
		LogError("Failed to parse XML file \"%s\"", actualfile.c_str());
		return;
	}

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
			else if( nodename == "string" )
			{
				ParseStringXML( node );
			}
			else if( nodename == "form" )
			{
				ParseFormXML( node );
			}
			else if( nodename == "vehicle" )
			{
				vehicleFactory.ParseVehicleDefinition( node );
			}
			else if (nodename == "apocfont" )
			{
				std::string fontName = node->Attribute("name");
				if (fontName == "")
				{
					LogError("apocfont element with no name");
					continue;
				}
				auto font = ApocalypseFont::loadFont(fw, node);
				if (!font)
				{
					LogError("apocfont element \"%s\" failed to load", fontName.c_str());
					continue;
				}

				if (this->fonts.find(fontName) != this->fonts.end())
				{
					LogError("multiple fonts with name \"%s\"", fontName.c_str());
					continue;
				}
				this->fonts[fontName] = font;
			}
			else
			{
				LogError("Unknown XML element \"%s\"", nodename.c_str());
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
			fw.Display_SetTitle( node->GetText() );
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
	forms[Source->Attribute("id")] = new Form( fw, Source );
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

std::shared_ptr<Image> GameCore::GetImage(std::string ImageData)
{
	return fw.data->load_image(ImageData);
}

std::shared_ptr<BitmapFont> GameCore::GetFont(std::string FontData)
{
	if( fonts.find( FontData ) == fonts.end() )
	{
		LogError("Missing font \"%s\"", FontData.c_str());
		return nullptr;
	}
	return fonts[FontData];
}

std::shared_ptr<Palette> GameCore::GetPalette(std::string Path)
{
	return fw.data->load_palette(Path);
}

}; //namespace OpenApoc

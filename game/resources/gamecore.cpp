
#include "game/resources/gamecore.h"
#include "framework/framework.h"

namespace OpenApoc {

GameCore::GameCore(Framework &fw)
	: languagetext(), fonts(), forms(), fw(fw), vehicleFactory(fw)
{
	Loaded = false;
}

void GameCore::Load(UString CoreXMLFilename, UString Language)
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

void GameCore::ParseXMLDoc( UString XMLFilename )
{
	tinyxml2::XMLDocument doc;
	tinyxml2::XMLElement* node;
	UString actualfile = fw.data->GetActualFilename(XMLFilename.str().c_str());

	if (actualfile == "")
	{
		LogError("Failed to read XML file \"%s\"", XMLFilename.str().c_str());
		return;
	}
	LogInfo("Loading XML file \"%s\" - found at \"%s\"", XMLFilename.str().c_str(), actualfile.str().c_str());

	doc.LoadFile( actualfile.str().c_str() );
	node = doc.RootElement();

	if (!node)
	{
		LogError("Failed to parse XML file \"%s\"", actualfile.str().c_str());
		return;
	}

	UString nodename = node->Name();

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
				UString fontName = node->Attribute("name");
				if (fontName == "")
				{
					LogError("apocfont element with no name");
					continue;
				}
				auto font = ApocalypseFont::loadFont(fw, node);
				if (!font)
				{
					LogError("apocfont element \"%s\" failed to load", fontName.str().c_str());
					continue;
				}

				if (this->fonts.find(fontName) != this->fonts.end())
				{
					LogError("multiple fonts with name \"%s\"", fontName.str().c_str());
					continue;
				}
				this->fonts[fontName] = font;
			}
			else
			{
				LogError("Unknown XML element \"%s\"", nodename.str().c_str());
			}
		}
	}
}

void GameCore::ParseGameXML( tinyxml2::XMLElement* Source )
{
	tinyxml2::XMLElement* node;
	UString nodename;

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
	UString nodename = Source->Name();
	if( nodename == "string" )
	{
		if( Source->FirstChildElement(language.str().c_str()) != nullptr )
		{
			languagetext[Source->Attribute("id")] = Source->FirstChildElement(language.str().c_str())->GetText();
		}
	}
}

void GameCore::ParseFormXML( tinyxml2::XMLElement* Source )
{
	forms[Source->Attribute("id")] = new Form( fw, Source );
}

UString GameCore::GetString(UString ID)
{
	UString s = languagetext[ID];
	if( s == "" )
	{
		s = ID;
	}
	return s;
}

Form* GameCore::GetForm(UString ID)
{
	return forms[ID];
}

std::shared_ptr<Image> GameCore::GetImage(UString ImageData)
{
	return fw.data->load_image(ImageData);
}

std::shared_ptr<BitmapFont> GameCore::GetFont(UString FontData)
{
	if( fonts.find( FontData ) == fonts.end() )
	{
		LogError("Missing font \"%s\"", FontData.str().c_str());
		return nullptr;
	}
	return fonts[FontData];
}

std::shared_ptr<Palette> GameCore::GetPalette(UString Path)
{
	return fw.data->load_palette(Path);
}

}; //namespace OpenApoc

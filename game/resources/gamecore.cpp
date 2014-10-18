
#include "gamecore.h"
#include "../../framework/framework.h"
#include "ttffont.h"

namespace OpenApoc {

GameCore::GameCore(Framework &fw)
	: languagetext(), palettes(), fonts(), forms(), fw(fw), vehicleFactory(fw)
{
	Loaded = false;
}

void GameCore::Load(std::string CoreXMLFilename, std::string Language)
{
	assert(Loaded == false);
	language = Language;
	ParseXMLDoc( CoreXMLFilename );
	DebugModeEnabled = false;

	MouseCursor = new Cursor( fw, fw.gamecore->GetPalette( "TACDATA/TACTICAL.PAL" ) );

	Loaded = true;
}

GameCore::~GameCore()
{
	for (auto & font : fonts)
		delete font.second;
	for (auto & form : forms)
		delete form.second;
	for (auto & palette : palettes)
		delete palette.second;
	delete MouseCursor;
}

void GameCore::ParseXMLDoc( std::string XMLFilename )
{
	tinyxml2::XMLDocument doc;
	tinyxml2::XMLElement* node;
	std::string actualfile = fw.data.GetActualFilename(XMLFilename);

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
			if( nodename == "vehicle" )
			{
				vehicleFactory.ParseVehicleDefinition( node );
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
	PCK* pck;

	if( ImageData.substr( 0, 4 ) == "PCK:" )
	{
		std::vector<std::string> pckdata = Strings::Split( ImageData, ':' );
		std::unique_ptr<PCK> pck(new PCK( fw, pckdata[1], pckdata[2], *GetPalette( pckdata[4] ), Strings::ToInteger( pckdata[3] ) ));
		return  pck->GetImage( 0 );
	} else {
		return fw.data.load_image(ImageData);
	}
}

IFont* GameCore::GetFont(std::string FontData)
{
	if( fonts.find( FontData ) == fonts.end() )
	{
		std::vector<std::string> segs = Strings::Split( FontData, ':' );
		if( segs.at(0) == "APOC" )
		{

			if( segs.at(1) == "LARGE" )
			{
				fonts[FontData] = new ApocalypseFont( fw, ApocalypseFont::LargeFont, GetPalette( segs.at(2) ) );
			}
			if( segs.at(1) == "SMALL" )
			{
				fonts[FontData] = new ApocalypseFont( fw, ApocalypseFont::SmallFont, GetPalette( segs.at(2) ) );
			}
			if( segs.at(1) == "TINY" )
			{
				fonts[FontData] = new ApocalypseFont( fw, ApocalypseFont::TinyFont, GetPalette( segs.at(2) ) );
			}

		} else {
			if( segs.size() == 1 )
			{
				fonts[FontData] = new TTFFont( segs.at(0), 8 );
			} else if( segs.size() == 2 ) {
				fonts[FontData] = new TTFFont( segs.at(0), Strings::ToInteger( segs.at(1) ) );
			}
		}
	}
	return fonts[FontData];
}

Palette* GameCore::GetPalette(std::string Path)
{
	if( palettes.find(Path) == palettes.end() )
	{
		palettes[Path] = new Palette( fw, Path );
	}
	return palettes[Path];
}

}; //namespace OpenApoc


#include "game/resources/gamecore.h"
#include "framework/framework.h"

#include "game/ufopaedia/ufopaedia.h"

namespace OpenApoc {

GameCore::GameCore(Framework &fw)
	: supportedlanguages(), languagetext(), fonts(), forms(), fw(fw)
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
	UString systemPath;
	{
		auto file = fw.data->load_file(XMLFilename);
		if (!file)
		{
			LogError("Failed to open XML file \"%s\"", XMLFilename.str().c_str());
		}
		systemPath = file.systemPath();
	}

	if (systemPath == "")
	{
		LogError("Failed to read XML file \"%s\"", XMLFilename.str().c_str());
		return;
	}
	LogInfo("Loading XML file \"%s\" - found at \"%s\"", XMLFilename.str().c_str(), systemPath.str().c_str());

	doc.LoadFile( systemPath.str().c_str() );
	node = doc.RootElement();

	if (!node)
	{
		LogError("Failed to parse XML file \"%s\"", systemPath.str().c_str());
		return;
	}

	UString nodename = node->Name();

	if( nodename == "openapoc" )
	{
		for( node = node->FirstChildElement(); node != nullptr; node = node->NextSiblingElement() )
		{
			ApplyAliases( node );
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
			else if (nodename == "language")
			{
				supportedlanguages[node->Attribute("id")] = node->GetText();
			}
			else if (nodename == "alias")
			{
				aliases[UString(node->Attribute("id"))] = UString(node->GetText());
			}
			else if (nodename == "ufopaedia")
			{
				tinyxml2::XMLElement* nodeufo;
				for( nodeufo = node->FirstChildElement(); nodeufo != nullptr; nodeufo = nodeufo->NextSiblingElement() )
				{
					nodename = nodeufo->Name();
					if( nodename == "category" )
					{
						Ufopaedia::UfopaediaDB.push_back( std::make_shared<UfopaediaCategory>( fw, nodeufo ) );
					}
				}
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

void GameCore::ApplyAliases( tinyxml2::XMLElement* Source )
{
	if( aliases.empty() )
	{
		return;
	}

	const tinyxml2::XMLAttribute* attr = Source->FirstAttribute();

	while( attr != nullptr )
	{
		// Is the attribute value the same as an alias? If so, replace with alias' value
		if( aliases.find( UString(attr->Value()) ) != aliases.end() )
		{
			LogInfo("%s attribute \"%s\" value \"%s\" matches alias \"%s\"", Source->Name(), attr->Name(), attr->Value(), aliases[UString(attr->Value())].str().c_str() );
			Source->SetAttribute( attr->Name(), aliases[UString(attr->Value())].str().c_str() );
		}

		attr = attr->Next();
	}

	// Replace inner text
	if( Source->GetText() != nullptr && aliases.find( UString(Source->GetText()) ) != aliases.end() )
	{
		LogInfo("%s  value \"%s\" matches alias \"%s\"", Source->Name(), Source->GetText(), aliases[UString(Source->GetText())].str().c_str() );
		Source->SetText( aliases[UString(Source->GetText())].str().c_str() );
	}

	// Recurse down tree
	tinyxml2::XMLElement* child = Source->FirstChildElement();
	while( child != nullptr )
	{
		ApplyAliases( child );
		child = child->NextSiblingElement();
	}

}

}; //namespace OpenApoc


#pragma once
#include "library/sp.h"

#include "game/apocresources/apocresource.h"
#include "framework/font.h"
#include "forms/forms.h"

namespace OpenApoc
{

class GameCore
{
  private:
	std::map<UString, sp<BitmapFont>> fonts;
	std::map<UString, Form *> forms;
	std::map<UString, UString> aliases;

	void ApplyAliases(tinyxml2::XMLElement *Source);
	void ParseXMLDoc(UString XMLFilename);
	void ParseGameXML(tinyxml2::XMLElement *Source);
	void ParseStringXML(tinyxml2::XMLElement *Source);
	void ParseFormXML(tinyxml2::XMLElement *Source);

	// void ParseAlienXML( tinyxml2::XMLElement* Source );
	// void ParseInventoryXML( tinyxml2::XMLElement* Source );
	// void ParseOrganisationXML( tinyxml2::XMLElement* Source );
	// void ParseUFOXML( tinyxml2::XMLElement* Source );
	// void ParseUFOpaediaXML( tinyxml2::XMLElement* Source );
	// void ParseVehicleXML( tinyxml2::XMLElement* Source );

  public:
	bool Loaded;
	bool DebugModeEnabled;
	ApocCursor *MouseCursor;

	GameCore();
	void Load(UString CoreXMLFilename);
	~GameCore();

	Form *GetForm(UString ID);
	sp<Image> GetImage(UString ImageData);
	sp<BitmapFont> GetFont(UString FontData);
	sp<Palette> GetPalette(UString Path);

	std::vector<UString> GetFormIDs();
};

}; // namespace OpenApoc

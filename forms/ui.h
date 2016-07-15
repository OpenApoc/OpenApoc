#pragma once
#include "framework/font.h"
#include "library/sp.h"

namespace tinyxml2
{
class XMLElement;
} // namespace tinyxml2

namespace OpenApoc
{

class Form;

class UI
{
  private:
	static up<UI> instance;
	std::map<UString, sp<BitmapFont>> fonts;
	std::map<UString, sp<Form>> forms;
	std::map<UString, UString> aliases;
	UString resourceNodeNameFilter;

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
	void Load(UString CoreXMLFilename);

  public:
	UI();
	~UI();

	sp<Form> GetForm(UString ID);
	sp<BitmapFont> GetFont(UString FontData);

	std::vector<UString> GetFormIDs();

	static void unload();
	static UI &getInstance();

	void reloadFormsXml();
};

UI &ui();

}; // namespace OpenApoc

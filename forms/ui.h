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

	void applyAliases(tinyxml2::XMLElement *Source);
	void parseXmlDoc(UString XMLFilename);
	void parseGameXml(tinyxml2::XMLElement *Source);
	void parseStringXml(tinyxml2::XMLElement *Source);
	void parseFormXml(tinyxml2::XMLElement *Source);

	// void ParseAlienXML( tinyxml2::XMLElement* Source );
	// void ParseInventoryXML( tinyxml2::XMLElement* Source );
	// void ParseOrganisationXML( tinyxml2::XMLElement* Source );
	// void ParseUFOXML( tinyxml2::XMLElement* Source );
	// void ParseUFOpaediaXML( tinyxml2::XMLElement* Source );
	// void ParseVehicleXML( tinyxml2::XMLElement* Source );
	void load(UString CoreXMLFilename);

  public:
	UI();
	~UI();

	sp<Form> getForm(UString ID);
	sp<BitmapFont> getFont(UString FontData);

	std::vector<UString> getFormIDs();

	static void unload();
	static UI &getInstance();

	void reloadFormsXml();
};

UI &ui();

}; // namespace OpenApoc

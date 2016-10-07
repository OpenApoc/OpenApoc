#include "forms/ui.h"
#include "forms/forms.h"
#include "framework/apocresources/apocfont.h"
#include "framework/configfile.h"
#include "framework/data.h"
#include "framework/framework.h"
#include "framework/trace.h"
#include "library/sp.h"
#include <stdexcept>
#include <tinyxml2.h>

namespace OpenApoc
{

ConfigOptionString uiFormsPath("Game", "UIForms", "path to XML containing UI definitions",
                               "xcomapoc.xml");

up<UI> UI::instance = nullptr;

UI &UI::getInstance()
{
	if (!instance)
	{
		instance.reset(new UI);
		instance->load(uiFormsPath.get());
	}
	return *instance;
}

UI &ui() { return UI::getInstance(); };

void UI::unload() { instance.reset(nullptr); }

UI::UI() : fonts(), forms() {}

void UI::load(UString CoreXMLFilename) { parseXmlDoc(CoreXMLFilename); }

UI::~UI() = default;

void UI::parseXmlDoc(UString XMLFilename)
{
	TRACE_FN_ARGS1("XMLFilename", XMLFilename);
	tinyxml2::XMLDocument doc;
	tinyxml2::XMLElement *node;
	auto file = fw().data->fs.open(XMLFilename);
	if (!file)
	{
		LogError("Failed to open XML file \"%s\"", XMLFilename.cStr());
	}

	LogInfo("Loading XML file \"%s\" - found at \"%s\"", XMLFilename.cStr(),
	        file.systemPath().cStr());

	auto xmlText = file.readAll();
	if (!xmlText)
	{
		LogError("Failed to read in XML file \"%s\"", XMLFilename.cStr());
	}

	auto err = doc.Parse(xmlText.get(), file.size());

	if (err != tinyxml2::XML_SUCCESS)
	{
		LogError("Failed to parse XML file \"%s\" - \"%s\" \"%s\"", XMLFilename.cStr(),
		         doc.GetErrorStr1(), doc.GetErrorStr2());
		return;
	}

	node = doc.RootElement();

	if (!node)
	{
		LogError("Failed to parse XML file \"%s\" - no root element", XMLFilename.cStr());
		return;
	}

	UString nodeName = node->Name();

	if (nodeName == "openapoc")
	{
		for (node = node->FirstChildElement(); node != nullptr; node = node->NextSiblingElement())
		{
			applyAliases(node);
			nodeName = node->Name();

			if (nodeName == "game")
			{
				parseGameXml(node);
			}
			else
			{
				if (!resourceNodeNameFilter.empty())
				{ // skip other nodes if limited to certain resource types
					if (nodeName != resourceNodeNameFilter)
					{
						continue;
					}
				}

				if (nodeName == "form")
				{
					parseFormXml(node);
				}
				else if (nodeName == "apocfont")
				{
					UString fontName = node->Attribute("name");
					if (fontName == "")
					{
						LogError("apocfont element with no name");
						continue;
					}
					auto font = ApocalypseFont::loadFont(node);
					if (!font)
					{
						LogError("apocfont element \"%s\" failed to load", fontName.cStr());
						continue;
					}

					if (this->fonts.find(fontName) != this->fonts.end())
					{
						LogError("multiple fonts with name \"%s\"", fontName.cStr());
						continue;
					}
					this->fonts[fontName] = font;
				}
				else if (nodeName == "alias")
				{
					aliases[UString(node->Attribute("id"))] = UString(node->GetText());
				}
				else
				{
					LogError("Unknown XML element \"%s\"", nodeName.cStr());
				}
			}
		}
	}
}

void UI::parseGameXml(tinyxml2::XMLElement *Source)
{
	tinyxml2::XMLElement *node;
	UString nodename;

	for (node = Source->FirstChildElement(); node != nullptr; node = node->NextSiblingElement())
	{
		nodename = node->Name();
		if (nodename == "title")
		{
			fw().displaySetTitle(node->GetText());
		}
		if (nodename == "include")
		{
			parseXmlDoc(node->GetText());
		}
	}
}

void UI::parseFormXml(tinyxml2::XMLElement *Source)
{
	auto form = mksp<Form>();
	form->readFormStyle(Source);
	forms[Source->Attribute("id")] = form;
}

sp<Form> UI::getForm(UString ID)
{
	try
	{
		return std::dynamic_pointer_cast<Form>(forms.at(ID)->copyTo(nullptr));
	}
	catch (const std::out_of_range)
	{
		LogError("Missing form \"%s\"", ID.cStr());
		return nullptr;
	}
}

sp<BitmapFont> UI::getFont(UString FontData)
{
	if (fonts.find(FontData) == fonts.end())
	{
		LogError("Missing font \"%s\"", FontData.cStr());
		return nullptr;
	}
	return fonts[FontData];
}

void UI::applyAliases(tinyxml2::XMLElement *Source)
{
	if (aliases.empty())
	{
		return;
	}

	const tinyxml2::XMLAttribute *attr = Source->FirstAttribute();

	while (attr != nullptr)
	{
		// Is the attribute value the same as an alias? If so, replace with alias' value
		if (aliases.find(UString(attr->Value())) != aliases.end())
		{
			LogInfo("%s attribute \"%s\" value \"%s\" matches alias \"%s\"", Source->Name(),
			        attr->Name(), attr->Value(), aliases[UString(attr->Value())].cStr());
			Source->SetAttribute(attr->Name(), aliases[UString(attr->Value())].cStr());
		}

		attr = attr->Next();
	}

	// Replace inner text
	if (Source->GetText() != nullptr && aliases.find(UString(Source->GetText())) != aliases.end())
	{
		LogInfo("%s  value \"%s\" matches alias \"%s\"", Source->Name(), Source->GetText(),
		        aliases[UString(Source->GetText())].cStr());
		Source->SetText(aliases[UString(Source->GetText())].cStr());
	}

	// Recurse down tree
	tinyxml2::XMLElement *child = Source->FirstChildElement();
	while (child != nullptr)
	{
		applyAliases(child);
		child = child->NextSiblingElement();
	}
}

void UI::reloadFormsXml()
{
	forms.clear();
	resourceNodeNameFilter = "form";
	instance->load(config().getString("uiforms"));
	resourceNodeNameFilter = "";
}

std::vector<UString> UI::getFormIDs()
{
	std::vector<UString> result;

	for (auto idx = forms.begin(); idx != forms.end(); idx++)
	{
		result.push_back(idx->first);
	}

	return result;
}

}; // namespace OpenApoc

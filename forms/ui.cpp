#include "forms/ui.h"
#include "dependencies/pugixml/src/pugixml.hpp"
#include "forms/forms.h"
#include "framework/apocresources/apocfont.h"
#include "framework/configfile.h"
#include "framework/data.h"
#include "framework/framework.h"
#include "library/sp.h"
#include <stdexcept>

namespace OpenApoc
{

up<UI> UI::instance = nullptr;

UI &UI::getInstance()
{
	if (!instance)
	{
		instance.reset(new UI);
	}
	return *instance;
}

UI &ui() { return UI::getInstance(); };

void UI::unload() { instance.reset(nullptr); }

UI::UI() : fonts(), forms() {}

UI::~UI() = default;

sp<Form> UI::getForm(UString ID)
{
	if (forms.find(ID) == forms.end())
	{
		auto formPath = UString("forms/") + ID + ".form";

		LogInfo("Trying to load form \"%s\" from \"%s\"", ID, formPath);
		auto form = Form::loadForm(formPath);
		if (!form)
		{
			LogError("Failed to find form \"%s\" at \"%s\"", ID, formPath);
			return nullptr;
		}
		forms[ID] = form;
	}
	return std::dynamic_pointer_cast<Form>(forms[ID]->copyTo(nullptr));
}

sp<BitmapFont> UI::getFont(UString FontData)
{
	if (fonts.find(FontData) == fonts.end())
	{
		auto fontPath = UString("fonts/") + FontData + ".font";
		LogInfo("Trying to load font \"%s\" from \"%s\"", FontData, fontPath);
		auto font = ApocalypseFont::loadFont(fontPath);
		if (!font)
		{
			LogError("Failed to find font \"%s\" at \"%s\"", FontData, fontPath);
			return nullptr;
		}
		fonts[FontData] = font;
	}
	return fonts[FontData];
}

void UI::reloadFormsXml() { forms.clear(); }

std::vector<UString> UI::getFormIDs()
{
	auto formPaths = fw().data->fs.enumerateDirectoryRecursive("forms", ".form");
	std::vector<UString> trimmedPaths;
	for (auto &name : formPaths)
	{
		if (name.substr(0, 6) != "forms/")
		{
			LogWarning("Unexpected form file prefix for \"%s\"", name);
			continue;
		}
		if (!ends_with(name, ".form"))
		{
			LogWarning("Unexpected extension on form file \"%s\"", name);
			continue;
		}
		else
		{
			// Remove '.form' suffix and 'forms/' prefix
			trimmedPaths.push_back(name.substr(0, name.length() - 5).substr(6));
		}
	}
	return trimmedPaths;
}

}; // namespace OpenApoc

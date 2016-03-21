#include "game/rules/rules.h"
#include "game/rules/rules_private.h"
#include "framework/framework.h"
#include "framework/trace.h"
#include <tinyxml2.h>
#include "game/ufopaedia/ufopaedia.h"

namespace OpenApoc
{

bool RulesLoader::isValidEquipmentID(const UString &str)
{
	static UString id_prefix = "VEQUIP_";
	// This also catches empty string etc.?
	if (str.substr(0, id_prefix.length()) != id_prefix)
	{
		return false;
	}
	return true;
}
bool RulesLoader::isValidStringID(const UString &str)
{
	// With gettext everything is a valid string
	return true;
}
bool RulesLoader::isValidVehicleTypeID(const UString &str)
{
	static UString id_prefix = "VEHICLE_";
	// This also catches empty string etc.?
	if (str.substr(0, id_prefix.length()) != id_prefix)
	{
		return false;
	}
	return true;
}

Rules::Rules() {}

Rules::Rules(const UString &rootFileName) : aliases(new ResourceAliases())
{
	TRACE_FN_ARGS1("rootFileName", rootFileName);

	UString systemPath;
	auto file = fw().data->fs.open(rootFileName);
	if (!file)
	{
		LogError("Failed to find rule file \"%s\"", rootFileName.c_str());
		return;
	}

	auto xmlData = file.readAll();

	tinyxml2::XMLDocument doc;
	doc.Parse(xmlData.get(), file.size());
	tinyxml2::XMLElement *root = doc.RootElement();
	if (!root)
	{
		LogError("Failed to parse rule file \"%s\"", systemPath.c_str());
		return;
	}

	UString nodeName = root->Name();
	if (nodeName != "openapoc_rules")
	{
		LogError("Unexpected root node \"%s\" in \"%s\" - expected \"%s\"", nodeName.c_str(),
		         systemPath.c_str(), "openapoc_rules");
	}

	UString rulesetName = root->Attribute("name");
	LogInfo("Loading ruleset \"%s\" from \"%s\"", rulesetName.c_str(), systemPath.c_str());

	/* Wire up the resource aliases */
	fw().data->aliases = this->aliases;

	if (!RulesLoader::ParseRules(*this, root))
	{
		LogError("Error loading ruleset \"%s\" from \"%s\"", rulesetName.c_str(),
		         systemPath.c_str());
	}
}

bool RulesLoader::ParseRules(Rules &rules, tinyxml2::XMLElement *root)
{
	TRACE_FN;
	UString nodeName = root->Name();
	if (nodeName != "openapoc_rules")
	{
		LogError("Unexpected root node \"%s\" - expected \"%s\"", nodeName.c_str(),
		         "openapoc_rules");
		return false;
	}

	for (tinyxml2::XMLElement *e = root->FirstChildElement(); e != nullptr;
	     e = e->NextSiblingElement())
	{
		UString name = e->Name();
		if (name == "ufopaedia")
		{
			tinyxml2::XMLElement *nodeufo;
			UString nodename;
			for (nodeufo = e->FirstChildElement(); nodeufo != nullptr;
			     nodeufo = nodeufo->NextSiblingElement())
			{
				nodename = nodeufo->Name();
				if (nodename == "category")
				{
					Ufopaedia::UfopaediaDB.push_back(mksp<UfopaediaCategory>(nodeufo));
				}
			}
		}
		else if (name == "include")
		{
			UString rootFileName = e->GetText();
			UString systemPath;
			auto file = fw().data->fs.open(rootFileName);
			if (!file)
			{
				LogError("Failed to find included rule file \"%s\"", rootFileName.c_str());
				return false;
			}
			auto xmlData = file.readAll();
			TRACE_FN_ARGS1("include", systemPath);
			LogInfo("Loading included ruleset from \"%s\"", systemPath.c_str());
			tinyxml2::XMLDocument doc;
			doc.Parse(xmlData.get(), file.size());
			tinyxml2::XMLElement *incRoot = doc.RootElement();
			if (!incRoot)
			{
				LogError("Failed to parse included rule file \"%s\"", systemPath.c_str());
				return false;
			}
			if (!ParseRules(rules, incRoot))
			{
				LogError("Error loading included ruleset \"%s\"", systemPath.c_str());
				return false;
			}
		}
		if (name == "aliases")
		{
			if (!ParseAliases(rules, e))
				return false;
		}
		else
		{
			LogError("Unexpected node \"%s\"", name.c_str());
			return false;
		}
	}

	return true;
}

bool Rules::isValid()
{
	// FIXME: Pull out validation for after loading in here
	return true;
}

}; // namespace OpenApoc

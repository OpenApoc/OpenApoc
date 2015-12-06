#include "game/rules/rules_private.h"
#include "framework/logger.h"
#include "rules_helper.h"

namespace OpenApoc
{

bool RulesLoader::ParseFacilityDefinition(Framework &fw, Rules &rules, tinyxml2::XMLElement *root)
{
	TRACE_FN;
	std::ignore = fw;
	FacilityDef def;

	if (UString(root->Name()) != "facilitydef")
	{
		LogError("Called on unexpected node \"%s\"", root->Name());
		return false;
	}

	def.id = root->Attribute("id");
	def.name = root->Attribute("name");

	for (auto node = root->FirstChildElement(); node != nullptr; node = node->NextSiblingElement())
	{
		UString tag = node->Name();
		if (tag == "fixed")
		{
			if (node->QueryBoolText(&def.fixed) != tinyxml2::XMLError::XML_SUCCESS)
			{
				LogError("Failed to read facility 'fixed' attribute");
				return false;
			}
		}
		else if (tag == "buildCost")
		{
			if (node->QueryIntText(&def.buildCost) != tinyxml2::XMLError::XML_SUCCESS)
			{
				LogError("Failed to read facility 'buildCost' attribute");
				return false;
			}
		}
		else if (tag == "buildTime")
		{
			if (node->QueryIntText(&def.buildTime) != tinyxml2::XMLError::XML_SUCCESS)
			{
				LogError("Failed to read facility 'buildTime' attribute");
				return false;
			}
		}
		else if (tag == "weeklyCost")
		{
			if (node->QueryIntText(&def.weeklyCost) != tinyxml2::XMLError::XML_SUCCESS)
			{
				LogError("Failed to read facility 'weeklyCost' attribute");
				return false;
			}
		}
		else if (tag == "capacityType")
		{
			if (!ReadElement(node,
			                 std::map<UString, FacilityDef::Capacity>{
			                     {"none", FacilityDef::Capacity::None},
			                     {"quarters", FacilityDef::Capacity::Quarters},
			                     {"stores", FacilityDef::Capacity::Stores},
			                     {"medical", FacilityDef::Capacity::Medical},
			                     {"training", FacilityDef::Capacity::Training},
			                     {"psi", FacilityDef::Capacity::Psi},
			                     {"repair", FacilityDef::Capacity::Repair},
			                     {"chemistry", FacilityDef::Capacity::Chemistry},
			                     {"physics", FacilityDef::Capacity::Physics},
			                     {"workshop", FacilityDef::Capacity::Workshop},
			                     {"aliens", FacilityDef::Capacity::Aliens}},
			                 def.capacityType))
			{
				LogError("Failed to read facility 'capacityType' attribute");
				return false;
			}
		}
		else if (tag == "capacityAmount")
		{
			if (node->QueryIntText(&def.capacityAmount) != tinyxml2::XMLError::XML_SUCCESS)
			{
				LogError("Failed to read facility 'capacityAmount' attribute");
				return false;
			}
		}
		else if (tag == "sprite")
		{
			def.sprite = node->GetText();
		}
		else if (tag == "size")
		{
			if (node->QueryIntText(&def.size) != tinyxml2::XMLError::XML_SUCCESS)
			{
				LogError("Failed to read facility 'size' attribute");
				return false;
			}
		}
		else
		{
			LogWarning("Unknown facility tag \"%s\"", tag.c_str());
		}
	}

	rules.facilities.emplace(def.id, def);

	return true;
}
}; // namespace OpenApoc

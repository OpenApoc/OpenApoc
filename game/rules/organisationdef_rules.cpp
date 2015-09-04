#include "game/rules/rules_private.h"
#include "framework/logger.h"

namespace OpenApoc
{

bool RulesLoader::ParseOrganisationDefinition(Framework &fw, Rules &rules,
                                              tinyxml2::XMLElement *root)
{
	std::ignore = fw;
	if (UString(root->Name()) != "organisation") {
		LogError("Called on unexpected node \"%s\"", root->Name());
		return false;
	}

	OrganisationDef def;

	def.name = root->GetText();

	rules.organisations.push_back(def);

	LogInfo("Organisation \"%s\" at idx %d", def.name.str().c_str(), rules.organisations.size());

	return true;
}
} // namespace OpenApoc

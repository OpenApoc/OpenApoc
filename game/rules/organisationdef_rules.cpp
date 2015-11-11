#include "game/rules/rules_private.h"
#include "game/rules/rules_helper.h"
#include "framework/logger.h"

namespace OpenApoc
{

bool RulesLoader::ParseOrganisationDefinition(Framework &fw, Rules &rules,
                                              tinyxml2::XMLElement *root)
{
	TRACE_FN;
	std::ignore = fw;
	if (UString(root->Name()) != "organisation")
	{
		LogError("Called on unexpected node \"%s\"", root->Name());
		return false;
	}

	Organisation org;

	if (!ReadAttribute(root, "ID", org.ID))
	{
		LogError("Organisation with no ID");
		return false;
	}

	if (org.ID.substr(0, 4) != "ORG_")
	{
		LogError("Organisation ID \"%s\" doesn't start with \"ORG_\"", org.ID.c_str());
		return false;
	}

	if (!ReadAttribute(root, "name", org.name))
	{
		LogError("Organisation ID \"%s\" has no name", org.ID.c_str());
		return false;
	}

	ReadAttribute(root, "balance", org.balance, 0);
	ReadAttribute(root, "income", org.income, 0);

	rules.organisations.push_back(org);

	return true;
}
}; // namespace OpenApoc

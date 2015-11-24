#include "library/sp.h"
#include "game/rules/rules_private.h"
#include "game/rules/rules_helper.h"
#include "framework/logger.h"
#include "framework/framework.h"

namespace OpenApoc
{
bool RulesLoader::ParseAliases(Framework &fw, Rules &rules, tinyxml2::XMLElement *root)
{
	TRACE_FN;
	if (UString(root->Name()) != "aliases")
	{
		LogError("Called on unexpected node \"%s\"", root->Name());
		return false;
	}

	for (tinyxml2::XMLElement *e = root->FirstChildElement(); e != nullptr;
	     e = e->NextSiblingElement())
	{
		UString name = e->Name();
		if (name == "sample")
		{
			UString match, replacement;
			if (!ReadAttribute(e, "match", match))
			{
				LogError("Sample alias has no \"match\"");
				return false;
			}
			if (!ReadAttribute(e, "replacement", replacement))
			{
				LogError("Sample alias has no \"replacement\"");
				return false;
			}

			auto it = rules.aliases->sample.find(match);
			if (it != rules.aliases->sample.end())
			{
				LogWarning("Replacing existing sample alias match \"%s\" replacement \"%s\" with "
				           "replacement \"%s\"",
				           it->first.c_str(), it->second.c_str(), replacement.c_str());
			}
			else
			{
				LogInfo("Adding new sample alias match \"%s\" replacement \"%s\"", match.c_str(),
				        replacement.c_str());
			}
			rules.aliases->sample[match] = replacement;
		}
		else
		{
			LogError("Unexpected node \"%s\"", name.c_str());
			return false;
		}
	}
	return true;
}
}; // namespace OpenApoc

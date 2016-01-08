#include "game/rules/doodaddef.h"
#include "game/rules/rules_private.h"
#include "game/rules/rules_helper.h"
#include "framework/logger.h"
#include "framework/framework.h"

namespace OpenApoc
{

static bool parseFrame(tinyxml2::XMLElement *root, DoodadFrame &f)
{
	if (UString(root->Name()) != "frame")
	{
		LogError("Called on unexpected node \"%s\"", root->Name());
		return false;
	}
	if (!ReadAttribute(root, "time", f.time))
	{
		LogError("No \"time\" attribute in frame");
		return false;
	}
	UString spritePath = root->GetText();
	f.image = fw().gamecore->GetImage(spritePath);
	if (!f.image)
	{
		LogError("Failed to load sprite \"%s\" for doodad frame", spritePath.c_str());
		return false;
	}
	return true;
}

bool RulesLoader::ParseDoodadDefinition(Rules &rules, tinyxml2::XMLElement *root)
{
	TRACE_FN;
	DoodadDef d;
	if (UString(root->Name()) != "doodad")
	{
		LogError("Called on unexpected node \"%s\"", root->Name());
		return false;
	}
	if (!ReadAttribute(root, "id", d.ID))
	{
		LogError("No \"id\" in doodad");
		return false;
	}
	if (!ReadAttribute(root, "lifetime", d.lifetime))
	{
		LogError("No \"lifetime\" in doodad \"%s\"", d.ID.c_str());
		return false;
	}

	if (!ReadAttribute(root, "imageOffsetX", d.imageOffset.x))
	{
		LogError("No \"imageOffsetX\" in doodad ID \"%s\"", d.ID.c_str());
		return false;
	}
	if (!ReadAttribute(root, "imageOffsetY", d.imageOffset.y))
	{
		LogError("No \"imageOffsetY\" in doodad ID \"%s\"", d.ID.c_str());
		return false;
	}
	for (tinyxml2::XMLElement *node = root->FirstChildElement(); node != nullptr;
	     node = node->NextSiblingElement())
	{
		if (UString(node->Name()) == "frame")
		{
			DoodadFrame f;
			if (!parseFrame(node, f))
			{
				LogError("Error parsing frame for doodad \"%s\"", d.ID.c_str());
				return false;
			}
			d.frames.emplace_back(f);
		}
	}
	if (d.frames.empty())
	{
		LogError("Doodad \"%s\" has no frames?", d.ID.c_str());
		return false;
	}

	if (rules.doodads.find(d.ID) != rules.doodads.end())
	{
		LogError("Multiple doodads with ID \"%s\"", d.ID.c_str());
		return false;
	}
	rules.doodads.emplace(d.ID, d);
	return true;
}
}

#include "library/sp.h"
#include "game/rules/rules_private.h"
#include "library/strings.h"
#include "framework/framework.h"
#include "game/tileview/voxel.h"
#include "game/rules/rules_helper.h"

#include <glm/glm.hpp>
#include <map>

namespace OpenApoc
{

static std::map<VehicleDefinition::Direction, Vec3<float>> directionsToVec = {
    {VehicleDefinition::Direction::N, {0, -1, 0}}, {VehicleDefinition::Direction::NE, {1, -1, 0}},
    {VehicleDefinition::Direction::E, {1, 0, 0}},  {VehicleDefinition::Direction::SE, {1, 1, 0}},
    {VehicleDefinition::Direction::S, {0, 1, 0}},  {VehicleDefinition::Direction::SW, {-1, 1, 0}},
    {VehicleDefinition::Direction::W, {-1, 0, 0}}, {VehicleDefinition::Direction::NW, {-1, -1, 0}},
};

static std::map<VehicleDefinition::Direction, sp<Image>>
parseDirectionalSprites(Framework &fw, tinyxml2::XMLElement *root)
{
	std::map<VehicleDefinition::Direction, sp<Image>> sprites;

	for (tinyxml2::XMLElement *node = root->FirstChildElement(); node != nullptr;
	     node = node->NextSiblingElement())
	{
		UString name = node->Name();
		VehicleDefinition::Direction dir;
		if (name == "N")
			dir = VehicleDefinition::Direction::N;
		else if (name == "NNE")
			dir = VehicleDefinition::Direction::NNE;
		else if (name == "NE")
			dir = VehicleDefinition::Direction::NE;
		else if (name == "NEE")
			dir = VehicleDefinition::Direction::NEE;
		else if (name == "E")
			dir = VehicleDefinition::Direction::E;
		else if (name == "SEE")
			dir = VehicleDefinition::Direction::SEE;
		else if (name == "SE")
			dir = VehicleDefinition::Direction::SE;
		else if (name == "SSE")
			dir = VehicleDefinition::Direction::SSE;
		else if (name == "S")
			dir = VehicleDefinition::Direction::S;
		else if (name == "SSW")
			dir = VehicleDefinition::Direction::SSW;
		else if (name == "SW")
			dir = VehicleDefinition::Direction::SW;
		else if (name == "SWW")
			dir = VehicleDefinition::Direction::SWW;
		else if (name == "W")
			dir = VehicleDefinition::Direction::W;
		else if (name == "NWW")
			dir = VehicleDefinition::Direction::NWW;
		else if (name == "NW")
			dir = VehicleDefinition::Direction::NW;
		else if (name == "NNW")
			dir = VehicleDefinition::Direction::NNW;
		else
		{
			LogError("Unknown sprite direction \"%s\"", name.c_str());
			continue;
		}

		UString spriteName = node->GetText();
		LogInfo("Loading image \"%s\"", spriteName.c_str());
		if (sprites[dir])
			LogWarning("Replacing directional sprite");
		auto sprite = fw.gamecore->GetImage(spriteName);

		if (!sprite)
			LogError("Failed to load directional sprite \"%s\"", spriteName.c_str());
		sprites[dir] = sprite;
	}
	return sprites;
}

bool RulesLoader::ParseVehicleDefinition(Framework &fw, Rules &rules, tinyxml2::XMLElement *root)
{
	TRACE_FN;
	VehicleDefinition def;

	if (UString(root->Name()) != "vehicledef")
	{
		LogError("Called on unexpected node \"%s\"", root->Name());
		return false;
	}

	if (!ReadAttribute(root, "id", def.ID))
	{
		LogError("No \"id\" in vehicle");
		return false;
	}

	if (!ReadAttribute(root, "name", def.name))
	{
		LogError("No \"name\" in vehicle ID \"%s\"", def.ID.c_str());
		return false;
	}

	if (!ReadAttribute(root, "manufacturer", def.manufacturer))
	{
		LogError("No \"manufacturer\" in vehicle ID \"%s\"", def.ID.c_str());
		return false;
	}

	if (!ReadAttribute(
	        root, "type",
	        std::map<UString, VehicleDefinition::Type>{{"flying", VehicleDefinition::Type::Flying},
	                                                   {"ground", VehicleDefinition::Type::Ground}},
	        def.type))
	{
		LogWarning("Failed to read vehicle 'type' attribute");
		return false;
	}
	if (!ReadAttribute(root, "sizeX", def.size.x))
	{
		LogError("No \"sizeX\" in vehicle ID \"%s\"", def.ID.c_str());
		return false;
	}
	if (!ReadAttribute(root, "sizeY", def.size.y))
	{
		LogError("No \"sizeY\" in vehicle ID \"%s\"", def.ID.c_str());
		return false;
	}
	if (!ReadAttribute(root, "sizeZ", def.size.z))
	{
		LogError("No \"sizeZ\" in vehicle ID \"%s\"", def.ID.c_str());
		return false;
	}
	if (!ReadAttribute(root, "imageOffsetX", def.imageOffset.x))
	{
		LogError("No \"imageOffsetX\" in vehicle ID \"%s\"", def.ID.c_str());
		return false;
	}
	if (!ReadAttribute(root, "imageOffsetY", def.imageOffset.y))
	{
		LogError("No \"imageOffsetY\" in vehicle ID \"%s\"", def.ID.c_str());
		return false;
	}

	if (!ReadAttribute(root, "shadowOffsetX", def.shadowOffset.x))
	{
		LogError("No \"shadowOffsetX\" in vehicle ID \"%s\"", def.ID.c_str());
		return false;
	}
	if (!ReadAttribute(root, "shadowOffsetY", def.shadowOffset.y))
	{
		LogError("No \"shadowOffsetY\" in vehicle ID \"%s\"", def.ID.c_str());
		return false;
	}

	for (tinyxml2::XMLElement *node = root->FirstChildElement(); node != nullptr;
	     node = node->NextSiblingElement())
	{
		UString tag = node->Name();
		if (tag == "flat")
		{
			def.sprites[VehicleDefinition::Banking::Flat] = parseDirectionalSprites(fw, node);
		}
		else if (tag == "ascending")
		{
			def.sprites[VehicleDefinition::Banking::Ascending] = parseDirectionalSprites(fw, node);
		}
		else if (tag == "decending")
		{
			def.sprites[VehicleDefinition::Banking::Decending] = parseDirectionalSprites(fw, node);
		}
		else if (tag == "banking_left")
		{
			def.sprites[VehicleDefinition::Banking::Left] = parseDirectionalSprites(fw, node);
		}
		else if (tag == "banking_right")
		{
			def.sprites[VehicleDefinition::Banking::Right] = parseDirectionalSprites(fw, node);
		}
		else if (tag == "stratmap")
		{
			def.strategySprites = parseDirectionalSprites(fw, node);
			LogWarning("Read %d strategy sprites", def.strategySprites.size());
		}
		else if (tag == "shadow")
		{
			def.shadowSprites = parseDirectionalSprites(fw, node);
			LogWarning("Read %d shadow sprites", def.strategySprites.size());
		}
		else
		{
			LogError("Unknown vehicle tag \"%s\"", tag.c_str());
			continue;
		}
	}
	// Push all directional sprites into 'directional vector' space
	// FIXME: How to do banking left/right?
	for (auto &s : def.sprites[VehicleDefinition::Banking::Flat])
	{
		Vec3<float> v = directionsToVec[s.first];
		v = glm::normalize(v);
		def.directionalSprites.emplace_back(v, s.second);
	}
	for (auto &s : def.sprites[VehicleDefinition::Banking::Ascending])
	{
		Vec3<float> v = directionsToVec[s.first];
		v.z = 1;
		v = glm::normalize(v);
		def.directionalSprites.emplace_back(v, s.second);
	}
	for (auto &s : def.sprites[VehicleDefinition::Banking::Decending])
	{
		Vec3<float> v = directionsToVec[s.first];
		v.z = -1;
		v = glm::normalize(v);
		def.directionalSprites.emplace_back(v, s.second);
	}

	for (auto &s : def.strategySprites)
	{
		Vec3<float> v = directionsToVec[s.first];
		v = glm::normalize(v);
		def.directionalStrategySprites.emplace_back(v, s.second);
	}

	for (auto &s : def.shadowSprites)
	{
		Vec3<float> v = directionsToVec[s.first];
		v = glm::normalize(v);
		def.directionalShadowSprites.emplace_back(v, s.second);
	}

	if (!def.voxelMap)
	{
		static std::weak_ptr<VoxelMap> stubVoxelMap;
		LogWarning("Using stub voxel map for vehicle \"%s\"", def.name.c_str());
		def.voxelMap = stubVoxelMap.lock();
		if (!def.voxelMap)
		{
			def.voxelMap = std::make_shared<VoxelMap>(Vec3<int>{32, 32, 16});
			stubVoxelMap = def.voxelMap;
		}
	}

	rules.vehicleDefs.emplace(def.name, def);

	return true;
}

}; // namespace OpenApoc

#include "game/rules/rules_private.h"
#include "framework/logger.h"
#include "framework/framework.h"
#include "game/tileview/voxel.h"


struct citymap_dat_chunk
{
        uint16_t unknown1[8];
        uint8_t voxelIdx[16];
        uint8_t unknown[20];

};
static_assert(sizeof(struct citymap_dat_chunk) == 52, "citymap data chunk wrong size");

namespace OpenApoc
{
namespace 
{
	bool LoadCityMap(Framework &fw, Vec3<int> size, const UString &fileName, std::vector<int> &tileIndices)
	{
		auto file = fw.data->load_file(fileName);
		if (!file)
		{
			LogError("Failed to open file \"%s\"", fileName.str().c_str());
			return false;
		}

		size_t tileCount = size.x * size.y * size.z;

		if (file.size() != 2*tileCount)
		{
			LogError("Unexpected file size %zu - expected %zu for a {%d,%d,%d} city",
				file.size(), 2*tileCount, size.x, size.y, size.z);
			return false;
		}

		tileIndices.resize(tileCount);

		for (size_t i = 0; i < tileCount; i++)
		{
			uint16_t val;
			if (!file.readule16(val))
			{
				LogError("Unexpected EOF at tile %zu", i);
				return false;
			}
			tileIndices[i] = val;
		}


		return true;
	}

}; //anonymous namespace
	bool
	RulesLoader::ParseCityDefinition(Framework &fw, Rules &rules, tinyxml2::XMLElement *root)
	{
		if (UString(root->Name()) != "city")
		{
			LogError("Called on unexpected node \"%s\"", root->Name());
			return false;
		}

		bool loadedMap = false;
		bool loadedBuildings = false;
		bool loadedTiles = false;


		for (tinyxml2::XMLElement *e = root->FirstChildElement();
			e != nullptr;
			e = e->NextSiblingElement())
		{
			UString name = e->Name();
			if (name == "map")
			{
				if (loadedMap)
				{
					LogError("Multiple map nodes read");
					return false;
				}
				int sizeX, sizeY, sizeZ;
				auto error = e->QueryIntAttribute("sizeX", &sizeX);
				if (error != tinyxml2::XML_SUCCESS)
				{
					LogError("Map has no sizeX");
					return false;
				}
				error = e->QueryIntAttribute("sizeY", &sizeY);
				if (error != tinyxml2::XML_SUCCESS)
				{
					LogError("Map has no sizeY");
					return false;
				}
				error = e->QueryIntAttribute("sizeZ", &sizeZ);
				if (error != tinyxml2::XML_SUCCESS)
				{
					LogError("Map has no sizeZ");
					return false;
				}
				Vec3<int> size{sizeX,sizeY,sizeZ};
				//FIXME: Allow sizes greater than 100x100x10
				if (size.x < 0 || size.x > 100
				 || size.y < 0 || size.y > 100
				 || size.z < 0 || size.z > 10)
				{
					LogError("Invalid map size {%d,%d,%d}", size.x, size.y, size.z);
					return false;
				}
				if (!LoadCityMap(fw, size, e->GetText(), rules.tileIndices))
				{
					LogError("Error parsing map \"%s\"", e->GetText());
					return false;
				}
				rules.citySize = size;
				loadedMap = true;
			}
			else if (name == "buildings")
			{
				if (loadedBuildings)
				{
					LogError("Multiple buildings nodes read");
					return false;
				}
				//TODO: Building loading
				LogError("FIXME: No buildings implemented (yet)");
				loadedBuildings = true;
			}
			else if (name == "tiles")
			{
				if (loadedTiles)
				{
					LogError("Multiple tiles nodes read");
					return false;
				}
				int count;
				auto error = e->QueryIntAttribute("count", &count);
				if (error != tinyxml2::XML_SUCCESS)
				{
					LogError("Tile list has no count");
					return false;
				}
				if (count < 0)
				{
					LogError("Invalid tile count %d", count);
					return false;
				}
				UString sprite = e->Attribute("sprites");
				if (sprite == "")
				{
					LogError("Tile list has no sprite set");
					return false;
				}
				auto spriteSet = fw.data->load_image_set(sprite);
				if (!spriteSet)
				{
					LogError("Failed to load sprite set \"%s\"", sprite.str().c_str());
					return false;
				}
				if (spriteSet->images.size() < (unsigned)count)
				{
					LogError("Tile count %d but only %zu entries in sprite set \"%s\"",
						count, spriteSet->images.size(), sprite.str().c_str());
					return false;
				}

				UString datPath = e->GetText();
				if (datPath == "")
				{
					LogError("Empty tile list declaration?");
					return false;
				}
				auto datFile = fw.data->load_file(datPath);
				if (!datFile)
				{
					LogError("Failed to open file list file \"%s\"",
						datPath.str().c_str());
					return false;
				}
				if (datFile.size() != sizeof(struct citymap_dat_chunk)*count)
				{
					LogError("Unexpected tile data file size - expected %zu for %d entries but got %zu",
						sizeof(citymap_dat_chunk)*count, count, datFile.size());
					return false;
				}
				//FIXME: Read loftemps
				for (int i = 0; i < count; i++)
				{
					BuildingTileDef t;
					t.sprite = spriteSet->images[i];
					// All city tiles are 32x32x16 voxels
					t.voxelMap = std::make_shared<VoxelMap>(Vec3<int>{32,32,16});
					rules.buildingTiles.push_back(t);
				}
			}
			else
			{
				LogError("Unexpected node \"%s\"", name.str().c_str());
				return false;
			}
		}


		if (!loadedMap)
		{
			LogError("No map specified in city");
			return false;
		}
		if (!loadedBuildings)
		{
			LogError("No buildings specified in city");
			return false;
		}
		if (!loadedTiles)
		{
			LogError("No tiles specified in city");
			return false;
		}

		return true;
	}
}; //namespace OpenApoc

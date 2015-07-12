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
	bool LoadCityMap(Framework &fw, Vec3<int> size, tinyxml2::XMLElement *root, std::vector<UString> &tileIDs)
	{
		size_t tileCount = size.x * size.y * size.z;

		tileIDs.resize(tileCount);

		LogInfo("Loaded city of size {%d,%d,%d}", size.x, size.y, size.z);

		return true;
	}

	bool LoadCityTile(Framework &fw, tinyxml2::XMLElement *root, UString &tileID, std::shared_ptr<Image> &sprite, std::shared_ptr<VoxelMap> &voxelMap)
	{
		std::shared_ptr<Image> readSprite = nullptr;
		std::shared_ptr<VoxelMap> readVoxelMap = nullptr;

		int numVoxelLayers = 0;

		if (UString(root->Name()) != "tile")
		{
			LogError("Called on unexpected node \"%s\"", root->Name());
			return false;
		}

		//FIXME: Check 'idx' to allow out-of-order (or a non-int id?)
		tileID = root->Attribute("id");
		if (tileID == "")
		{
			LogError("Tile with no ID");
			return false;
		}
		UString spriteString = root->Attribute("sprite");

		if (spriteString == "")
		{
			LogError("No sprite in tile");
			return false;
		}

		readSprite = fw.data->load_image(spriteString);
		if (!readSprite)
		{
			LogError("Failed to load image \"%s\"", spriteString.str().c_str());
			return false;
		}

		for (tinyxml2::XMLElement *e = root->FirstChildElement();
			e != nullptr;
			e = e->NextSiblingElement())
		{
			UString name = e->Name();
			if (name == "lof")
			{
				int voxelSizeX = 0, voxelSizeY = 0, voxelSizeZ = 0;
				auto err = e->QueryIntAttribute("sizeX", &voxelSizeX);
				if (err != tinyxml2::XML_SUCCESS)
				{
					LogError("Failed to load voxel \"sizeX\" attribute");
					return false;
				}
				err = e->QueryIntAttribute("sizeY", &voxelSizeY);
				if (err != tinyxml2::XML_SUCCESS)
				{
					LogError("Failed to load voxel \"sizeY\" attribute");
					return false;
				}
				err = e->QueryIntAttribute("sizeZ", &voxelSizeZ);
				if (err != tinyxml2::XML_SUCCESS)
				{
					LogError("Failed to load voxel \"sizeZ\" attribute");
					return false;
				}
				//FIXME: Support non-32x32x16 tile voxels?
				//Would that then be 'scaled' into a single tile, or allow larger/smaller tiles?
				//Would cause a lot of effort for little obvious use
				if (voxelSizeX != 32 || voxelSizeY != 32 || voxelSizeZ != 16)
				{
					LogError("Unexpected LOF voxel size {%d,%d,%d} - expected {32,32,16}",
						voxelSizeX, voxelSizeY, voxelSizeZ);
					return false;
				}
				
				readVoxelMap.reset(new VoxelMap(Vec3<int>{voxelSizeX, voxelSizeY, voxelSizeZ}));
				


				for (tinyxml2::XMLElement *layer = e->FirstChildElement();
					layer != nullptr;
					layer = layer->NextSiblingElement())
				{
					UString layerName = layer->Name();
					if (layerName != "loflayer")
					{
						LogError("Unexpected node \"%s\" - expected \"loflayer\"",
							name.str().c_str());
						return false;

					}
					if (numVoxelLayers >= voxelSizeZ)
					{
						LogError("Too many lof layers specified (sizeZ %d)",
							voxelSizeZ);
						return false;
					}
					UString voxelSliceString = layer->GetText();
					if (voxelSliceString == "")
					{
						LogError("No loflayer specified");
						return false;
					}
					auto lofSlice = fw.data->load_voxel_slice(voxelSliceString);
					if (!lofSlice)
					{
						LogError("Failed to load loflayer \"%s\"",
							voxelSliceString.str().c_str());
						return false;
					}
					if (lofSlice->getSize().x != voxelSizeX
					 || lofSlice->getSize().y != voxelSizeY)
					{
						LogError("Voxel slice size {%d,%d} in {%d,%d} tile",
							lofSlice->getSize().x, lofSlice->getSize().y,
							voxelSizeX, voxelSizeY);
						return false;
					}
					readVoxelMap->setSlice(numVoxelLayers, lofSlice);
					numVoxelLayers++;
				}
			}
			else
			{
				LogError("Unexpected node \"%s\" - expected \"lof\"",
					name.str().c_str());
				return false;
			}
		}

		if (!readSprite)
		{
			LogError("Tile with no sprite");
			return false;
		}
		if (!readVoxelMap)
		{
			LogError("Tile with no voxel map");
			return false;
		}

		sprite = readSprite;
		voxelMap = readVoxelMap;

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

		for (tinyxml2::XMLElement *e = root->FirstChildElement();
			e != nullptr;
			e = e->NextSiblingElement())
		{
			UString name = e->Name();
			if (name == "map")
			{
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
				if (!LoadCityMap(fw, size, e, rules.tileIDs))
				{
					LogError("Error parsing map \"%s\"", e->GetText());
					return false;
				}
				rules.citySize = size;
			}
			else if (name == "buildings")
			{
				//TODO: Building loading
				LogError("FIXME: No buildings implemented (yet)");
			}
			else if (name == "tiles")
			{
				int count;
				int numRead = 0;
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
				for (tinyxml2::XMLElement *tile = e->FirstChildElement();
					tile != nullptr;
					tile = tile->NextSiblingElement())
				{
					UString tileNodeName = tile->Name();
					if (tileNodeName == "tile")
					{
						UString tileID = "";
						numRead++;
						if (numRead > count)
						{
							LogError("Skipping tile %d (%d listed in count)",
								numRead, count);
							continue;
						}
						BuildingTileDef def;

						if (!LoadCityTile(fw, tile, tileID, def.sprite, def.voxelMap))
						{
							LogError("Error loading tile %d", numRead);
							return false;
						}
					}
					else
					{
						LogError("Unexpected node \"%s\" (expected \"tile\")", tileNodeName.str().c_str());
						return false;
					}

				}
				if (numRead != count)
				{
					LogError("Tile list count is %d but only read %d",
						count, numRead);
				}
				else
				{
					LogInfo("Loaded %d tiles", count);
				}
			}
			else
			{
				LogError("Unexpected node \"%s\"", name.str().c_str());
				return false;
			}
		}
		return true;
	}
}; //namespace OpenApoc

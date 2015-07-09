#include "game/city/buildingtile.h"
#include "framework/palette.h"
#include "game/apocresources/pck.h"
#include "framework/framework.h"
#include "game/tileview/voxel.h"
#include "game/apocresources/loftemps.h"

namespace OpenApoc {

namespace {

	struct citymap_dat_chunk
	{
		uint16_t unknown1[8];
		uint8_t voxelIdx[16];
		uint8_t unknown[20];

	};

	static_assert(sizeof(struct citymap_dat_chunk) == 52, "citymap data chunk wrong size");
}; //anonymous namespace


std::vector<CityTile>
CityTile::loadTilesFromFile(Framework &fw)
{
	std::vector<CityTile> v;

	auto pal = fw.data->load_palette("xcom3/ufodata/PAL_04.DAT");

	auto sprites = fw.data->load_image_set("PCK:xcom3/ufodata/CITY.PCK:xcom3/ufodata/CITY.TAB");

	auto datFile = fw.data->load_file("xcom3/ufodata/CITYMAP.DAT");
	if (!datFile)
	{
		LogError("Failed to load CITYMAP.DAT");
		return v;
	}

	auto lofDat = fw.data->load_file("xcom3/ufodata/LOFTEMPS.DAT");
	if (!lofDat)
	{
		LogError("Failed to load ufodata/loftemps.dat");
		return v;
	}

	auto lofTab = fw.data->load_file("xcom3/ufodata/LOFTEMPS.TAB");
	if (!lofTab)
	{
		LogError("Failed to load ufodata/loftemps.tab");
		return v;
	}

	LOFTemps lofTemps(lofDat, lofTab);

	int numTiles = sprites->images.size();

	auto datFileSize = datFile.size();

	int numDatEntries = datFileSize / 52;

	if (numDatEntries != numTiles)
	{
		LogError("Number of city sprite tiles does not match number of dat entries (%d dat chunks, %d images)",
			numDatEntries, numTiles);
	}

	LogInfo("Loading %d city tiles", numTiles);

	for (int t = 0; t < numTiles; t++)
	{
		CityTile tile;
		tile.sprite = sprites->images[t];

		struct citymap_dat_chunk tileData;
		datFile.seekg(sizeof(struct citymap_dat_chunk) * t, std::ios::beg);
		if (!datFile)
		{
			LogError("Failed to seek to citymap chunk %d", t);
			return v;
		}

		datFile.read((char*)&tileData, sizeof(tileData));
		if (!datFile)
		{
			LogError("Failed to read citymap chunk %d", t);
			return v;
		}

		/* All city tiles are 32x32x16 voxels */
		tile.voxelMap.reset(new VoxelMap(Vec3<int>{32,32,16}));

		for (int i = 0; i < 16; i++)
		{
			
			tile.voxelMap->setSlice(i, lofTemps.getSlice(tileData.voxelIdx[i]));
		}
		v.push_back(tile);
	}
	return v;
}

BuildingSection::BuildingSection(TileMap &map, CityTile &cityTile, Vec3<int> pos, Building *building)
	: TileObject(map, pos),
	TileObjectSprite(map, pos, cityTile.sprite),
	TileObjectCollidable(map, pos, {32,32,16}, cityTile.voxelMap),
	cityTile(cityTile), pos(pos), building(building)
{

}

BuildingSection::~BuildingSection()
{

}

};//namesapce OpenApoc

#include "game/city/buildingtile.h"
#include "framework/palette.h"
#include "game/apocresources/pck.h"
#include "framework/framework.h"

namespace OpenApoc {

std::vector<CityTile>
CityTile::loadTilesFromFile(Framework &fw)
{
	std::vector<CityTile> v;

	auto pal = fw.data->load_palette("xcom3/ufodata/PAL_04.DAT");

	auto sprites = fw.data->load_image_set("PCK:xcom3/ufodata/CITY.PCK:xcom3/ufodata/CITY.TAB");

	auto datFile = fw.data->load_file("xcom3/ufodata/CITYMAP.DAT", Data::FileMode::Read);

	int numTiles = sprites->images.size();

	int64_t datFileSize = PHYSFS_fileLength(datFile);

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
		v.push_back(tile);
	}

	PHYSFS_close(datFile);
	return v;
}

BuildingSection::BuildingSection(Tile *owningTile, CityTile &cityTile, Vec3<int> pos, Building *building)
	: TileObject(owningTile, Vec3<float>{(float)pos.x,(float)pos.y,(float)pos.z}, Vec3<float>{1.0f,1.0f,1.0f}, true, true, cityTile.sprite), cityTile(cityTile), pos(pos), building(building)
{

}

BuildingSection::~BuildingSection()
{

}

void
BuildingSection::update(unsigned int ticks)
{
	std::ignore = ticks;
}

TileObjectCollisionVoxels&
BuildingSection::getCollisionVoxels()
{
	return this->cityTile.collisionVoxels;
}

void
BuildingSection::processCollision(TileObject &otherObject)
{
	std::ignore = otherObject;
}

};//namesapce OpenApoc

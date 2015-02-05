#include "buildingtile.h"
#include "framework/palette.h"
#include "game/apocresources/pck.h"
#include "framework/framework.h"

namespace OpenApoc {

std::vector<CityTile>
CityTile::loadTilesFromFile(Framework &fw)
{
	std::vector<CityTile> v;

	auto pal = fw.data.load_palette("UFODATA/PAL_04.DAT");

	auto sprites = fw.data.load_image_set("PCK:UFODATA/CITY.PCK:UFODATA/CITY.TAB");

	auto datFile = fw.data.load_file("UFODATA/CITYMAP.DAT", "rb");

	int numTiles = sprites->images.size();

	int64_t datFileSize = al_fsize(datFile);

	int numDatEntries = datFileSize / 52;

	if (numDatEntries != numTiles)
	{
		std::cerr << "Number of city sprite tiles does not match number of dat entries (" 
			<< numDatEntries << " dat chunks, " << numTiles << " images\n";
	}

	std::cerr << "Loading " << numTiles << " city tiles\n";

	for (int t = 0; t < numTiles; t++)
	{
		CityTile tile;
		tile.sprite = sprites->images[t];
		v.push_back(tile);
	}

	al_fclose(datFile);
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

}

TileObjectCollisionVoxels&
BuildingSection::getCollisionVoxels()
{
	return this->cityTile.collisionVoxels;
}

void
BuildingSection::processCollision(TileObject &otherObject)
{

}

};//namesapce OpenApoc

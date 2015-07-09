#pragma once

#include "game/tileview/tile.h"
#include "game/tileview/tile_visible.h"
#include "game/tileview/tile_collidable.h"

/* MSVC warns about inherited virtual functions that are implemented
 * in different superclasses though multiple inheritance, even
 * if one is a subclass of the other. So disable that as we rely
 * on inherited subclasses of TileObject overriding various functions */
#ifdef _MSC_VER
#pragma warning( disable : 4250 )
#endif // _MSC_VER

namespace OpenApoc {

class Building;

class CityTile
{
public:
	std::shared_ptr<Image> sprite;
	std::shared_ptr<VoxelMap> voxelMap;

	static std::vector<CityTile> loadTilesFromFile(Framework &fw);
};

class BuildingSection : public TileObjectSprite, public TileObjectCollidable
{
	private:
		CityTile &cityTile;
		Vec3<int> pos;
		Building *building;
	public:

		BuildingSection(TileMap &map, CityTile &cityTile, Vec3<int> pos, Building *building);
		virtual ~BuildingSection();
};

}; //namespace OpenApoc

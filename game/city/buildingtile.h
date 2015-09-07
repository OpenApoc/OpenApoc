#pragma once

#include "game/tileview/tile.h"
#include "game/tileview/tile_visible.h"
#include "game/tileview/tile_collidable.h"
#include "game/rules/rules.h"

/* MSVC warns about inherited virtual functions that are implemented
 * in different superclasses though multiple inheritance, even
 * if one is a subclass of the other. So disable that as we rely
 * on inherited subclasses of TileObject overriding various functions */
#ifdef _MSC_VER
#pragma warning(disable : 4250)
#endif // _MSC_VER

namespace OpenApoc
{

class Building;

class BuildingTile : public TileObjectSprite, public TileObjectCollidable
{
  private:
	BuildingTileDef &tileDef;
	Vec3<int> pos;
	// May be NULL for no building
	Building *building;

  public:
	BuildingTile(TileMap &map, BuildingTileDef &tileDef, Vec3<int> pos, Building *building);
	virtual ~BuildingTile();
};

} // namespace OpenApoc

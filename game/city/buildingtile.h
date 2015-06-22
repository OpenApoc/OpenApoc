#pragma once

#include "game/tileview/tile.h"

namespace OpenApoc {

class Building;

class CityTile
{
public:
	std::shared_ptr<Image> sprite;

	static std::vector<CityTile> loadTilesFromFile(Framework &fw);
};

class BuildingSection : public TileObjectNonDirectionalSprite
{
	private:
		CityTile &cityTile;
		Vec3<int> pos;
		Building *building;
	public:

		BuildingSection(TileMap &map, CityTile &cityTile, Vec3<int> pos, Building *building);
		virtual ~BuildingSection();
		virtual void update(unsigned int ticks);
};

}; //namespace OpenApoc

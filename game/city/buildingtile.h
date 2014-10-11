#pragma once

#include "game/tileview/tile.h"

namespace OpenApoc {

class Building;

class CityTile
{
public:
	std::shared_ptr<Image> sprite;
	TileObjectCollisionVoxels collisionVoxels;

	static std::vector<CityTile> loadTilesFromFile(Framework &fw);
};

class BuildingSection : public TileObject
{
	private:
		CityTile &cityTile;
		Vec3<int> pos;
		Building *building;
	public:

		BuildingSection(Tile &owningTile, CityTile &cityTile, Vec3<int> pos, Building *building);
		virtual ~BuildingSection();
		virtual void update(unsigned int ticks);
		virtual TileObjectCollisionVoxels &getCollisionVoxels();
		virtual void processCollision(TileObject &otherObject);


};

}; //namespace OpenApoc

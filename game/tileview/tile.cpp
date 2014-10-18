#include "tile.h"
#include "framework/framework.h"

namespace OpenApoc {

TileMap::TileMap(Framework &fw, Vec3<int> size)
	: fw(fw), size(size)
{
	tiles.resize(size.z);
	for (int z = 0; z < size.z; z++)
	{
		tiles[z].resize(size.y);
		for (int y = 0; y < size.y; y++)
		{
			tiles[z][y].reserve(size.x);
			for (int x = 0; x < size.x; x++)
			{
				tiles[z][y].emplace_back(*this, Vec3<int>{x,y,z});
			}
		}
	}
}

void
TileMap::update(unsigned int ticks)
{
	//Default tilemap update calls update(ticks) on all tiles
	//Subclasses can optimise this if they know which tiles might be 'active'
	for (auto& plane : this->tiles)
	{
		for (auto& line : plane)
		{
			for (auto& tile : line)
				tile.update(ticks);
		}
	}
}

TileMap::~TileMap()
{
}

Tile::Tile(TileMap &map, Vec3<int> position)
	: map(map), position(position)
{
}

void
Tile::update(unsigned int ticks)
{
	for (auto &o : objects)
		o->update(ticks);
}

TileObject::TileObject(Tile &owningTile, Vec3<float> position, Vec3<float> size, bool visible, bool collides, std::shared_ptr<Image> sprite)
	: owningTile(owningTile), position(position), size(size), visible(visible), collides(collides), sprite(sprite)
{

}

TileObject::~TileObject()
{

}

Cubeoid<int>
TileObject::getBoundingBox()
{
	Vec3<int> p1 {(int)floor((float)position.x), (int)floor((float)position.y), (int)floor((float)position.z)};
	Vec3<int> p2 {(int)ceil((float)position.x + size.x), (int)ceil((float)position.y + size.y), (int)ceil((float)position.z + size.z)};
	return Cubeoid<int>{p1,p2};
}

Vec3<float>
TileObject::getSize()
{
	return this->size;
}

Vec3<float>
TileObject::getPosition()
{
	return this->position;
}

Image&
TileObject::getSprite()
{
	return *this->sprite;
}

TileObjectCollisionVoxels&
TileObject::getCollisionVoxels()
{
	return this->collisionVoxels;
}




}; //namespace OpenApoc

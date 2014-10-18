#pragma once

#include "framework/includes.h"

namespace OpenApoc {

class Framework;
class Image;
class TileMap;
class Tile;

class TileObjectCollisionVoxels
{
	Vec3<int> blockSize;
	std::vector<std::vector<std::vector<bool> > > voxelBits;
	public:
		bool collides(Vec3<float> thisOffset, const TileObjectCollisionVoxels &other, Vec3<float> otherOffset);
};

class TileObject
{
	public:
		//Every object is 'owned' by a single tile - this defines the point the
		//sprite will be drawn (so it will have the same x/y/z as the owning tile)
		Tile &owningTile;

		// Flag to set if the object is visible (IE should be drawn)
		bool visible;
		// Flag to set if the object can collide - without this set processCollision()
		// will never be called on this
		bool collides;

		std::shared_ptr<Image> sprite;
		Vec3<float> size;
		Vec3<float> position;
		TileObjectCollisionVoxels collisionVoxels;

		TileObject(Tile &owningTile, Vec3<float> position, Vec3<float> size, bool visible, bool collides, std::shared_ptr<Image> sprite);
		virtual ~TileObject();
		virtual void update(unsigned int ticks) = 0;
		virtual Cubeoid<int> getBoundingBox();
		virtual Vec3<float> getSize();
		virtual Vec3<float> getPosition();
		virtual TileObjectCollisionVoxels &getCollisionVoxels();
		virtual Image& getSprite();

		virtual void processCollision(TileObject &otherObject) = 0;
};

class Tile
{
	public:
		TileMap &map;
		Vec3<int> position;
		std::list<std::shared_ptr<TileObject> > objects;

		Tile(TileMap &map, Vec3<int> position);
		void update(unsigned int ticks);
};

class TileMap
{
	public:
		Framework &fw;
		std::vector < std::vector < std::vector < Tile > > > tiles;
		Vec3<int> size;

		TileMap (Framework &fw, Vec3<int> size);
		~TileMap();
		virtual void update(unsigned int ticks);
};
}; //namespace OpenApoc

#pragma once

#include "framework/includes.h"
#include <set>

namespace OpenApoc {

class Framework;
class Image;
class TileMap;
class Tile;
class WeaponBeam;

class TileObjectCollisionVoxels
{
	Vec3<int> blockSize;
	std::vector<std::vector<std::vector<bool> > > voxelBits;
	public:
		bool collides(Vec3<float> thisOffset, const TileObjectCollisionVoxels &other, Vec3<float> otherOffset);
};

class TileObject : public std::enable_shared_from_this<TileObject>
{
	protected:
		TileObject(TileMap &map, Vec3<float> position);
		Vec3<float> position;
	public:
		//Every object is 'owned' by a single tile - this defines the point the
		//sprite will be drawn (so it will have the same x/y/z as the owning tile)
		Tile *owningTile;
		TileObjectCollisionVoxels collisionVoxels;

		virtual ~TileObject();
		virtual void update(unsigned int ticks);
		void setPosition(Vec3<float> newPos);
		Vec3<float> getPosition();
};

class TileObjectSprite : public TileObject
{
public:
	TileObjectSprite(TileMap &map, Vec3<float> position)
		: TileObject(map, position){}
	virtual std::shared_ptr<Image> getSprite() = 0;
};

class TileObjectNonDirectionalSprite : public TileObjectSprite
{
private:
	 std::shared_ptr<Image> sprite;
public:
	TileObjectNonDirectionalSprite(TileMap &map, std::shared_ptr<Image> sprite, Vec3<float> position)
		: TileObjectSprite(map, position), sprite(sprite){}
	virtual std::shared_ptr<Image> getSprite();
};

class TileObjectDirectionalSprite : public TileObjectSprite
{
private:
	std::vector<std::pair<Vec3<float>, std::shared_ptr<Image>>> sprites;
	Vec3<float> direction;
public:
	TileObjectDirectionalSprite(TileMap &map, std::vector<std::pair<Vec3<float>, std::shared_ptr<Image>>> sprites, Vec3<float> position, Vec3<float> direction)
		: TileObjectSprite(map, position), sprites(sprites), direction(direction){}
	virtual std::shared_ptr<Image> getSprite();

	void setDirection(Vec3<float> d){this->direction = d;}
	Vec3<float> getDirection() {return this->direction;}
};

class Tile
{
	public:
		TileMap &map;
		Vec3<int> position;
		std::set<std::shared_ptr<TileObject> > objects;

		Tile(TileMap &map, Vec3<int> position);
};

class TileMap
{
	private:
		std::vector <Tile> tiles;
		std::set<std::shared_ptr<TileObject>> objects;
	public:
		Framework &fw;
		Tile* getTile(int x, int y, int z);
		Tile* getTile(Vec3<int> pos);
		//Returns the tile this point is 'within'
		Tile* getTile(Vec3<float> pos);
		Vec3<int> size;

		TileMap (Framework &fw, Vec3<int> size);
		~TileMap();
		virtual void update(unsigned int ticks);

		std::list<Tile*> findShortestPath(Vec3<int> origin, Vec3<int> destination);

		void removeObject(std::shared_ptr<TileObject> obj);
		void addObject(std::shared_ptr<TileObject> obj);

};
}; //namespace OpenApoc

#pragma once

#include "framework/includes.h"
#include <set>

namespace OpenApoc {

class Framework;
class Image;
class TileMap;
class Tile;
class Collision;
class VoxelMap;
class Renderer;
class TileView;

class TileObject : public std::enable_shared_from_this<TileObject>
{
	protected:
		TileObject(TileMap &map, Vec3<float> position, bool active = false, bool collides = false, bool visible = false, bool projectile = false);
		Vec3<float> position;
		Tile *owningTile;
		bool active;
		bool collides;
		bool visible;
		bool projectile;
	public:
		//Every object is 'owned' by a single tile - this defines the point the
		//sprite will be drawn (so it will have the same x/y/z as the owning tile)

		Tile* getOwningTile() const {return this->owningTile;}
		virtual ~TileObject();
		virtual void setPosition(Vec3<float> newPos);
		const Vec3<float>& getPosition() const;
		
		bool isActive() const {return active;}
		virtual void update(unsigned int ticks);

		bool isVisible() const {return visible;}
		virtual Vec3<float> getDrawPosition() const;
		virtual std::shared_ptr<Image> getSprite();

		bool isCollidable() const {return collides;}
		//Returns the number of voxels per tile
		virtual const Vec3<int> &getTileSizeInVoxels() const;
		//returns bounds in /number of voxels/
		virtual const Vec3<int> &getBounds() const;
		//Takes position in world space
		virtual bool hasVoxelAt(const Vec3<float> &worldPosition) const;
		virtual void handleCollision(const Collision &c);
		virtual void removeFromAffectedTiles();
		virtual void addToAffectedTiles();

		bool isProjectile() const {return projectile;}
		virtual void checkProjectileCollision();
		virtual void drawProjectile(TileView &v, Renderer &r, Vec2<int> screenPosition); 
};

class Tile
{
	public:
		TileMap &map;
		Vec3<int> position;

		std::set<std::shared_ptr<TileObject> > ownedObjects;
		std::set<std::shared_ptr<TileObject> > visibleObjects;
		std::set<std::shared_ptr<TileObject> > collideableObjects;

		std::set<std::shared_ptr<TileObject> > ownedProjectiles;

		Tile(TileMap &map, Vec3<int> position);

		Collision findCollision(Vec3<float> lineSegmentStart, Vec3<float> lineSegmentEnd);
};

class TileMap
{
	private:
		std::vector <Tile> tiles;
		std::set<std::shared_ptr<TileObject>> projectiles;
	public:
		std::set<std::shared_ptr<TileObject>> activeObjects;
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

		Collision findCollision(Vec3<float> lineSegmentStart, Vec3<float> lineSegmentEnd);
};
}; //namespace OpenApoc

#include "game/tileview/tile_collidable.h"
#include "game/tileview/voxel.h"
#include "framework/logger.h"

namespace OpenApoc {

TileObjectCollidable::TileObjectCollidable(TileMap &map, Vec3<float> position, Vec3<int> tileSizeInVoxels, std::shared_ptr<VoxelMap> voxels)
	: TileObject(map, position),
	tileSizeInVoxels(tileSizeInVoxels), voxels(voxels)
{
	this->collides = true;
	this->bounds.x = (voxels->getSize().x + (tileSizeInVoxels.x - 1)) / tileSizeInVoxels.x;
	this->bounds.y = (voxels->getSize().y + (tileSizeInVoxels.y - 1)) / tileSizeInVoxels.y;
	this->bounds.z = (voxels->getSize().z + (tileSizeInVoxels.z - 1)) / tileSizeInVoxels.z;
}

const Vec3<int>&
TileObjectCollidable::getTileSizeInVoxels() const
{
	return this->tileSizeInVoxels;
}

const Vec3<int>&
TileObjectCollidable::getBounds() const
{
	return this->bounds;
}

bool
TileObjectCollidable::hasVoxelAt(const Vec3<float> &worldPosition) const
{
	Vec3<float> transformedPos = worldPosition;
	transformedPos -= this->getPosition();
	transformedPos *= this->tileSizeInVoxels;
	return this->voxels->getBit(transformedPos);
}

void
TileObjectCollidable::handleCollision(const Collision &c)
{
	//Should be overridden
	LogWarning("Unhandled collision at {%f,%f,%f}", c.position.x, c.position.y,c.position.z);
	std::ignore = c;
}

void
TileObjectCollidable::setPosition(Vec3<float> newPos)
{
	TileObject::setPosition(newPos);
	this->removeFromAffectedTiles();
	this->addToAffectedTiles();
}

void
TileObjectCollidable::removeFromAffectedTiles()
{
	   auto thisPtr = std::dynamic_pointer_cast<TileObjectCollidable>(shared_from_this());
	   if (!thisPtr)
	   {
			   LogError("this not a TileObjectCollidable?");
			   return;
	   }
	   for (auto *tile : this->affectedTiles)
	   {
			   tile->collideableObjects.erase(thisPtr);
	   }
	   this->affectedTiles.clear();
}

void
TileObjectCollidable::addToAffectedTiles()
{
	   auto thisPtr = std::dynamic_pointer_cast<TileObjectCollidable>(shared_from_this());
	   if (!thisPtr)
	   {
			   LogError("this not a TileObjectCollidable?");
			   return;
	   }
	   //We want less, not less-than-or-equal-to as we want to be inclusive of
	   // the end points
	   for (int z = 0; z < this->bounds.z; z++)
	   {
			   for (int y = 0; y < this->bounds.y; y++)
			   {
					   for (int x = 0; x < this->bounds.x; x++)
					   {
							   Tile *t = this->owningTile->map.getTile(this->getPosition() + Vec3<float>{x,y,z});
							   // Can be null if out of bounds
							   if (!t)
									   continue;
							   this->affectedTiles.insert(t);
							   t->collideableObjects.insert(thisPtr);
					   }
			   }
	   }

}

};

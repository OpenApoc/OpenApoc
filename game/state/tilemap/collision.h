#pragma once

#include "library/sp.h"
#include "library/vec.h"
#include <list>

namespace OpenApoc
{

class TileObject;
class Projectile;
class Tile;

class Collision
{
  public:
	sp<TileObject> obj;
	sp<Projectile> projectile;
	Vec3<float> position;
	std::list<Vec3<int>> passedTiles;
	bool outOfRange = false;
	explicit operator bool() const { return obj != nullptr; }

	static Vec3<float> getLeadingOffset(Vec3<float> tarPosRelative, float ourVelocity,
	                                    Vec3<float> tarVelocity);
};

}; // namespace OpenApoc

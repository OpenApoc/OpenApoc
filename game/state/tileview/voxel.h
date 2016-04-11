#pragma once
#include "library/sp.h"
#include "library/vec.h"

namespace OpenApoc
{

class TileObject;
class Projectile;

class Collision
{
  public:
	sp<TileObject> obj;
	sp<Projectile> projectile;
	Vec3<float> position;
	explicit operator bool() const { return obj != nullptr; }
};

}; // namespace OpenApoc

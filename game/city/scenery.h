#pragma once

#include "library/sp.h"
#include "library/vec.h"

namespace OpenApoc
{

class TileObjectScenery;
class SceneryTileDef;
class Building;
class Collision;

class Scenery
{

  public:
	SceneryTileDef &tileDef;

	const Vec3<float> getPosition() const
	{ // The "position" is the center, so offset by {0.5,0.5}
		Vec3<float> offsetPos = pos;
		offsetPos += Vec3<float>{0.5, 0.5, 0.0};
		return offsetPos;
	}

	Vec3<int> pos;
	// May be NULL for no building
	sp<Building> building;
	sp<TileObjectScenery> tileObject;

	void handleCollision(Collision &c);

	Scenery(SceneryTileDef &tileDef, Vec3<int> pos, sp<Building> bld);
	~Scenery() = default;
};
} // namespace OpenApoc

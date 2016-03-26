#pragma once

#include "game/rules/scenery_tile_type.h"
#include "game/stateobject.h"
#include "library/sp.h"
#include "library/vec.h"
#include <set>

namespace OpenApoc
{

class TileObjectScenery;
class SceneryTileType;
class Building;
class Collision;
class GameState;
class TileMap;
class Doodad;
class City;

class Scenery : public std::enable_shared_from_this<Scenery>
{

  public:
	StateRef<SceneryTileType> type;

	const Vec3<float> getPosition() const
	{
		// The "position" is the center, so offset by {0.5,0.5,0.5}
		Vec3<float> offsetPos = currentPosition;
		offsetPos += Vec3<float>{0.5, 0.5, 0.5};
		return offsetPos;
	}

	Vec3<int> initialPosition;
	Vec3<float> currentPosition;

	bool damaged;
	bool falling;
	bool destroyed;

	void handleCollision(GameState &state, Collision &c);

	void update(GameState &state, unsigned int ticks);
	void collapse(GameState &state);

	bool canRepair() const;
	void repair(GameState &state);

	bool isAlive() const;

	sp<TileObjectScenery> tileObject;
	sp<Doodad> overlayDoodad;
	std::set<sp<Scenery>> supports;
	std::set<sp<Scenery>> supportedBy;
	// May be NULL for no building
	StateRef<Building> building;
	StateRef<City> city;

	Scenery();
	~Scenery() = default;
};
} // namespace OpenApoc

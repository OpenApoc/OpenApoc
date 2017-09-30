#pragma once

#include "game/state/stateobject.h"
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

	Vec3<float> getPosition() const
	{
		// The "position" is the center, so offset by {0.5,0.5,0.5}
		Vec3<float> offsetPos = currentPosition;
		offsetPos += Vec3<float>{0.5f, 0.5f, 0.5f};
		return offsetPos;
	}

	Vec3<int> initialPosition;
	Vec3<float> currentPosition;

	bool damaged;
	bool falling;
	bool destroyed;

	void handleCollision(GameState &state, Collision &c);
	// Returns true if sound and doodad were handled by it
	bool applyDamage(GameState &state, int power);
	// Handles mappart ceasing to exist (fatal damage or fell on something)
	void die(GameState &state);

	void update(GameState &state, unsigned int ticks);
	void collapse(GameState &state);

	bool canRepair() const;
	void repair(GameState &state);

	bool isAlive() const;

	// Following members are not serialized, but rather are set in City::initMap method

	sp<TileObjectScenery> tileObject;
	sp<Doodad> overlayDoodad;
	std::set<sp<Scenery>> supports;
	std::set<sp<Scenery>> supportedBy;
	StateRef<Building> building;
	StateRef<City> city;

	Scenery();
	~Scenery() = default;
};
} // namespace OpenApoc

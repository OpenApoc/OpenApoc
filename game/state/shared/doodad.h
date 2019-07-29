#pragma once

#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/vec.h"

namespace OpenApoc
{
class TileObjectDoodad;
class TileMap;
class DoodadType;
class Image;
class GameState;
class VoxelMap;

/* A doodad is a visual only effect (IE doesn't change the game state) for
 * animated sprites, like hit animations/explosions etc. They do not move(?) */
class Doodad : public std::enable_shared_from_this<Doodad>
{
  public:
	Doodad() = default;
	~Doodad() = default;

	Doodad(Vec3<float> position, Vec2<int> imageOffset, bool temporary, int lifetime,
	       sp<Image> image);
	Doodad(Vec3<float> position, StateRef<DoodadType> type);

	sp<Image> getSprite();
	const Vec2<int> &getImageOffset() const { return this->imageOffset; }
	void update(GameState &state, int ticks);
	const Vec3<float> &getPosition() const { return this->position; }

	void setPosition(Vec3<float> position);

	void remove(GameState &state);

	sp<TileObjectDoodad> tileObject;

	Vec3<float> position = {0, 0, 0};
	Vec2<int> imageOffset = {0, 0};
	bool temporary = false;
	int age = 0;
	int lifetime = 0;
	sp<VoxelMap> voxelMap;
	// A doodad is either a single image sprite
	sp<Image> sprite;
	// Or a DoodadType containing an animation
	StateRef<DoodadType> type;
};

} // namespace OpenApoc

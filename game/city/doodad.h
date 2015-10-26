#pragma once
#include "library/sp.h"
#include "library/vec.h"
#include "library/strings.h"

namespace OpenApoc
{
class TileObjectDoodad;
class TileMap;
class DoodadDef;
class Framework;
class Image;
class GameState;

/* A doodad is a visual only effect (IE doesn't change the game state) for
 * animated sprited, like hit animations/explosions etc. The do not move(?) */
class Doodad : public std::enable_shared_from_this<Doodad>
{
  public:
	Doodad(DoodadDef &def, Vec3<float> position);
	sp<Image> getSprite();
	const Vec2<int> &getImageOffset() const;
	void update(GameState &state, int ticks);
	const Vec3<float> &getPosition() const { return this->position; }
	~Doodad() = default;

	sp<TileObjectDoodad> tileObject;

  private:
	DoodadDef &def;
	Vec3<float> position;
	int age;
};
} // namespace OpenApoc

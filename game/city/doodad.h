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
	virtual sp<Image> getSprite() = 0;
	const Vec2<int> &getImageOffset() const { return this->imageOffset; };
	virtual void update(GameState &state, int ticks);
	const Vec3<float> &getPosition() const { return this->position; }
	virtual ~Doodad() = default;

	void setPosition(Vec3<float> position);

	void remove(GameState &state);

	sp<TileObjectDoodad> tileObject;

  protected:
	Doodad(Vec3<float> position, Vec2<int> imageOffset, bool temporary, int lifetime);
	Vec3<float> position;
	Vec2<int> imageOffset;
	bool temporary;
	int age;
	int lifetime;
};

class AnimatedDoodad : public Doodad
{
  public:
	AnimatedDoodad(const DoodadDef &def, Vec3<float> position);
	virtual ~AnimatedDoodad() = default;
	virtual sp<Image> getSprite() override;

  private:
	const DoodadDef &def;
};

class StaticDoodad : public Doodad
{
  public:
	StaticDoodad(sp<Image> sprite, Vec3<float> position, Vec2<int> imageOffset,
	             bool temporary = false, int lifetime = 0);
	virtual ~StaticDoodad() = default;
	virtual sp<Image> getSprite() override;

  private:
	sp<Image> sprite;
};

} // namespace OpenApoc

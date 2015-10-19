#pragma once
#include "library/sp.h"

#include "game/tileview/tile.h"

namespace OpenApoc
{

class TileObjectSprite : virtual public TileObject
{
  protected:
	sp<Image> sprite;
	sp<Image> strategySprite;

  public:
	TileObjectSprite(TileMap &map, Vec3<float> position, sp<Image> sprite,
	                 sp<Image> strategySprite);
	virtual sp<Image> getSprite() const override;
	virtual sp<Image> getStrategySprite() const override;
	virtual Vec3<float> getDrawPosition() const override;
};

class TileObjectDirectionalSprite : virtual public TileObject
{
  protected:
	std::vector<std::pair<Vec3<float>, sp<Image>>> sprites;
	std::vector<std::pair<Vec3<float>, sp<Image>>> strategySprites;
	Vec3<float> direction;

  public:
	TileObjectDirectionalSprite(TileMap &map, Vec3<float> position,
	                            std::vector<std::pair<Vec3<float>, sp<Image>>> sprites,
	                            std::vector<std::pair<Vec3<float>, sp<Image>>> strategySprites,
	                            Vec3<float> initialDirection = Vec3<float>{1, 0, 0});
	virtual sp<Image> getSprite() const override;
	virtual sp<Image> getStrategySprite() const override;
	virtual Vec3<float> getDrawPosition() const override;

	virtual void setDirection(const Vec3<float> &d) { this->direction = d; }
	virtual const Vec3<float> &getDirection() const { return this->direction; }
};

}; // namespace OpenApoc

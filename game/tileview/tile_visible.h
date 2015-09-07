#pragma once

#include "game/tileview/tile.h"

namespace OpenApoc
{

class TileObjectSprite : virtual public TileObject
{
  protected:
	std::shared_ptr<Image> sprite;

  public:
	TileObjectSprite(TileMap &map, Vec3<float> position, std::shared_ptr<Image> sprite);
	virtual std::shared_ptr<Image> getSprite() const override;
	virtual Vec3<float> getDrawPosition() const override;
};

class TileObjectDirectionalSprite : virtual public TileObject
{
  protected:
	std::vector<std::pair<Vec3<float>, std::shared_ptr<Image>>> sprites;
	Vec3<float> direction;

  public:
	TileObjectDirectionalSprite(TileMap &map, Vec3<float> position,
	                            std::vector<std::pair<Vec3<float>, std::shared_ptr<Image>>> sprites,
	                            Vec3<float> initialDirection = Vec3<float>{1, 0, 0});
	virtual std::shared_ptr<Image> getSprite() const override;
	virtual Vec3<float> getDrawPosition() const override;

	virtual void setDirection(const Vec3<float> &d) { this->direction = d; }
	virtual const Vec3<float> &getDirection() const { return this->direction; }
};

} // namespace OpenApoc

#include "game/tileview/tile_visible.h"

namespace OpenApoc
{

TileObjectSprite::TileObjectSprite(TileMap &map, Vec3<float> position,
                                   std::shared_ptr<Image> sprite,
                                   std::shared_ptr<Image> strategySprite)
    : TileObject(map, position), sprite(sprite), strategySprite(strategySprite)
{
	this->visible = true;
}

std::shared_ptr<Image> TileObjectSprite::getSprite() const { return this->sprite; }
std::shared_ptr<Image> TileObjectSprite::getStrategySprite() const { return this->strategySprite; }

TileObjectDirectionalSprite::TileObjectDirectionalSprite(
    TileMap &map, Vec3<float> position,
    std::vector<std::pair<Vec3<float>, std::shared_ptr<Image>>> sprites,
    std::vector<std::pair<Vec3<float>, std::shared_ptr<Image>>> strategySprites,
    Vec3<float> initialDirection)
    : TileObject(map, position), sprites(sprites), strategySprites(strategySprites),
      direction(initialDirection)
{
	this->visible = true;
}

std::shared_ptr<Image> TileObjectDirectionalSprite::getSprite() const
{
	float closestAngle = FLT_MAX;
	std::shared_ptr<Image> closestImage;
	for (auto &p : sprites)
	{
		float angle = glm::angle(glm::normalize(p.first), glm::normalize(this->getDirection()));
		if (angle < closestAngle)
		{
			closestAngle = angle;
			closestImage = p.second;
		}
	}
	return closestImage;
}

std::shared_ptr<Image> TileObjectDirectionalSprite::getStrategySprite() const
{
	float closestAngle = FLT_MAX;
	std::shared_ptr<Image> closestImage;
	for (auto &p : strategySprites)
	{
		float angle = glm::angle(glm::normalize(p.first), glm::normalize(this->getDirection()));
		if (angle < closestAngle)
		{
			closestAngle = angle;
			closestImage = p.second;
		}
	}
	return closestImage;
}

Vec3<float> TileObjectSprite::getDrawPosition() const
{
	/* By default the sprite draw position starts at the same point as the object position */
	return this->getPosition();
}

Vec3<float> TileObjectDirectionalSprite::getDrawPosition() const
{
	/* By default the sprite draw position starts at the same point as the object position */
	return this->getPosition();
}
};

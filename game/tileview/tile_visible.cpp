#include "game/tileview/tile_visible.h"

namespace OpenApoc {

TileObjectSprite::TileObjectSprite(TileMap &map, Vec3<float> position, std::shared_ptr<Image> sprite)
	: TileObject(map, position),
	sprite(sprite)
{
	this->visible = true;
}

std::shared_ptr<Image>
TileObjectSprite::getSprite()
{
	return this->sprite;
}

TileObjectDirectionalSprite::TileObjectDirectionalSprite(TileMap &map, Vec3<float> position, std::vector<std::pair<Vec3<float>, std::shared_ptr<Image>>> sprites, Vec3<float> initialDirection)
	: TileObject(map, position),
	sprites(sprites), direction(initialDirection)
{
	this->visible = true;
}

std::shared_ptr<Image>
TileObjectDirectionalSprite::getSprite()
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

Vec3<float>
TileObjectSprite::getDrawPosition() const
{
	/* By default the sprite draw position starts at the same point as the object position */
	return this->getPosition();
}

Vec3<float>
TileObjectDirectionalSprite::getDrawPosition() const
{
	/* By default the sprite draw position starts at the same point as the object position */
	return this->getPosition();
}

};

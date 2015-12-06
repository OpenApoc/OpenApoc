#include "game/tileview/tileobject_shadow.h"
#include "game/tileview/voxel.h"
#include "game/city/vehicle.h"

namespace OpenApoc
{

void TileObjectShadow::draw(Renderer &r, TileView &view, Vec2<float> screenPosition,
                            TileViewMode mode)
{
	std::ignore = view;
	auto vehicle = this->owner.lock();
	if (!vehicle)
	{
		LogError("Called with no owning vehicle object");
		return;
	}
	if (this->fellOffTheBottomOfTheMap)
	{
		return;
	}
	switch (mode)
	{
		case TileViewMode::Isometric:
		{
			float closestAngle = FLT_MAX;
			sp<Image> closestImage;
			for (auto &p : vehicle->type.directional_shadow_sprites)
			{
				float angle =
				    glm::angle(glm::normalize(p.first), glm::normalize(vehicle->getDirection()));
				if (angle < closestAngle)
				{
					closestAngle = angle;
					closestImage = p.second;
				}
			}
			if (!closestImage)
			{
				LogError("No image found for vehicle");
				return;
			}
			r.draw(closestImage, screenPosition - vehicle->type.shadow_offset);
			break;
		}
		case TileViewMode::Strategy:
		{
			// No shadows in strategy view
			break;
		}
		default:
			LogError("Unsupported view mode");
	}
}

void TileObjectShadow::setPosition(Vec3<float> newPosition)
{
	// This projects a line downwards and draws places the shadow at the z of the first thing hit

	auto shadowPosition = newPosition;
	auto c = map.findCollision(newPosition, Vec3<float>{newPosition.x, newPosition.y, 0});
	if (c)
	{
		shadowPosition.z = c.position.z;
		this->fellOffTheBottomOfTheMap = false;
	}
	else
	{
		// May be a normal occurance (e.g. landing pads have a 'hole'
		LogInfo("Nothing beneath {%f,%f,%f} to receive shadow", newPosition.x, newPosition.y,
		        newPosition.z);
		shadowPosition.z = 0;
		// Mark it as not to be drawn
		this->fellOffTheBottomOfTheMap = true;
	}

	TileObject::setPosition(shadowPosition);
}

TileObjectShadow::~TileObjectShadow() {}

TileObjectShadow::TileObjectShadow(TileMap &map, sp<Vehicle> vehicle)
    : TileObject(map, TileObject::Type::Vehicle, vehicle->getPosition(), Vec3<float>{0, 0, 0}),
      owner(vehicle), fellOffTheBottomOfTheMap(false)
{
}

} // namespace OpenApoc

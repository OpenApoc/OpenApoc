#include "game/tileview/tileobject_vehicle.h"

namespace OpenApoc
{

void TileObjectVehicle::draw(Renderer &r, TileView &view, Vec2<float> screenPosition,
                             TileViewMode mode)
{
	std::ignore = view;
	auto vehicle = this->vehicle.lock();
	if (!vehicle)
	{
		LogError("Called with no owning vehicle object");
		return;
	}
	switch (mode)
	{
		case TileViewMode::Isometric:
		{
			float closestAngle = FLT_MAX;
			sp<Image> closestImage;
			for (auto &p : vehicle->type.directional_sprites)
			{
				float angle =
				    glm::angle(glm::normalize(p.first), glm::normalize(this->getDirection()));
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
			r.draw(closestImage, screenPosition - vehicle->type.image_offset);
			break;
		}
		case TileViewMode::Strategy:
		{
			float closestAngle = FLT_MAX;
			sp<Image> closestImage;
			for (auto &p : vehicle->type.directional_strategy_sprites)
			{
				float angle =
				    glm::angle(glm::normalize(p.first), glm::normalize(this->getDirection()));
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
			// All strategy sprites so far are 8x8 so offset by 4 to draw from the center
			// FIXME: Not true for large sprites (2x2 UFOs?)
			r.draw(closestImage, screenPosition - Vec2<float>{4, 4});
			break;
			break;
		}
		default:
			LogError("Unsupported view mode");
	}
}

TileObjectVehicle::~TileObjectVehicle() {}

TileObjectVehicle::TileObjectVehicle(TileMap &map, sp<Vehicle> vehicle,
                                     Vec3<float> initialDirection)
    : TileObject(map, TileObject::Type::Vehicle, vehicle->getPosition(), Vec3<float>{0, 0, 0}),
      vehicle(vehicle), direction(initialDirection)
{
}

} // namespace OpenApoc

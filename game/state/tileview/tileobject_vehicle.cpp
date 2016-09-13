#include "game/state/tileview/tileobject_vehicle.h"
#include "framework/renderer.h"
#include "game/state/tileview/tile.h"
#include "library/voxel.h"

namespace OpenApoc
{

void TileObjectVehicle::draw(Renderer &r, TileTransform &transform, Vec2<float> screenPosition,
                             TileViewMode mode, int)
{
	std::ignore = transform;
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

			if (vehicle->type->type == VehicleType::Type::UFO)
			{
				if (vehicle->isCrashed())
				{
					closestImage = vehicle->type->crashed_sprite;
				}
				else
				{
					closestImage = *animationFrame;
				}
			}
			else
			{
				auto bank = VehicleType::Banking::Flat;
				if (this->getDirection().z >= 0.1f)
					bank = VehicleType::Banking::Ascending;
				else if (this->getDirection().z <= -0.1f)
					bank = VehicleType::Banking::Descending;
				auto it = vehicle->type->directional_sprites.find(bank);
				if (it == vehicle->type->directional_sprites.end())
				{
					// If missing the requested banking try flat
					it = vehicle->type->directional_sprites.find(VehicleType::Banking::Flat);
					if (it == vehicle->type->directional_sprites.end())
					{
						LogError("Vehicle type missing 'Flat' banking");
					}
				}
				for (auto &p : it->second)
				{
					float angle =
					    glm::angle(glm::normalize(p.first), glm::normalize(this->getDirection()));
					if (angle < closestAngle)
					{
						closestAngle = angle;
						closestImage = p.second;
					}
				}
			}

			if (!closestImage)
			{
				LogError("No image found for vehicle");
				return;
			}
			r.draw(closestImage, screenPosition - vehicle->type->image_offset);
			break;
		}
		case TileViewMode::Strategy:
		{
			float closestAngle = FLT_MAX;
			sp<Image> closestImage;
			for (auto &p : vehicle->type->directional_strategy_sprites)
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
		}
		default:
			LogError("Unsupported view mode");
	}
}

TileObjectVehicle::~TileObjectVehicle() = default;

TileObjectVehicle::TileObjectVehicle(TileMap &map, sp<Vehicle> vehicle)
    : TileObject(map, Type::Vehicle, Vec3<float>{0, 0, 0}), vehicle(vehicle), animationDelay(0)
{
	animationFrame = vehicle->type->animation_sprites.begin();
}

Vec3<float> TileObjectVehicle::getCentrePosition()
{
	Vec3<float> tileSize = {32, 32, 16};
	this->getVoxelMap()->calculateCentre();
	auto voxelCentre = this->getVoxelMap()->centre;
	auto objPos = this->getPosition();
	objPos -= this->getVoxelOffset();
	return Vec3<float>(objPos.x + voxelCentre.x / tileSize.x, objPos.y + voxelCentre.y / tileSize.y,
	                   objPos.z + voxelCentre.z / tileSize.z);
}

sp<VoxelMap> TileObjectVehicle::getVoxelMap() { return this->getVehicle()->type->voxelMap; }

sp<Vehicle> TileObjectVehicle::getVehicle() const { return this->vehicle.lock(); }

Vec3<float> TileObjectVehicle::getPosition() const
{
	auto v = this->vehicle.lock();
	if (!v)
	{
		LogError("Called with no owning vehicle object");
		return {0, 0, 0};
	}
	return v->getPosition();
}

void TileObjectVehicle::nextFrame(int ticks)
{
	auto v = this->vehicle.lock();
	animationDelay += ticks;
	if (v && animationDelay > 10)
	{
		animationDelay = 0;
		animationFrame++;
		if (animationFrame == v->type->animation_sprites.end())
		{
			animationFrame = v->type->animation_sprites.begin();
		}
	}
}

} // namespace OpenApoc

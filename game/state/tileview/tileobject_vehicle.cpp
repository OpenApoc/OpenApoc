#include "game/state/tileview/tileobject_vehicle.h"
#include "framework/renderer.h"
#include "game/state/city/vehiclemission.h"
#include "game/state/tileview/tile.h"
#include "library/voxel.h"
#include <glm/glm.hpp>
#include <glm/gtx/vector_angle.hpp>

namespace OpenApoc
{

void TileObjectVehicle::draw(Renderer &r, TileTransform &transform, Vec2<float> screenPosition,
                             TileViewMode mode, int currentLevel, bool friendly, bool hostile)
{
	std::ignore = transform;
	std::ignore = currentLevel;
	std::ignore = friendly;
	std::ignore = hostile;
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
    : TileObject(map, Type::Vehicle, vehicle->type->size), vehicle(vehicle), animationDelay(0)
{
	animationFrame = vehicle->type->animation_sprites.begin();
}

Vec3<float> TileObjectVehicle::getVoxelCentrePosition() const
{
	auto vtype = this->getVehicle()->type;

	Vec3<int> voxelCentre = {0, 0, 0};
	for (int x = 0; x < vtype->size.x; x++)
	{
		for (int y = 0; y < vtype->size.y; y++)
		{
			for (int z = 0; z < vtype->size.z; z++)
			{
				voxelCentre += getVoxelMap({x, y, z})->centre;
			}
		}
	}
	voxelCentre /= vtype->size.x * vtype->size.y * vtype->size.z;

	auto objPos = this->getCenter();
	objPos -= this->getVoxelOffset();
	return Vec3<float>(objPos.x + (float)voxelCentre.x / map.voxelMapSize.x,
	                   objPos.y + (float)voxelCentre.y / map.voxelMapSize.y,
	                   objPos.z + (float)voxelCentre.z / map.voxelMapSize.z);
}

sp<VoxelMap> TileObjectVehicle::getVoxelMap(Vec3<int> mapIndex) const
{
	auto vtype = this->getVehicle()->type;
	if (mapIndex.x >= vtype->size.x || mapIndex.y >= vtype->size.y || mapIndex.z >= vtype->size.z)
		return nullptr;

	float closestAngle = FLT_MAX;
	sp<VoxelMap> closestMap = nullptr;
	auto it = vtype->voxelMaps[mapIndex.z * (int)vtype->size.y * (int)vtype->size.x +
	                           mapIndex.y * (int)vtype->size.x + mapIndex.x];
	for (auto &p : it)
	{
		float angle = glm::angle(glm::normalize(p.first), glm::normalize(this->getDirection()));
		if (angle < closestAngle)
		{
			closestAngle = angle;
			closestMap = p.second;
		}
	}
	return closestMap;
}

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

void TileObjectVehicle::addToDrawnTiles(Tile *tile)
{
	auto v = getVehicle();
	if (!v)
	{
		return;
	}
	Vec3<int> maxCoords = {-1, -1, -1};
	for (auto intersectingTile : intersectingTiles)
	{
		int x = intersectingTile->position.x;
		int y = intersectingTile->position.y;
		int z = intersectingTile->position.z;

		// Vehicles are drawn in the topmost tile they intersect
		if (maxCoords.z * 1000 + maxCoords.x + maxCoords.y < z * 1000 + x + y)
		{
			tile = intersectingTile;
			maxCoords = {x, y, z};
		}
	}
	// Vehicles are also never drawn below level 1, so that when they take off they're drawn above
	// landing pad's scenery
	if (!v->missions.empty() && v->missions.front()->isTakingOff(*v))
	{
		tile = map.getTile(tile->position.x, tile->position.y, tile->position.z + 1);
	}

	TileObject::addToDrawnTiles(tile);
}
} // namespace OpenApoc

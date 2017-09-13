#include "game/state/tileview/tileobject_vehicle.h"
#include "framework/renderer.h"
#include "game/state/city/vehicle.h"
#include "game/state/city/vehiclemission.h"
#include "game/state/rules/vehicle_type.h"
#include "game/state/tileview/tile.h"
#include "library/voxel.h"
#include <glm/glm.hpp>
#include <glm/gtx/vector_angle.hpp>

namespace OpenApoc
{

void TileObjectVehicle::draw(Renderer &r, TileTransform &transform, Vec2<float> screenPosition,
                             TileViewMode mode, bool, int currentLevel, bool friendly, bool hostile)
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
    : TileObject(map, Type::Vehicle, {0.0f, 0.0f, 0.0f}), vehicle(vehicle), animationDelay(0)
{
	animationFrame = vehicle->type->animation_sprites.begin();
}

Vec3<float> TileObjectVehicle::getVoxelCentrePosition() const
{
	// We could do all the following, but since we always aim at voxelmap's center,
	// regardless of which bits are filled or not (only accounting for height)
	// it is enough to just offset center according to voxelmap's height
	// Leaving the code here though as it may be useful later
	/*
	auto vtype = this->getVehicle()->type;
	auto facing = vtype->getVoxelMapFacing(getDirection());
	auto size = vtype->size.at(facing);

	Vec3<int> voxelCentre = {0, 0, 0};
	for (int x = 0; x < size.x; x++)
	{
	    for (int y = 0; y < size.y; y++)
	    {
	        for (int z = 0; z < size.z; z++)
	        {
	            voxelCentre += getVoxelMap({x, y, z})->centre;
	        }
	    }
	}
	voxelCentre /= size.x * size.y * size.z;

	auto objPos = this->getCenter();
	objPos -= this->getVoxelOffset();
	return Vec3<float>(objPos.x + (float)voxelCentre.x / map.voxelMapSize.x,
	                   objPos.y + (float)voxelCentre.y / map.voxelMapSize.y,
	                   objPos.z + (float)voxelCentre.z / map.voxelMapSize.z);
	*/

	// Simple version:
	auto objPos = this->getCenter();
	return Vec3<float>(objPos.x, objPos.y,
	                   objPos.z - getVoxelOffset().z +
	                       (float)getVehicle()->type->height / 2.0f / 16.0f);
}

sp<VoxelMap> TileObjectVehicle::getVoxelMap(Vec3<int> mapIndex, bool los) const
{
	auto vtype = this->getVehicle()->type;
	auto facing = vtype->getVoxelMapFacing(getDirection());
	auto size = vtype->size.at(facing);
	if (mapIndex.x >= size.x || mapIndex.y >= size.y || mapIndex.z >= size.z)
		return nullptr;
	if (los)
	{
		return vtype->voxelMapsLOS.at(facing).at(mapIndex.z * size.y * size.x +
		                                         mapIndex.y * size.x + mapIndex.x);
	}
	else
	{
		return vtype->voxelMaps.at(facing).at(mapIndex.z * size.y * size.x + mapIndex.y * size.x +
		                                      mapIndex.x);
	}
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

void TileObjectVehicle::setPosition(Vec3<float> newPosition)
{
	auto vtype = this->getVehicle()->type;
	auto facing = vtype->getVoxelMapFacing(getDirection());
	auto size = vtype->size.at(facing);

	setBounds({size.x, size.y, size.z});

	TileObject::setPosition(newPosition);
}

void TileObjectVehicle::addToDrawnTiles(Tile *tile)
{
	auto v = getVehicle();
	if (!v)
	{
		return;
	}
	Vec3<int> maxCoords = {-1, -1, -1};
	for (auto &intersectingTile : intersectingTiles)
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

const Vec3<float> &TileObjectVehicle::getDirection() const { return this->getVehicle()->velocity; }

void TileObjectVehicle::setDirection(const Vec3<float> &dir)
{
	this->getVehicle()->facing = dir;
	this->getVehicle()->velocity = dir;
}
} // namespace OpenApoc

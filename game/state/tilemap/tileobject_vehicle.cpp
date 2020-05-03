#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include "game/state/tilemap/tileobject_vehicle.h"
#include "framework/renderer.h"
#include "game/state/city/vehicle.h"
#include "game/state/city/vehiclemission.h"
#include "game/state/rules/city/vehicletype.h"
#include "game/state/tilemap/tilemap.h"
#include "library/voxel.h"
#include <glm/glm.hpp>
#include <glm/gtx/vector_angle.hpp>

namespace OpenApoc
{

void TileObjectVehicle::draw(Renderer &r, TileTransform &transform, Vec2<float> screenPosition,
                             TileViewMode mode, bool visible, int currentLevel, bool friendly,
                             bool hostile)
{
	auto vehicle = this->vehicle.lock();
	if (!vehicle)
	{
		LogError("Called with no owning vehicle object");
		return;
	}
	drawStatic(r, vehicle, transform, screenPosition, mode, visible, currentLevel, friendly,
	           hostile);
}

void TileObjectVehicle::drawStatic(Renderer &r, sp<Vehicle> vehicle, TileTransform &transform,
                                   Vec2<float> screenPosition, TileViewMode mode, bool, int,
                                   bool friendly, bool hostile)
{
	static const Colour COLOUR_TRANSPARENT = {255, 255, 255, 95};

	static const Colour COLOUR_STUNNED = {128, 128, 255, 255};

	static const int offset_arrow = 5;
	static const int offset_large = 1;

	static const std::map<float, int> offset_dir_map = {
	    {0.0f, 0},
	    {0.25f * (float)M_PI, 1},
	    {0.5f * (float)M_PI, 2},
	    {0.75f * (float)M_PI, 3},
	    {(float)M_PI, 4},
	    {1.25f * (float)M_PI, 5},
	    {1.5f * (float)M_PI, 6},
	    {1.75f * (float)M_PI, 7},
	};

	std::ignore = transform;

	switch (mode)
	{
		case TileViewMode::Isometric:
		{
			sp<Image> closestImage;

			if (vehicle->type->type == VehicleType::Type::UFO)
			{
				if (vehicle->crashed)
				{
					closestImage = vehicle->type->crashed_sprite;
				}
				else
				{
					closestImage = *vehicle->animationFrame;
				}
			}
			else
			{
				closestImage =
				    vehicle->type->directional_sprites.at(vehicle->banking).at(vehicle->direction);
			}

			if (!closestImage)
			{
				LogError("No image found for vehicle");
				return;
			}
			if (vehicle->isCloaked())
			{
				r.drawTinted(closestImage, screenPosition - vehicle->type->image_offset,
				             COLOUR_TRANSPARENT);
			}
			else if (vehicle->stunTicksRemaining > 0)
			{
				r.drawTinted(closestImage, screenPosition - vehicle->type->image_offset,
				             COLOUR_STUNNED);
			}
			else
			{
				r.draw(closestImage, screenPosition - vehicle->type->image_offset);
			}
			break;
		}
		case TileViewMode::Strategy:
		{
			float closestDiff = FLT_MAX;
			int facing_offset = 0;
			for (auto &p : offset_dir_map)
			{
				float d1 = p.first - vehicle->facing;
				if (d1 < 0.0f)
				{
					d1 += 2.0f * (float)M_PI;
				}
				float d2 = vehicle->facing - p.first;
				if (d2 < 0.0f)
				{
					d2 += 2.0f * (float)M_PI;
				}
				float diff = std::min(d1, d2);
				if (diff < closestDiff)
				{
					closestDiff = diff;
					facing_offset = p.second;
				}
			}

			// 1 = friendly, 0 = enemy, 2 = neutral
			int side_offset = friendly ? 1 : (hostile ? 0 : 2);

			switch (vehicle->type->mapIconType)
			{
				case VehicleType::MapIconType::Arrow:
					r.draw(vehicle->strategyImages->at(side_offset * 14 + offset_arrow +
					                                   facing_offset),
					       screenPosition - Vec2<float>{4, 4});
					break;
				case VehicleType::MapIconType::SmallCircle:
					r.draw(vehicle->strategyImages->at(side_offset * 14),
					       screenPosition - Vec2<float>{4, 4});
					break;
				case VehicleType::MapIconType::LargeCircle:
					r.draw(vehicle->strategyImages->at(side_offset * 14 + offset_large + 0),
					       screenPosition - Vec2<float>{8.0f, 8.0f});
					r.draw(vehicle->strategyImages->at(side_offset * 14 + offset_large + 1),
					       screenPosition - Vec2<float>{0.0f, 8.0f});
					r.draw(vehicle->strategyImages->at(side_offset * 14 + offset_large + 2),
					       screenPosition - Vec2<float>{8.0f, 0.0f});
					r.draw(vehicle->strategyImages->at(side_offset * 14 + offset_large + 3),
					       screenPosition - Vec2<float>{0.0f, 0.0f});
					break;
			}

			break;
		}
		default:
			LogError("Unsupported view mode");
	}
}

TileObjectVehicle::~TileObjectVehicle() = default;

TileObjectVehicle::TileObjectVehicle(TileMap &map, sp<Vehicle> vehicle)
    : TileObject(map, Type::Vehicle, {0.0f, 0.0f, 0.0f}), vehicle(vehicle)
{
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
	auto v = getVehicle();
	// Fire at crashed's top or ground's centre
	if (v->crashed || v->type->isGround())
	{
		return Vec3<float>(objPos.x, objPos.y, objPos.z + (float)v->type->height / 2.0f / 16.0f);
	}
	// Fire at flyer's centre
	else
	{
		return Vec3<float>(objPos.x, objPos.y, objPos.z);
	}
}

sp<VoxelMap> TileObjectVehicle::getVoxelMap(Vec3<int> mapIndex, bool los) const
{
	auto v = this->getVehicle();
	auto vtype = v->type;
	auto facing = vtype->getVoxelMapFacing(v->facing);
	auto size = vtype->size.at(facing);
	if (mapIndex.x >= size.x || mapIndex.y >= size.y || mapIndex.z >= size.z)
	{
		return nullptr;
	}
	// Crashed vehicles have proper voxel models
	if (los || v->crashed)
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

void TileObjectVehicle::setPosition(Vec3<float> newPosition)
{
	auto v = this->getVehicle();
	auto vtype = v->type;
	auto facing = vtype->getVoxelMapFacing(v->facing);
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

} // namespace OpenApoc

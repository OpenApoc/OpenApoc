#include "game/tileview/tileobject_shadow.h"
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
	switch (mode)
	{
		case TileViewMode::Isometric:
		{
			float closestAngle = FLT_MAX;
			sp<Image> closestImage;
			for (auto &p : vehicle->def.directionalShadowSprites)
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
			r.draw(closestImage, screenPosition - vehicle->def.imageOffset);
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
	// This finds the next scenery tile on or below newPosition and sets the
	// real position to that

	// Truncate down to int
	Vec3<int> pos = {newPosition.x, newPosition.y, newPosition.z};
	bool found = false;
	// and keep going down until we find a tile with scenery
	while (!found && pos.z >= 0)
	{
		auto *t = map.getTile(pos);
		for (auto &obj : t->ownedObjects)
		{
			if (obj->getType() == TileObject::Type::Scenery)
			{
				newPosition.z = obj->getPosition().z;
				// FIXME: I /think/ this is due to the sprite offset in the pck not being handled,
				// but here's a workaround to make it look kinda-right
				newPosition.x += 0.5;
				newPosition.y += 0.5;
				found = true;
				break;
			}
		}
		pos.z--;
	}
	if (!found)
	{
		LogWarning("No scenery found below {%f,%f,%f}", newPosition.x, newPosition.y,
		           newPosition.z);
		newPosition.z = 0;
	}

	TileObject::setPosition(newPosition);
}

TileObjectShadow::~TileObjectShadow() {}

TileObjectShadow::TileObjectShadow(TileMap &map, sp<Vehicle> vehicle)
    : TileObject(map, TileObject::Type::Vehicle, vehicle->getPosition(), Vec3<float>{0, 0, 0}),
      owner(vehicle)
{
}

} // namespace OpenApoc

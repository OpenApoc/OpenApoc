#include "game/state/tilemap/tileobject_shadow.h"
#include "framework/logger.h"
#include "framework/renderer.h"
#include "game/state/battle/battleitem.h"
#include "game/state/battle/battleunit.h"
#include "game/state/city/vehicle.h"
#include "game/state/rules/aequipmenttype.h"
#include "game/state/rules/battle/battleunitanimationpack.h"
#include "game/state/rules/city/vehicletype.h"
#include "game/state/shared/aequipment.h"
#include "game/state/tilemap/collision.h"
#include "game/state/tilemap/tileobject_battleunit.h"
#include <glm/glm.hpp>
#include <glm/gtx/vector_angle.hpp>

namespace OpenApoc
{

void TileObjectShadow::draw(Renderer &r, TileTransform &transform, Vec2<float> screenPosition,
                            TileViewMode mode, bool visible, int, bool, bool)
{
	if (!visible)
		return;
	std::ignore = transform;
	auto vehicle = this->ownerVehicle.lock();
	auto unit = this->ownerBattleUnit.lock();
	auto item = this->ownerBattleItem.lock();
	if (!vehicle && !unit && !item)
	{
		LogError("Called with no owning object");
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
			if (vehicle)
			{
				r.draw(vehicle->type->directional_shadow_sprites[vehicle->shadowDirection],
				       screenPosition - vehicle->type->shadow_offset);
			}
			if (unit)
			{
				// Bodies on the ground drop no shadows
				if (!unit->isConscious() && !unit->falling)
					break;
				unit->agent->getAnimationPack()->drawShadow(
				    r, screenPosition, unit->agent->type->shadow_pack, unit->displayedItem,
				    unit->facing, unit->current_body_state, unit->target_body_state,
				    unit->current_hand_state, unit->target_hand_state,
				    unit->usingLift ? MovementState::None : unit->current_movement_state,
				    unit->getBodyAnimationFrame(), unit->getHandAnimationFrame(),
				    unit->getDistanceTravelled(), visible);
			}
			if (item)
			{
				// Items on the ground give no shadows
				if (!item->falling)
					break;
				if (item->item->type->dropped_shadow_sprite)
					r.draw(item->item->type->dropped_shadow_sprite,
					       screenPosition - item->item->type->shadow_offset);
			}
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

void TileObjectShadow::addToDrawnTiles(Tile *tile)
{
	if (ownerBattleUnit.lock())
	{
		Vec3<int> maxCoords = {-1, -1, -1};
		for (auto &intersectingTile : intersectingTiles)
		{
			int x = intersectingTile->position.x;
			int y = intersectingTile->position.y;
			int z = intersectingTile->position.z;

			// Shadows are drawn in the topmost tile they intersect
			if (maxCoords.z * 1000 + maxCoords.x + maxCoords.y < z * 1000 + x + y)
			{
				tile = intersectingTile;
				maxCoords = {x, y, z};
			}
		}
	}
	TileObject::addToDrawnTiles(tile);
}

void TileObjectShadow::setPosition(Vec3<float> newPosition)
{
	static const std::set<TileObject::Type> mapPartSet = {
	    TileObject::Type::Ground, TileObject::Type::LeftWall, TileObject::Type::RightWall,
	    TileObject::Type::Feature, TileObject::Type::Scenery};

	// This projects a line downwards and draws places the shadow at the z of the first thing hit

	auto shadowPosition = newPosition;
	auto c =
	    map.findCollision(newPosition, Vec3<float>{newPosition.x, newPosition.y, -1}, mapPartSet);
	if (c)
	{
		shadowPosition.z = c.position.z;
		this->fellOffTheBottomOfTheMap = false;
	}
	else
	{
		shadowPosition.z = 0;
		// Mark it as not to be drawn
		this->fellOffTheBottomOfTheMap = true;
	}

	this->shadowPosition = shadowPosition;

	auto unit = ownerBattleUnit.lock();
	if (unit)
	{
		setBounds({unit->tileObject->getBounds().x, unit->tileObject->getBounds().y, 0.0f});
	}
	TileObject::setPosition(shadowPosition);
}

TileObjectShadow::~TileObjectShadow() = default;

TileObjectShadow::TileObjectShadow(TileMap &map, sp<Vehicle> vehicle)
    : TileObject(map, Type::Shadow, Vec3<float>{0, 0, 0}), ownerVehicle(vehicle),
      fellOffTheBottomOfTheMap(false)
{
}
TileObjectShadow::TileObjectShadow(TileMap &map, sp<BattleUnit> unit)
    : TileObject(map, Type::Shadow, Vec3<float>{0, 0, 0}), ownerBattleUnit(unit),
      fellOffTheBottomOfTheMap(false)
{
	if (unit->isLarge())
		setBounds({2.0f, 2.0f, 2.0f});
	else
		setBounds({1.0f, 1.0f, 1.0f});
}
TileObjectShadow::TileObjectShadow(TileMap &map, sp<BattleItem> item)
    : TileObject(map, Type::Shadow, Vec3<float>{0, 0, 0}), ownerBattleItem(item),
      fellOffTheBottomOfTheMap(false)
{
}

Vec3<float> TileObjectShadow::getPosition() const { return this->shadowPosition; }

} // namespace OpenApoc

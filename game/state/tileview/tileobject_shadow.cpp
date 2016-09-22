#include "game/state/tileview/tileobject_shadow.h"
#include "framework/renderer.h"
#include "game/state/battle/battleitem.h"
#include "game/state/battle/battleunit.h"
#include "game/state/city/vehicle.h"
#include "game/state/tileview/collision.h"

namespace OpenApoc
{

void TileObjectShadow::draw(Renderer &r, TileTransform &transform, Vec2<float> screenPosition,
                            TileViewMode mode, int)
{
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
				float closestAngle = FLT_MAX;
				sp<Image> closestImage;
				for (auto &p : vehicle->type->directional_shadow_sprites)
				{
					float angle = glm::angle(glm::normalize(p.first),
					                         glm::normalize(vehicle->getDirection()));
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
				r.draw(closestImage, screenPosition - vehicle->type->shadow_offset);
			}
			if (unit)
			{
				// Bodies on the ground drop no shadows
				if (!unit->isConscious() && !unit->falling)
					break;
				unit->agent->getAnimationPack()->drawShadow(r, screenPosition,
					unit->agent->type->shadow_pack,
					unit->agent->getItemInHands(), unit->facing,
					unit->current_body_state, unit->target_body_state,
					unit->current_hand_state, unit->target_hand_state,
					unit->usingLift ? AgentType::MovementState::None : unit->current_movement_state, unit->getBodyAnimationFrame(), unit->getHandAnimationFrame(), unit->getDistanceTravelled());
			}
			if (item)
			{
				// Items on the ground give no shadows
				if (item->supported)
					break;
				if (item->item->type->dropped_shadow_sprite)
					r.draw(item->item->type->dropped_shadow_sprite, screenPosition - item->item->type->shadow_offset);
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

void TileObjectShadow::setPosition(Vec3<float> newPosition)
{
	// This projects a line downwards and draws places the shadow at the z of the first thing hit

	auto shadowPosition = newPosition;
	auto c = map.findCollision(newPosition, Vec3<float>{newPosition.x, newPosition.y, -1});
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

	this->shadowPosition = shadowPosition;

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
	if (unit->agent->type->large)
		setBounds({ 2.0f, 2.0f, 2.0f });
	else
		setBounds({ 1.0f, 1.0f, 1.0f });
}
TileObjectShadow::TileObjectShadow(TileMap &map, sp<BattleItem> item)
    : TileObject(map, Type::Shadow, Vec3<float>{0, 0, 0}), ownerBattleItem(item),
      fellOffTheBottomOfTheMap(false)
{
}

Vec3<float> TileObjectShadow::getPosition() const { return this->shadowPosition; }

} // namespace OpenApoc
